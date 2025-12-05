// Test for cppx.file_system module

import std;
import cppx.file_system;
import cppx.logging;

#define assert(x) if (!(x)) { std::cerr << "Assertion failed: " #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; std::abort(); }

using namespace cppx;

void test_temp_file() {
    auto temp = temp_file();
    auto path = temp.path();
    
    // Write to temp file
    auto write_result = write_file(path, "test content");
    assert(write_result.is_ok());
    
    // Read from temp file
    auto read_result = read_file_text(path);
    assert(read_result.is_ok());
    assert(read_result.value() == "test content");
    
auto logger = get_logger("test_file_system");
    logger->info("✓ temp_file test passed");
}

void test_file_operations() {
    auto temp = temp_file();
    auto path = temp.path();
    
    // Write lines
    std::vector<std::string> lines = {"line1", "line2", "line3"};
    auto write_result = write_lines(path, lines);
    assert(write_result.is_ok());
    
    // Read lines
    auto read_result = read_lines(path);
    assert(read_result.is_ok());
    assert(read_result.value().size() == 3);
    assert(read_result.value()[0] == "line1");
    
    auto logger = get_logger("test_file_system");
    logger->info("✓ file operations test passed");
}

void test_path_operations() {
    auto joined = path_join("/home", "user", "file.txt");
    assert(joined.string().find("user") != std::string::npos);
auto logger = get_logger("test_file_system");
    logger->info("✓ path operations test passed");
}

void test_temp_dir() {
    auto temp = temp_dir();
    auto path = temp.path();
    
    assert(dir_exists(path));
    
    auto file_path = path_join(path, "test.txt");
    auto write_result = write_file(file_path, "hello");
    assert(write_result.is_ok());
    assert(file_exists(file_path));
    
auto logger = get_logger("test_file_system");
    logger->info("✓ temp_dir test passed");
}

int main() {
    auto logger = get_logger("test_file_system");
    logger->info("=== File System Test Suite ===\n");
    
    test_temp_file();
    test_file_operations();
    test_path_operations();
    test_temp_dir();
    
    logger->info("\n✓ All file_system tests passed!");
    return 0;
}

