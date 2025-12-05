// cppx.file_system module demo

import std;
import cppx.file_system;

import cppx.logging;

using namespace cppx;

int main() {
  auto logger = get_logger("file_system_demo");
  logger->info("=== cppx.file_system Demo ===\n");

  // Example 1: Temporary files
  logger->info("1. Working with temporary files:");
  {
    auto temp = temp_file("demo");
    logger->info("   Created temp file: {}", temp.path().string());

    // Write to temp file
    auto write_result = write_file(temp.path(), "Hello, temporary world!");
    if (write_result.is_ok()) {
      logger->info("   Written to temp file");
    }

    // Read from temp file
    auto read_result = read_file_text(temp.path());
    if (read_result.is_ok()) {
      logger->info("   Read from temp file: {}", read_result.value());
    }

    logger->info("   Temp file will be deleted when going out of scope");
  }
  logger->info("\n");

  // Example 2: Working with lines
  logger->info("2. Writing and reading lines:");
  {
    auto temp = temp_file("lines_demo");
    std::vector<std::string> lines = {"Line 1: First line", "Line 2: Second line",
                                      "Line 3: Third line"};

    auto write_result = write_lines(temp.path(), lines);
    if (write_result.is_ok()) {
      logger->info("   Written {} lines", lines.size());
    }

    auto read_result = read_lines(temp.path());
    if (read_result.is_ok()) {
      logger->info("   Read {} lines:", read_result.value().size());
      for (const auto& line : read_result.value()) {
        logger->info("      {}", line);
      }
    }
  }
  logger->info("\n");

  // Example 3: Path operations
  logger->info("3. Path operations:");
  auto home = std::filesystem::path("/home/user");
  auto docs = path_join(home, "documents");
  auto file = path_join(docs, "report.txt");
  logger->info("   Base: {}", home.string());
  logger->info("   Documents: {}", docs.string());
  logger->info("   File: {}", file.string());

  // Example 4: Temporary directory
  logger->info("4. Working with temporary directory:");
  {
    temp_dir tmp_directory("demo_dir");
    logger->info("   Created temp dir: {}", tmp_directory.path().string());

    // Create files in temp directory
    auto file1 = path_join(tmp_directory.path(), "file1.txt");
    auto file2 = path_join(tmp_directory.path(), "file2.txt");

    write_file(file1, "Content 1");
    write_file(file2, "Content 2");

    logger->info("   Created 2 files in temp directory");

    auto entries = list_dir(tmp_directory.path());
    if (entries.is_ok()) {
      logger->info("   Directory contains {} entries", entries.value().size());
    }

    logger->info("   Temp directory will be deleted when going out of scope");
  }
  logger->info("\n");

  // Example 5: File existence checks
  logger->info("5. File existence checks:");
  {
    auto temp = temp_file();
    logger->info("   file_exists: {}", (file_exists(temp.path()) ? "yes" : "no"));
    logger->info("   dir_exists: {}", (dir_exists(temp.path()) ? "yes" : "no"));
  }

  logger->info("\n=== Demo completed ===");
  return 0;
}
