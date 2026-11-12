// cppx.file_system - File system utilities
// Provides file I/O, temporary file management, and path operations

module;
#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <Windows.h>
#endif

export module cppx.file_system;

import std;
import cppx.error;
#ifdef CPPX_FAULT_INJECTION
import cppx.fault_injection;
#endif

namespace cppx {

namespace fs = std::filesystem;

// ============================================================================
// File reading
// ============================================================================

// Read entire file as text
export expected<std::string, error_info> read_file_text(const fs::path& path) {
  try {
    std::ifstream file(path, std::ios::in);

    if (!file.is_open()) {
      return expected<std::string, error_info>::err(
          error_info("Failed to open file: " + path.string()));
    }

#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_read)) file.setstate(std::ios::badbit);
#endif
    std::string content;
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
      content.append(buffer, static_cast<size_t>(file.gcount()));
    if (file.bad() || !file.eof()) throw std::runtime_error("Failed to read complete file");

    return expected<std::string, error_info>::ok(std::move(content));
  } catch (const std::exception& e) {
    return expected<std::string, error_info>::err(
        error_info(std::string("Error reading file: ") + e.what()));
  }
}

// Read entire file as binary
export expected<std::vector<std::byte>, error_info> read_file_binary(const fs::path& path) {
  try {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
      return expected<std::vector<std::byte>, error_info>::err(
          error_info("Failed to open file: " + path.string()));
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0 || !file || !std::in_range<size_t>(size))
      throw std::runtime_error("Failed to determine file size");
    std::vector<std::byte> buffer(static_cast<size_t>(size));
#ifdef CPPX_FAULT_INJECTION
    // Force a real short read after the size has been obtained.
    if (testing::hit(testing::fault::file_read)) file.seekg(0, std::ios::end);
#endif
    if (size) file.read(reinterpret_cast<char*>(buffer.data()), size);
    if (!file || (size && file.gcount() != size))
      throw std::runtime_error("Short or failed binary read");

    return expected<std::vector<std::byte>, error_info>::ok(std::move(buffer));
  } catch (const std::exception& e) {
    return expected<std::vector<std::byte>, error_info>::err(
        error_info(std::string("Error reading file: ") + e.what()));
  }
}

// Read file line by line
export expected<std::vector<std::string>, error_info> read_lines(const fs::path& path) {
  try {
    std::ifstream file(path);

    if (!file.is_open()) {
      return expected<std::vector<std::string>, error_info>::err(
          error_info("Failed to open file: " + path.string()));
    }

#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_read)) file.setstate(std::ios::badbit);
#endif
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line)) {
      lines.push_back(std::move(line));
    }

    if (file.bad() || !file.eof()) throw std::runtime_error("Failed to read complete file");
    return expected<std::vector<std::string>, error_info>::ok(std::move(lines));
  } catch (const std::exception& e) {
    return expected<std::vector<std::string>, error_info>::err(
        error_info(std::string("Error reading file: ") + e.what()));
  }
}

// ============================================================================
// File writing
// ============================================================================

// Write text to file
export expected<void, error_info> write_file(const fs::path& path, std::string_view content,
                                             bool append = false) {
  try {
    auto mode = append ? (std::ios::out | std::ios::app) : std::ios::out;
    std::ofstream file(path, mode);

    if (!file.is_open()) {
      return expected<void, error_info>::err(
          error_info("Failed to open file for writing: " + path.string()));
    }

    if (!std::in_range<std::streamsize>(content.size()))
      throw std::length_error("File content too large");
    if (!content.empty()) file.write(content.data(), static_cast<std::streamsize>(content.size()));
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_write)) file.setstate(std::ios::badbit);
#endif
    file.flush();
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_flush)) file.setstate(std::ios::badbit);
#endif

    if (!file) {
      return expected<void, error_info>::err(
          error_info("Failed to write to file: " + path.string()));
    }

    file.close();
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_close)) file.setstate(std::ios::badbit);
#endif
    if (!file) throw std::runtime_error("Failed to close file after writing");
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(
        error_info(std::string("Error writing file: ") + e.what()));
  }
}

// Write binary data to file
export expected<void, error_info>
write_file_binary(const fs::path& path, const std::vector<std::byte>& data, bool append = false) {
  try {
    auto mode = append ? (std::ios::binary | std::ios::out | std::ios::app)
                       : (std::ios::binary | std::ios::out);
    std::ofstream file(path, mode);

    if (!file.is_open()) {
      return expected<void, error_info>::err(
          error_info("Failed to open file for writing: " + path.string()));
    }

    if (!std::in_range<std::streamsize>(data.size()))
      throw std::length_error("File content too large");
    if (!data.empty())
      file.write(reinterpret_cast<const char*>(data.data()),
                 static_cast<std::streamsize>(data.size()));
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_write)) file.setstate(std::ios::badbit);
#endif
    file.flush();
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_flush)) file.setstate(std::ios::badbit);
#endif

    if (!file) {
      return expected<void, error_info>::err(
          error_info("Failed to write to file: " + path.string()));
    }

    file.close();
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_close)) file.setstate(std::ios::badbit);
#endif
    if (!file) throw std::runtime_error("Failed to close file after writing");
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(
        error_info(std::string("Error writing file: ") + e.what()));
  }
}

// Write lines to file
export expected<void, error_info>
write_lines(const fs::path& path, const std::vector<std::string>& lines, bool append = false) {
  try {
    auto mode = append ? (std::ios::out | std::ios::app) : std::ios::out;
    std::ofstream file(path, mode);

    if (!file.is_open()) {
      return expected<void, error_info>::err(
          error_info("Failed to open file for writing: " + path.string()));
    }

    for (const auto& line : lines) {
      file << line << '\n';
    }
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_write)) file.setstate(std::ios::badbit);
#endif
    file.flush();
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_flush)) file.setstate(std::ios::badbit);
#endif

    if (!file) {
      return expected<void, error_info>::err(
          error_info("Failed to write to file: " + path.string()));
    }

    file.close();
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_close)) file.setstate(std::ios::badbit);
#endif
    if (!file) throw std::runtime_error("Failed to close file after writing");
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(
        error_info(std::string("Error writing file: ") + e.what()));
  }
}

// ============================================================================
// Path operations
// ============================================================================

// Join path components
export fs::path path_join(const fs::path& base, const fs::path& component) {
  return base / component;
}

// Join multiple path components
export template <typename... Args>
fs::path path_join(const fs::path& base, const fs::path& component, Args&&... args) {
  return path_join(base / component, std::forward<Args>(args)...);
}

// Ensure directory exists (create if needed)
export expected<void, error_info> ensure_dir(const fs::path& path) {
  try {
    fs::create_directories(path);
    if (!fs::is_directory(path))
      return expected<void, error_info>::err(
          error_info("Path is not a directory: " + path.string()));
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(
        error_info(std::string("Error creating directory: ") + e.what()));
  }
}

// Check if file exists
export bool file_exists(const fs::path& path) {
  try {
    return fs::exists(path) && fs::is_regular_file(path);
  } catch (...) {
    return false;
  }
}

// Check if directory exists
export bool dir_exists(const fs::path& path) {
  try {
    return fs::exists(path) && fs::is_directory(path);
  } catch (...) {
    return false;
  }
}

// Get file size
export expected<size_t, error_info> file_size(const fs::path& path) {
  try {
    if (!fs::exists(path)) {
      return expected<size_t, error_info>::err(error_info("File does not exist: " + path.string()));
    }
    return expected<size_t, error_info>::ok(fs::file_size(path));
  } catch (const std::exception& e) {
    return expected<size_t, error_info>::err(
        error_info(std::string("Error getting file size: ") + e.what()));
  }
}

// Copy file
export expected<void, error_info> copy_file(const fs::path& source, const fs::path& dest,
                                            bool overwrite = false) {
  try {
    auto options = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none;

    fs::copy_file(source, dest, options);
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(
        error_info(std::string("Error copying file: ") + e.what()));
  }
}

// Move/rename file
export expected<void, error_info> move_file(const fs::path& source, const fs::path& dest) {
  try {
    fs::rename(source, dest);
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(
        error_info(std::string("Error moving file: ") + e.what()));
  }
}

// Delete file
export expected<void, error_info> remove_file(const fs::path& path) {
  try {
    if (!fs::remove(path)) {
      return expected<void, error_info>::err(error_info("Failed to remove file: " + path.string()));
    }
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(
        error_info(std::string("Error removing file: ") + e.what()));
  }
}

// Delete directory recursively
export expected<void, error_info> remove_dir(const fs::path& path) {
  try {
    auto count = fs::remove_all(path);
    if (count == 0 && fs::exists(path)) {
      return expected<void, error_info>::err(
          error_info("Failed to remove directory: " + path.string()));
    }
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(
        error_info(std::string("Error removing directory: ") + e.what()));
  }
}

// List directory contents
export expected<std::vector<fs::path>, error_info> list_dir(const fs::path& path) {
  try {
    if (!fs::is_directory(path)) {
      return expected<std::vector<fs::path>, error_info>::err(
          error_info("Not a directory: " + path.string()));
    }

    std::vector<fs::path> entries;
    for (const auto& entry : fs::directory_iterator(path)) {
      entries.push_back(entry.path());
    }

    return expected<std::vector<fs::path>, error_info>::ok(std::move(entries));
  } catch (const std::exception& e) {
    return expected<std::vector<fs::path>, error_info>::err(
        error_info(std::string("Error listing directory: ") + e.what()));
  }
}

// ============================================================================
// Temporary file/directory management
// ============================================================================

// Candidate names are only hints: exclusive creation establishes ownership.
inline std::string generate_random_suffix() {
  thread_local std::mt19937_64 engine(std::random_device{}());
  return std::format("{:016x}{:016x}", engine(), engine());
}

namespace file_detail {
inline void validate_prefix(std::string_view prefix) {
  if (prefix.empty() || prefix.find_first_of("/\\:") != std::string_view::npos ||
      prefix.find('\0') != std::string_view::npos || prefix == "." || prefix == "..")
    throw std::invalid_argument("Temporary prefix must be a filename component");
}
inline fs::path create_temporary(std::string_view prefix, const fs::path& parent, bool directory) {
  validate_prefix(prefix);
  auto absolute_parent = fs::absolute(parent);
  for (int attempt = 0; attempt < 128; ++attempt) {
    auto candidate = absolute_parent / (std::string(prefix) + "_" + generate_random_suffix());
    std::error_code ec;
    if (directory) {
      if (fs::create_directory(candidate, ec)) return candidate;
      if (!ec || ec == std::errc::file_exists) continue;
      throw fs::filesystem_error("Cannot create temporary directory", candidate, ec);
    }
    std::ofstream file(candidate, std::ios::out | std::ios::binary | std::ios::noreplace);
    if (file.is_open()) {
      file.close();
      if (file) return candidate;
      fs::remove(candidate, ec);
      throw std::runtime_error("Cannot close temporary file");
    }
    if (fs::exists(fs::symlink_status(candidate, ec))) continue;
    throw fs::filesystem_error("Cannot create temporary file", candidate,
                               std::make_error_code(std::errc::io_error));
  }
  throw std::runtime_error("Temporary name collision retry limit exceeded");
}
}  // namespace file_detail

export class temp_file {
 public:
  explicit temp_file(std::string_view prefix = "tmp",
                     const fs::path& parent = fs::temp_directory_path())
      : path_(file_detail::create_temporary(prefix, parent, false)) {}
  ~temp_file() { cleanup(); }
  temp_file(const temp_file&) = delete;
  temp_file& operator=(const temp_file&) = delete;
  temp_file(temp_file&& other) noexcept
      : path_(std::move(other.path_)), owned_(std::exchange(other.owned_, false)) {
    other.path_.clear();
  }
  temp_file& operator=(temp_file&& other) noexcept {
    if (this != &other) {
      cleanup();
      path_ = std::move(other.path_);
      owned_ = std::exchange(other.owned_, false);
      other.path_.clear();
    }
    return *this;
  }
  [[nodiscard]] const fs::path& path() const noexcept { return path_; }
  void keep() noexcept { owned_ = false; }

 private:
  void cleanup() noexcept {
    if (owned_) {
      try {
        std::error_code ec;
        fs::remove(path_, ec);
      } catch (...) {
      }
      owned_ = false;
    }
  }
  fs::path path_;
  bool owned_ = true;
};

export class temp_dir {
 public:
  explicit temp_dir(std::string_view prefix = "tmpdir",
                    const fs::path& parent = fs::temp_directory_path())
      : path_(file_detail::create_temporary(prefix, parent, true)) {}
  ~temp_dir() { cleanup(); }
  temp_dir(const temp_dir&) = delete;
  temp_dir& operator=(const temp_dir&) = delete;
  temp_dir(temp_dir&& other) noexcept
      : path_(std::move(other.path_)), owned_(std::exchange(other.owned_, false)) {
    other.path_.clear();
  }
  temp_dir& operator=(temp_dir&& other) noexcept {
    if (this != &other) {
      cleanup();
      path_ = std::move(other.path_);
      owned_ = std::exchange(other.owned_, false);
      other.path_.clear();
    }
    return *this;
  }
  [[nodiscard]] const fs::path& path() const noexcept { return path_; }
  void keep() noexcept { owned_ = false; }

 private:
  void cleanup() noexcept {
    if (owned_) {
      try {
        std::error_code ec;
        fs::remove_all(path_, ec);
      } catch (...) {
      }
      owned_ = false;
    }
  }
  fs::path path_;
  bool owned_ = true;
};

// Replace using a sibling temporary file. Atomic visibility depends on the filesystem;
// stream flush does not promise power-loss durability.
export expected<void, error_info> write_file_atomic(const fs::path& path,
                                                    std::string_view content) {
  try {
    auto parent = path.has_parent_path() ? path.parent_path() : fs::current_path();
    temp_file staging("cppx_write", parent);
    std::ofstream file(staging.path(), std::ios::binary | std::ios::trunc);
    if (!std::in_range<std::streamsize>(content.size()))
      throw std::length_error("File content too large");
    if (!content.empty()) file.write(content.data(), static_cast<std::streamsize>(content.size()));
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_write)) file.setstate(std::ios::badbit);
#endif
    file.flush();
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_flush)) file.setstate(std::ios::badbit);
#endif
    if (!file) throw std::runtime_error("Failed to write atomic staging file");
    file.close();
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::file_close)) file.setstate(std::ios::badbit);
#endif
    if (!file) throw std::runtime_error("Failed to close atomic staging file");
#ifdef CPPX_FAULT_INJECTION
    if (testing::hit(testing::fault::replace))
      throw std::system_error(std::make_error_code(std::errc::permission_denied));
#endif
#ifdef _WIN32
    if (!MoveFileExW(staging.path().c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
      throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                              "Atomic replace failed");
#else
    fs::rename(staging.path(), path);
#endif
    staging.keep();
    return expected<void, error_info>::ok();
  } catch (const std::exception& e) {
    return expected<void, error_info>::err(error_info(e.what(), "write_file_atomic"));
  }
}

}  // namespace cppx
