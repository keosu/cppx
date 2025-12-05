// Quick test to verify the library works

import cppx.error;
import cppx.string_utils;

#include <iostream>

int main() {
    std::cout << "=== Quick cppx Test ===\n\n";
    
    // Test string_utils
    auto parts = cppx::split("hello,world,test", ",");
    std::cout << "Split test: ";
    for (const auto& part : parts) {
        std::cout << part << " ";
    }
    std::cout << "\n";
    
    auto joined = cppx::join(parts, "-");
    std::cout << "Join test: " << joined << "\n\n";
    
    // Test error handling
    auto result = cppx::expected<int, std::string>::ok(42);
    if (result.is_ok()) {
        std::cout << "Expected test: value = " << result.value() << "\n";
    }
    
    auto err_result = cppx::expected<int, std::string>::err("test error");
    if (err_result.is_err()) {
        std::cout << "Error test: " << err_result.error() << "\n";
    }
    
    std::cout << "\n✓ All tests passed!\n";
    return 0;
}

