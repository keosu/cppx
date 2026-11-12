// cppx.logging - Lightweight logging system
// Provides leveled logging with color support and file rotation

module;

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <Windows.h>
  #include <fcntl.h>
  #include <io.h>

  #include <cstdio>
#endif

export module cppx.logging;

import std;
import cppx.error;
#ifdef CPPX_FAULT_INJECTION
import cppx.fault_injection;
#endif
import cppx.string_utils;
import cppx.time_utils;

namespace cppx {

// ============================================================================
// Log levels
// ============================================================================

export enum class log_level { debug = 0, info = 1, warn = 2, error = 3, off = 4 };

export constexpr std::string_view log_level_name(log_level level) {
  switch (level) {
    case log_level::debug: return "DEBUG";
    case log_level::info: return "INFO";
    case log_level::warn: return "WARN";
    case log_level::error: return "ERROR";
    case log_level::off: return "OFF";
    default: return "UNKNOWN";
  }
}

// ============================================================================
// ANSI colors
// ============================================================================

namespace ansi {
constexpr std::string_view reset = "\033[0m";
constexpr std::string_view bold = "\033[1m";
constexpr std::string_view red = "\033[31m";
constexpr std::string_view green = "\033[32m";
constexpr std::string_view yellow = "\033[33m";
constexpr std::string_view blue = "\033[34m";
constexpr std::string_view magenta = "\033[35m";
constexpr std::string_view cyan = "\033[36m";
constexpr std::string_view white = "\033[37m";
constexpr std::string_view gray = "\033[90m";
}  // namespace ansi

constexpr std::string_view log_level_color(log_level level) {
  switch (level) {
    case log_level::debug: return ansi::gray;
    case log_level::info: return ansi::green;
    case log_level::warn: return ansi::yellow;
    case log_level::error: return ansi::red;
    default: return ansi::reset;
  }
}

// ============================================================================
// Time formatting
// ============================================================================

inline std::string format_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::floor<std::chrono::milliseconds>(now) -
            std::chrono::floor<std::chrono::seconds>(now);
  return std::format("{}.{:03}", format_time(now), ms.count());
}

// ============================================================================
// Log sink interface
// ============================================================================

class log_sink {
 public:
  virtual ~log_sink() = default;
  virtual void write(const std::string& message) = 0;
  virtual void flush() = 0;
};

// Console sink with UTF-8 and emoji support
class console_sink : public log_sink {
 public:
  explicit console_sink(bool use_color = true) : use_color_(use_color) {
// Enable UTF-8 output on Windows
#ifdef _WIN32
    // Set console output code page to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Enable ANSI escape sequences on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
      DWORD dwMode = 0;
      if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
      }
    }

    // Store console handle
    console_handle_ = hOut;

    // Check if output is redirected (e.g., piped to xmake test)
    // If redirected, GetFileType returns FILE_TYPE_PIPE or FILE_TYPE_DISK
    DWORD type = GetFileType(hOut);
    is_redirected_ = (type != FILE_TYPE_CHAR);
#endif
  }

  void write(const std::string& message) override {
#ifdef _WIN32
    // If output is redirected (e.g., xmake test captures stdout),
    // use standard streams for compatibility
    if (is_redirected_) {
      std::cout << message << std::endl;
    } else {
      // Use WriteConsoleW for proper UTF-8 support on Windows console
      // Convert UTF-8 to UTF-16
      int wlen = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
      if (wlen > 0) {
        std::vector<wchar_t> wbuf(wlen + 1);
        MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, wbuf.data(), wlen);

        // Write to console
        DWORD written = 0;
        WriteConsoleW(console_handle_, wbuf.data(), wlen - 1, &written, nullptr);
        WriteConsoleW(console_handle_, L"\n", 1, &written, nullptr);
      }
    }
#else
    std::cout << message << std::endl;
#endif
  }

  void flush() override {
    std::cout.flush();
    if (!std::cout) throw std::runtime_error("Console flush failed");
  }

  bool use_color() const { return use_color_; }

 private:
  bool use_color_;
#ifdef _WIN32
  HANDLE console_handle_;
  bool is_redirected_ = false;  // True if output is redirected/piped
#endif
};

// File sink with rotation
class file_sink : public log_sink {
 public:
  explicit file_sink(const std::filesystem::path& path,
                     size_t max_size = 10 * 1024 * 1024  // 10 MB default
                     )
      : path_(path), max_size_(max_size), current_size_(0) {
    open_file();
  }

  void write(const std::string& message) override {
    std::lock_guard<std::mutex> lock(mutex_);

    if (max_size_ > 0 && current_size_ > 0 &&
        (current_size_ >= max_size_ || message.size() >= max_size_ - current_size_)) {
      rotate();
    }

    if (!file_.is_open()) throw std::runtime_error("Log file is not open");
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::log_write)) file_.setstate(std::ios::badbit);
#endif
    file_ << message << '\n';
    current_size_ += message.size() + 1;
  }

  void flush() override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open()) throw std::runtime_error("Log file is not open");
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::log_flush)) file_.setstate(std::ios::badbit);
#endif
    file_.flush();
  }

 private:
  void open_file() {
    file_.exceptions(std::ios::badbit | std::ios::failbit);
    file_.open(path_, std::ios::app | std::ios::binary);
    if (file_.is_open()) {
      file_.seekp(0, std::ios::end);
      auto position = file_.tellp();
      if (position < 0 || !std::in_range<size_t>(static_cast<std::streamoff>(position)))
        throw std::runtime_error("Invalid log file size");
      current_size_ = static_cast<size_t>(position);
    }
  }

  void rotate() {
    if (file_.is_open()) {
      file_.close();
    }

    // Rotate existing files
    for (int i = 4; i >= 0; --i) {
      auto old_path = path_;
      if (i > 0) {
        old_path += "." + std::to_string(i);
      }

      if (std::filesystem::exists(old_path)) {
        if (i == 4) {
          std::filesystem::remove(old_path);
        } else {
          auto new_path = path_;
          new_path += "." + std::to_string(i + 1);
          std::filesystem::rename(old_path, new_path);
        }
      }
    }

    current_size_ = 0;
    open_file();
  }

  std::filesystem::path path_;
  size_t max_size_;
  size_t current_size_;
  std::ofstream file_;
  std::mutex mutex_;
};

// ============================================================================
// Logger class
// ============================================================================

export class logger {
 public:
  explicit logger(std::string name) : name_(std::move(name)), level_(log_level::info) {
    // Default console sink
    add_console_sink(true);
  }

  // Set log level
  void set_level(log_level level) noexcept { level_.store(level, std::memory_order_relaxed); }

  [[nodiscard]] log_level get_level() const { return level_.load(std::memory_order_relaxed); }

  // Add sinks
  void add_console_sink(bool use_color = true) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_sink_ = std::make_unique<console_sink>(use_color);
  }

  void add_file_sink(const std::filesystem::path& path, size_t max_size = 10 * 1024 * 1024) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto replacement = std::make_unique<file_sink>(path, max_size);
    if (file_sink_) file_sink_->flush();
    file_sink_ = std::move(replacement);
  }

  void remove_console_sink() {
    std::lock_guard<std::mutex> lock(mutex_);
    console_sink_.reset();
  }

  void remove_file_sink() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto detached = std::move(file_sink_);
    if (detached) detached->flush();
  }

  // Log methods
  template <typename... Args>
  void debug(std::format_string<Args...> fmt, Args&&... args) {
    log(log_level::debug, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void info(std::format_string<Args...> fmt, Args&&... args) {
    log(log_level::info, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void warn(std::format_string<Args...> fmt, Args&&... args) {
    log(log_level::warn, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void error(std::format_string<Args...> fmt, Args&&... args) {
    log(log_level::error, std::format(fmt, std::forward<Args>(args)...));
  }

  // Simple string log methods
  void debug(std::string_view message) { log(log_level::debug, std::string(message)); }

  void info(std::string_view message) { log(log_level::info, std::string(message)); }

  void warn(std::string_view message) { log(log_level::warn, std::string(message)); }

  void error(std::string_view message) { log(log_level::error, std::string(message)); }

  // Flush all sinks
  void flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (console_sink_) {
      console_sink_->flush();
    }
    if (file_sink_) {
      file_sink_->flush();
    }
  }

  // Destructors cannot throw. log_scope retains its failure here for later inspection.
  void record_error(std::exception_ptr error) noexcept {
    try {
      std::lock_guard lock(error_mutex_);
      last_error_ = std::move(error);
    } catch (...) {
    }
  }
  [[nodiscard]] std::exception_ptr last_error() const {
    std::lock_guard lock(error_mutex_);
    return last_error_;
  }

 private:
  void log(log_level level, const std::string& message) {
    if (level < get_level()) {
      return;
    }

    std::string timestamp = format_timestamp();
    std::string level_str(log_level_name(level));

    // Format: [timestamp] [LEVEL] [logger_name] message
    std::string formatted;

    std::lock_guard<std::mutex> lock(mutex_);

    // Console output (with color if enabled)
    if (console_sink_) {
      if (console_sink_->use_color()) {
        formatted = std::format("{}[{}]{} {}[{}]{} {}[{}]{} {}", ansi::gray, timestamp, ansi::reset,
                                log_level_color(level), level_str, ansi::reset, ansi::cyan, name_,
                                ansi::reset, message);
      } else {
        formatted = std::format("[{}] [{}] [{}] {}", timestamp, level_str, name_, message);
      }
      console_sink_->write(formatted);
    }

    // File output (no color)
    if (file_sink_) {
      formatted = std::format("[{}] [{}] [{}] {}", timestamp, level_str, name_, message);
      file_sink_->write(formatted);
    }
  }

  mutable std::mutex error_mutex_;
  std::exception_ptr last_error_;
  std::string name_;
  std::atomic<log_level> level_;
  std::unique_ptr<console_sink> console_sink_;
  std::unique_ptr<file_sink> file_sink_;
  std::mutex mutex_;
};

// ============================================================================
// Global logger registry
// ============================================================================

class logger_registry {
 public:
  static logger_registry& instance() {
    static logger_registry registry;
    return registry;
  }

  std::shared_ptr<logger> get_logger(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = loggers_.find(name);
    if (it != loggers_.end()) {
      return it->second;
    }

    auto new_logger = std::make_shared<logger>(name);
    loggers_[name] = new_logger;
    return new_logger;
  }

  void remove_logger(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    loggers_.erase(name);
  }

  void set_global_level(log_level level) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [name, logger] : loggers_) {
      logger->set_level(level);
    }
  }

 private:
  logger_registry() = default;

  std::unordered_map<std::string, std::shared_ptr<logger>> loggers_;
  std::mutex mutex_;
};

// ============================================================================
// Public API
// ============================================================================

// Get or create logger by name
export std::shared_ptr<logger> get_logger(const std::string& name = "default") {
  return logger_registry::instance().get_logger(name);
}

// Create new logger
export std::shared_ptr<logger> create_logger(const std::string& name) {
  return std::make_shared<logger>(name);
}

// Set global log level for all loggers
export void set_global_log_level(log_level level) {
  logger_registry::instance().set_global_level(level);
}

// ============================================================================
// Scoped log context (for timing operations)
// ============================================================================

export class log_scope {
 public:
  log_scope(std::shared_ptr<logger> logger, std::string scope_name)
      : logger_(std::move(logger)),
        scope_name_(std::move(scope_name)),
        start_(std::chrono::steady_clock::now()) {
    if (logger_) {
      logger_->debug("Entering scope: {}", scope_name_);
    }
  }

  ~log_scope() {
    if (logger_) {
      auto end = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
      try {
        logger_->debug("Exiting scope: {} (took {}ms)", scope_name_, duration.count());
      } catch (...) {
        logger_->record_error(std::current_exception());
      }
    }
  }

  log_scope(const log_scope&) = delete;
  log_scope& operator=(const log_scope&) = delete;

 private:
  std::shared_ptr<logger> logger_;
  std::string scope_name_;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace cppx
