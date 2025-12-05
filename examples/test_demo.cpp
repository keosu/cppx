// Demo of cppx.test - A modern C++20 module-based testing framework

import std;
import cppx.test;
import cppx.logging;

#include "../src/test_macros.h"

using namespace cppx;

// Simple test case
TEST_CASE("Basic assertions", "[basic]") {
    REQUIRE(1 + 1 == 2);
    REQUIRE(true);
    CHECK(2 * 2 == 4);
}

// Test case with sections
TEST_CASE("Vector operations", "[vector]") {
    std::vector<int> v;
    
    REQUIRE(v.empty());
    REQUIRE(v.size() == 0);
    
    SECTION("Adding elements") {
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        
        REQUIRE(v.size() == 3);
        CHECK(v[0] == 1);
        CHECK(v[1] == 2);
        CHECK(v[2] == 3);
    }
    
    SECTION("Clearing vector") {
        v.push_back(1);
        v.push_back(2);
        
        REQUIRE(v.size() == 2);
        
        v.clear();
        
        REQUIRE(v.empty());
        REQUIRE(v.size() == 0);
    }
}

// Test with comparison operators
TEST_CASE("Comparison operators", "[operators]") {
    int a = 5;
    int b = 10;
    
    REQUIRE_LT(a, b);
    REQUIRE_LE(a, b);
    REQUIRE_GT(b, a);
    REQUIRE_GE(b, a);
    REQUIRE_NE(a, b);
    
    int c = 5;
    REQUIRE_EQ(a, c);
}

// Test with exceptions
TEST_CASE("Exception handling", "[exceptions]") {
    REQUIRE_THROWS(throw std::runtime_error("error"));
    
    REQUIRE_NOTHROW([]() {
        int x = 42;
        (void)x;
    }());
}

// Test with strings
TEST_CASE("String operations", "[string]") {
    std::string s = "hello";
    
    REQUIRE(s.length() == 5);
    REQUIRE(s == "hello");
    
    SECTION("Appending") {
        s += " world";
        
        REQUIRE(s == "hello world");
        REQUIRE(s.length() == 11);
    }
    
    SECTION("Substring") {
        auto sub = s.substr(0, 4);
        
        REQUIRE(sub == "hell");
        REQUIRE(s == "hello");  // Original unchanged
    }
}

// Test with containers
TEST_CASE("Map operations", "[map]") {
    std::map<std::string, int> m;
    
    REQUIRE(m.empty());
    
    m["one"] = 1;
    m["two"] = 2;
    m["three"] = 3;
    
    REQUIRE(m.size() == 3);
    CHECK_EQ(m["one"], 1);
    CHECK_EQ(m["two"], 2);
    CHECK_EQ(m["three"], 3);
    
    REQUIRE(m.contains("one"));
    REQUIRE(!m.contains("four"));
}

// Test with algorithms
TEST_CASE("Algorithm tests", "[algorithm]") {
    std::vector<int> v = {5, 2, 8, 1, 9};
    
    SECTION("Sorting") {
        std::sort(v.begin(), v.end());
        
        REQUIRE(v[0] == 1);
        REQUIRE(v[4] == 9);
        REQUIRE(std::is_sorted(v.begin(), v.end()));
    }
    
    SECTION("Finding") {
        auto it = std::find(v.begin(), v.end(), 8);
        
        REQUIRE(it != v.end());
        REQUIRE(*it == 8);
        
        auto it2 = std::find(v.begin(), v.end(), 100);
        REQUIRE(it2 == v.end());
    }
}

// Test with floating point
TEST_CASE("Floating point comparisons", "[float]") {
    double a = 0.1 + 0.2;
    double b = 0.3;
    
    // Note: Direct comparison might fail due to floating point precision
    // In a real test framework, you'd use approximate comparison
    CHECK(std::abs(a - b) < 0.0001);
}

// Test that demonstrates failure (commented out by default)
// TEST_CASE("This test will fail") {
//     REQUIRE(1 + 1 == 3);  // This will fail
//     CHECK(false);         // This will also fail
// }

int main() {
    auto logger = get_logger("test_demo");
    
    logger->info("=== cppx.test Framework Demo ===\n");
    
    // Create test runner
    test_runner runner;
    
    // Run all tests
    auto results = runner.run_all();
    
    logger->info("\n=== Demo completed ===");
    logger->info("Tests passed: {}", runner.passed_count());
    logger->info("Tests failed: {}", runner.failed_count());
    
    return runner.failed_count() == 0 ? 0 : 1;
}

