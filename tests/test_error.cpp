// Test for cppx.error module

import std;
import cppx.error;
import cppx.logging;

#define assert(x) if (!(x)) { std::cerr << "Assertion failed: " #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; std::abort(); }

using namespace cppx;

void test_expected_ok() {
    auto result = expected<int, std::string>::ok(42);
    assert(result.is_ok());
    assert(!result.is_err());
    assert(result.value() == 42);
auto logger = get_logger("test_error");
    logger->info("✓ expected ok test passed");
}

void test_expected_err() {
    auto result = expected<int, std::string>::err("error message");
    assert(!result.is_ok());
    assert(result.is_err());
    assert(result.error() == "error message");
auto logger = get_logger("test_error");
    logger->info("✓ expected err test passed");
}

void test_expected_map() {
    auto result = expected<int, std::string>::ok(10);
    auto mapped = result.map([](int x) { return x * 2; });
    assert(mapped.is_ok());
    assert(mapped.value() == 20);
    auto logger = get_logger("test_error");
    logger->info("✓ expected map test passed");
}

void test_expected_and_then() {
    auto result = expected<int, std::string>::ok(10);
    auto chained = result.and_then([](int x) {
        return expected<int, std::string>::ok(x * 2);
    });
    assert(chained.is_ok());
    assert(chained.value() == 20);
    auto logger = get_logger("test_error");
    logger->info("✓ expected and_then test passed");
}

void test_error_info() {
    error_info info("Test error");
    assert(info.message() == "Test error");
    
    info.add_context("function_name");
    assert(info.to_string() == "function_name: Test error");
auto logger = get_logger("test_error");
    logger->info("✓ error_info test passed");
}

void test_try_catch() {
    auto result = try_catch([]() {
        return 42;
    });
    assert(result.is_ok());
    assert(result.value() == 42);
    
    auto err_result = try_catch([]() -> int {
        throw std::runtime_error("test error");
    });
    assert(err_result.is_err());
    auto logger = get_logger("test_error");
    logger->info("✓ try_catch test passed");
}

int main() {
    auto logger = get_logger("test_error");
    logger->info("=== Error Module Test Suite ===\n");
    
    test_expected_ok();
    test_expected_err();
    test_expected_map();
    test_expected_and_then();
    test_error_info();
    test_try_catch();
    
    logger->info("\n✓ All error tests passed!");
    return 0;
}

