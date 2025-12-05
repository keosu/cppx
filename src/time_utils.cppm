// cppx.time_utils - Time formatting, parsing, and utilities
// Provides ISO8601, RFC3339 formatting and scope timers

export module cppx.time_utils;

import std;
import cppx.error;

namespace cppx {

using std::chrono::duration_cast;
using std::chrono::hours;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::minutes;
using std::chrono::nanoseconds;
using std::chrono::seconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;

// ============================================================================
// Time formatting
// ============================================================================

// Format time point as ISO8601 string (YYYY-MM-DDTHH:MM:SS.sss)
export std::string format_iso8601(const system_clock::time_point& time_point) {
  auto time = system_clock::to_time_t(time_point);
  auto ms = duration_cast<milliseconds>(time_point.time_since_epoch()) % 1000;

  std::tm* tm_ptr = std::gmtime(&time);
  if (!tm_ptr) {
    return "";
  }

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", tm_ptr);

  return std::format("{}.{:03d}Z", buffer, ms.count());
}

// Format time point as RFC3339 string
export std::string format_rfc3339(const system_clock::time_point& time_point) {
  return format_iso8601(time_point);
}

// Format time point with custom format string
export std::string format_time(const system_clock::time_point& time_point,
                               std::string_view format = "%Y-%m-%d %H:%M:%S") {
  auto time = system_clock::to_time_t(time_point);

  std::tm* tm_ptr = std::localtime(&time);
  if (!tm_ptr) {
    return "";
  }

  char buffer[128];
  std::strftime(buffer, sizeof(buffer), format.data(), tm_ptr);

  return std::string(buffer);
}

// Get current time formatted
export std::string now_iso8601() { return format_iso8601(system_clock::now()); }

export std::string now_rfc3339() { return format_rfc3339(system_clock::now()); }

export std::string now_formatted(std::string_view format = "%Y-%m-%d %H:%M:%S") {
  return format_time(system_clock::now(), format);
}

// ============================================================================
// Time parsing
// ============================================================================

// Parse ISO8601 time string
export expected<system_clock::time_point, error_info> parse_iso8601(std::string_view str) {
  std::tm tm = {};
  std::string temp_str{str};
  std::istringstream ss{temp_str};

  // Try to parse YYYY-MM-DDTHH:MM:SS format
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

  if (ss.fail()) {
    return expected<system_clock::time_point, error_info>::err(
        error_info("Failed to parse ISO8601 time string"));
  }

  // Convert to time_point
  auto time = std::mktime(&tm);
  if (time == -1) {
    return expected<system_clock::time_point, error_info>::err(error_info("Invalid time value"));
  }

  return expected<system_clock::time_point, error_info>::ok(system_clock::from_time_t(time));
}

// Parse custom time format
export expected<system_clock::time_point, error_info>
parse_time(std::string_view str, std::string_view format = "%Y-%m-%d %H:%M:%S") {
  std::tm tm = {};
  std::string temp_str{str};
  std::istringstream ss{temp_str};

  ss >> std::get_time(&tm, format.data());

  if (ss.fail()) {
    return expected<system_clock::time_point, error_info>::err(
        error_info("Failed to parse time string"));
  }

  auto time = std::mktime(&tm);
  if (time == -1) {
    return expected<system_clock::time_point, error_info>::err(error_info("Invalid time value"));
  }

  return expected<system_clock::time_point, error_info>::ok(system_clock::from_time_t(time));
}

// ============================================================================
// Duration formatting
// ============================================================================

// Format duration in human-readable form
export std::string format_duration(milliseconds duration) {
  auto ms = duration.count();

  if (ms < 1000) {
    return std::format("{}ms", ms);
  }

  auto secs = ms / 1000;
  ms %= 1000;

  if (secs < 60) {
    return std::format("{}.{:03d}s", secs, ms);
  }

  auto mins = secs / 60;
  secs %= 60;

  if (mins < 60) {
    return std::format("{}m {}s", mins, secs);
  }

  auto hrs = mins / 60;
  mins %= 60;

  return std::format("{}h {}m {}s", hrs, mins, secs);
}

export template <typename Rep, typename Period>
std::string format_duration(std::chrono::duration<Rep, Period> duration) {
  return format_duration(duration_cast<milliseconds>(duration));
}

// ============================================================================
// Scope timer (RAII timing)
// ============================================================================

export class scope_timer {
 public:
  using callback_type = std::function<void(milliseconds)>;

  explicit scope_timer(std::string name, callback_type callback = nullptr)
      : name_(std::move(name)), callback_(std::move(callback)), start_(steady_clock::now()) {}

  ~scope_timer() {
    auto end = steady_clock::now();
    auto duration = duration_cast<milliseconds>(end - start_);

    if (callback_) {
      callback_(duration);
    }
  }

  // Get elapsed time without destroying the timer
  [[nodiscard]] milliseconds elapsed() const {
    auto now = steady_clock::now();
    return duration_cast<milliseconds>(now - start_);
  }

  // Reset the timer
  void reset() { start_ = steady_clock::now(); }

  [[nodiscard]] const std::string& name() const { return name_; }

 private:
  std::string name_;
  callback_type callback_;
  steady_clock::time_point start_;
};

// Helper to create scope timer with lambda
export auto time_scope(std::string name, auto callback) {
  return scope_timer(std::move(name), [callback = std::move(callback)](milliseconds duration) {
    callback(duration);
  });
}

// ============================================================================
// Timer class (for scheduled tasks)
// ============================================================================

export class timer {
 public:
  timer() : running_(false) {}

  ~timer() { stop(); }

  // Start periodic timer
  void start_periodic(milliseconds interval, std::function<void()> callback) {
    stop();

    running_ = true;
    thread_ = std::thread([this, interval, callback = std::move(callback)]() {
      while (running_) {
        auto start = steady_clock::now();

        callback();

        auto end = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(end - start);
        auto sleep_time = interval - elapsed;

        if (sleep_time.count() > 0 && running_) {
          std::this_thread::sleep_for(sleep_time);
        }
      }
    });
  }

  // Start one-shot timer
  void start_once(milliseconds delay, std::function<void()> callback) {
    stop();

    running_ = true;
    thread_ = std::thread([this, delay, callback = std::move(callback)]() {
      std::this_thread::sleep_for(delay);
      if (running_) {
        callback();
      }
    });
  }

  // Stop timer
  void stop() {
    if (running_) {
      running_ = false;
      if (thread_.joinable()) {
        thread_.join();
      }
    }
  }

  [[nodiscard]] bool is_running() const { return running_; }

 private:
  bool running_;
  std::thread thread_;
};

// ============================================================================
// Sleep utilities
// ============================================================================

// Sleep for duration
export template <typename Rep, typename Period>
void sleep_for(std::chrono::duration<Rep, Period> duration) {
  std::this_thread::sleep_for(duration);
}

// Sleep until time point
export void sleep_until(const system_clock::time_point& time_point) {
  std::this_thread::sleep_until(time_point);
}

// Sleep for milliseconds (convenience)
export void sleep_ms(long long milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Sleep for seconds (convenience)
export void sleep_sec(long long seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

// ============================================================================
// Stopwatch class
// ============================================================================

export class stopwatch {
 public:
  stopwatch() : running_(false) {}

  void start() {
    start_ = steady_clock::now();
    running_ = true;
  }

  void stop() {
    if (running_) {
      end_ = steady_clock::now();
      running_ = false;
    }
  }

  void reset() {
    start_ = steady_clock::time_point{};
    end_ = steady_clock::time_point{};
    running_ = false;
  }

  void restart() {
    start_ = steady_clock::now();
    running_ = true;
  }

  [[nodiscard]] milliseconds elapsed_ms() const {
    if (running_) {
      return duration_cast<milliseconds>(steady_clock::now() - start_);
    } else {
      return duration_cast<milliseconds>(end_ - start_);
    }
  }

  [[nodiscard]] microseconds elapsed_us() const {
    if (running_) {
      return duration_cast<microseconds>(steady_clock::now() - start_);
    } else {
      return duration_cast<microseconds>(end_ - start_);
    }
  }

  [[nodiscard]] nanoseconds elapsed_ns() const {
    if (running_) {
      return duration_cast<nanoseconds>(steady_clock::now() - start_);
    } else {
      return duration_cast<nanoseconds>(end_ - start_);
    }
  }

  [[nodiscard]] bool is_running() const { return running_; }

 private:
  steady_clock::time_point start_;
  steady_clock::time_point end_;
  bool running_;
};

// ============================================================================
// Timestamp utilities
// ============================================================================

// Get current Unix timestamp (seconds since epoch)
export long long timestamp_sec() {
  return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

// Get current Unix timestamp (milliseconds since epoch)
export long long timestamp_ms() {
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Get current Unix timestamp (microseconds since epoch)
export long long timestamp_us() {
  return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

// Convert timestamp to time_point
export system_clock::time_point from_timestamp_sec(long long timestamp) {
  return system_clock::time_point(seconds(timestamp));
}

export system_clock::time_point from_timestamp_ms(long long timestamp) {
  return system_clock::time_point(milliseconds(timestamp));
}

export system_clock::time_point from_timestamp_us(long long timestamp) {
  return system_clock::time_point(microseconds(timestamp));
}

}  // namespace cppx
