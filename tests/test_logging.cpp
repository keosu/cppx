// Test for cppx.logging module

import std;
import cppx.logging;

#define assert(x) if (!(x)) { std::cerr << "Assertion failed: " #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; std::abort(); }

using namespace cppx;

void test_logger_creation() {
    auto logger = get_logger("test");
    assert(logger.get() != nullptr);
    logger->info("✓ logger creation test passed");
}

void test_log_levels() {
    auto logger = create_logger("test_levels");
    logger->set_level(log_level::warn);
    assert(logger->get_level() == log_level::warn);
    
    // These should not crash
    logger->debug("Debug message (should not appear)");
    logger->info("Info message (should not appear)");
    logger->warn("Warning message");
    logger->error("Error message");
    
    logger->info("✓ log levels test passed");
}

void test_log_formatting() {
    auto logger = create_logger("test_format");
    logger->set_level(log_level::debug);
    
    logger->info("Simple message");
    logger->info("Formatted: {} {}", 42, "test");
    
    logger->info("✓ log formatting test passed");
}

void test_log_scope() {
    auto logger = create_logger("test_scope");
    
    {
        log_scope scope(logger, "test_operation");
        // Do some work...
    }
    
    logger->info("✓ log scope test passed");
}

int main() {
    auto logger = get_logger("test_logging");
    logger->info("=== Logging Test Suite ===\n");
    
    test_logger_creation();
    test_log_levels();
    test_log_formatting();
    test_log_scope();
    
    logger->info("\n✓ All logging tests passed!");
    return 0;
}

