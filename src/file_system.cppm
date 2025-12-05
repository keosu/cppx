// cppx.file_system - File system utilities
// Provides file I/O, temporary file management, and path operations

export module cppx.file_system;

import std;
import cppx.error;

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

    std::string content;
    file.seekg(0, std::ios::end);
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

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

    std::vector<std::byte> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

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

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line)) {
      lines.push_back(std::move(line));
    }

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

    file.write(content.data(), content.size());

    if (!file) {
      return expected<void, error_info>::err(
          error_info("Failed to write to file: " + path.string()));
    }

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

    file.write(reinterpret_cast<const char*>(data.data()), data.size());

    if (!file) {
      return expected<void, error_info>::err(
          error_info("Failed to write to file: " + path.string()));
    }

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

    if (!file) {
      return expected<void, error_info>::err(
          error_info("Failed to write to file: " + path.string()));
    }

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
    if (!fs::exists(path)) {
      if (!fs::create_directories(path)) {
        return expected<void, error_info>::err(
            error_info("Failed to create directory: " + path.string()));
      }
    } else if (!fs::is_directory(path)) {
      return expected<void, error_info>::err(
          error_info("Path exists but is not a directory: " + path.string()));
    }
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

// Generate random suffix for temp files
inline std::string generate_random_suffix() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);

  const char* hex = "0123456789abcdef";
  std::string suffix;
  suffix.reserve(8);

  for (int i = 0; i < 8; ++i) {
    suffix += hex[dis(gen)];
  }

  return suffix;
}

// RAII temporary file
export class temp_file {
 public:
  temp_file(std::string_view prefix = "tmp")
      : path_(fs::temp_directory_path() / (std::string(prefix) + "_" + generate_random_suffix())) {
    // Create empty file
    std::ofstream file(path_);
  }

  ~temp_file() {
    try {
      if (fs::exists(path_)) {
        fs::remove(path_);
      }
    } catch (...) {
      // Ignore errors in destructor
    }
  }

  // Non-copyable
  temp_file(const temp_file&) = delete;
  temp_file& operator=(const temp_file&) = delete;

  // Movable
  temp_file(temp_file&& other) noexcept : path_(std::move(other.path_)) { other.path_.clear(); }

  temp_file& operator=(temp_file&& other) noexcept {
    if (this != &other) {
      cleanup();
      path_ = std::move(other.path_);
      other.path_.clear();
    }
    return *this;
  }

  [[nodiscard]] const fs::path& path() const noexcept { return path_; }

  void keep() { path_.clear(); }

 private:
  void cleanup() {
    try {
      if (!path_.empty() && fs::exists(path_)) {
        fs::remove(path_);
      }
    } catch (...) {
      // Ignore errors
    }
  }

  fs::path path_;
};

// RAII temporary directory
export class temp_dir {
 public:
  temp_dir(std::string_view prefix = "tmpdir")
      : path_(fs::temp_directory_path() / (std::string(prefix) + "_" + generate_random_suffix())) {
    fs::create_directory(path_);
  }

  ~temp_dir() {
    try {
      if (fs::exists(path_)) {
        fs::remove_all(path_);
      }
    } catch (...) {
      // Ignore errors in destructor
    }
  }

  // Non-copyable
  temp_dir(const temp_dir&) = delete;
  temp_dir& operator=(const temp_dir&) = delete;

  // Movable
  temp_dir(temp_dir&& other) noexcept : path_(std::move(other.path_)) { other.path_.clear(); }

  temp_dir& operator=(temp_dir&& other) noexcept {
    if (this != &other) {
      cleanup();
      path_ = std::move(other.path_);
      other.path_.clear();
    }
    return *this;
  }

  [[nodiscard]] const fs::path& path() const noexcept { return path_; }

  void keep() { path_.clear(); }

 private:
  void cleanup() {
    try {
      if (!path_.empty() && fs::exists(path_)) {
        fs::remove_all(path_);
      }
    } catch (...) {
      // Ignore errors
    }
  }

  fs::path path_;
};

}  // namespace cppx
