// Test for cppx.test module - testing the testing framework itself!

import std;
import cppx.test;
import cppx.logging;

#include "../src/test_macros.h"

using namespace cppx;

// Test the test registry
TEST_CASE("Test registry", "[registry]") {
  // The registry should contain this test
  const auto& tests = test_registry::instance().get_tests();

  REQUIRE(!tests.empty());

  // Check that we can find tests by name
  bool found = false;
  for (const auto& test : tests) {
    if (test.name.find("Test registry") != std::string::npos) {
      found = true;
      break;
    }
  }

  REQUIRE(found);
}

// Test assertions
TEST_CASE("Assertion functionality", "[assertions]") {
  // These should all pass
  REQUIRE(true);
  REQUIRE(1 == 1);
  REQUIRE(2 + 2 == 4);

  CHECK(true);
  CHECK_EQ(1, 1);
  CHECK_NE(1, 2);
  CHECK_LT(1, 2);
  CHECK_LE(1, 2);
  CHECK_GT(2, 1);
  CHECK_GE(2, 1);
}

// Test exception assertions
TEST_CASE("Exception assertions", "[exceptions]") {
  // Test that we can detect exceptions
  REQUIRE_THROWS(throw std::runtime_error("test error"));

  // Test that we can detect no exceptions
  REQUIRE_NOTHROW(([]() {
    int x = 42;
    int y = x * 2;
    (void)y;
  })());

  // Test with different exception types
  REQUIRE_THROWS(throw std::logic_error("logic error"));
  REQUIRE_THROWS(throw std::invalid_argument("invalid arg"));
}

// Test sections
TEST_CASE("Section functionality", "[sections]") {
  int value = 0;

  REQUIRE(value == 0);

  SECTION("First section") {
    value = 1;
    REQUIRE(value == 1);

    SECTION("Nested section") {
      value = 2;
      REQUIRE(value == 2);
    }
  }

  SECTION("Second section") {
    value = 10;
    REQUIRE(value == 10);
  }
}

// Test with various data types
TEST_CASE("Data type assertions", "[types]") {
  SECTION("Integers") {
    int a = 42;
    REQUIRE_EQ(a, 42);
    REQUIRE_NE(a, 0);
  }

  SECTION("Floats") {
    double d = 3.14159;
    CHECK(d > 3.0);
    CHECK(d < 4.0);
  }

  SECTION("Strings") {
    std::string s = "test";
    REQUIRE(s == "test");
    REQUIRE(s.length() == 4);
  }

  SECTION("Booleans") {
    bool b = true;
    REQUIRE(b);
    REQUIRE(b == true);
  }
}

// Test container assertions
TEST_CASE("Container assertions", "[containers]") {
  std::vector<int> v = {1, 2, 3, 4, 5};

  REQUIRE(v.size() == 5);
  REQUIRE(!v.empty());
  REQUIRE_EQ(v.front(), 1);
  REQUIRE_EQ(v.back(), 5);

  SECTION("Element access") {
    REQUIRE_EQ(v[0], 1);
    REQUIRE_EQ(v[2], 3);
    REQUIRE_EQ(v[4], 5);
  }

  SECTION("Iteration") {
    int sum = 0;
    for (int x : v) {
      sum += x;
    }
    REQUIRE_EQ(sum, 15);
  }
}

// Test string operations
TEST_CASE("String assertions", "[strings]") {
  std::string s1 = "hello";
  std::string s2 = "world";

  REQUIRE(s1 != s2);
  REQUIRE(s1.length() == 5);
  REQUIRE(s2.length() == 5);

  SECTION("Concatenation") {
    std::string combined = s1 + " " + s2;
    REQUIRE(combined == "hello world");
    REQUIRE(combined.length() == 11);
  }

  SECTION("Substring") {
    std::string sub = s1.substr(0, 4);
    REQUIRE(sub == "hell");
  }

  SECTION("Find") {
    REQUIRE(s1.find('e') == 1);
    REQUIRE(s1.find('z') == std::string::npos);
  }
}

// Test pointer assertions
TEST_CASE("Pointer assertions", "[pointers]") {
  int* ptr = nullptr;
  REQUIRE(ptr == nullptr);

  int value = 42;
  ptr = &value;

  REQUIRE(ptr != nullptr);
  REQUIRE(*ptr == 42);

  *ptr = 100;
  REQUIRE(value == 100);
}

// Test smart pointers
TEST_CASE("Smart pointer assertions", "[smart_pointers]") {
  auto ptr = std::make_unique<int>(42);

  REQUIRE(ptr != nullptr);
  REQUIRE(*ptr == 42);

  auto shared = std::make_shared<std::string>("test");
  REQUIRE(shared != nullptr);
  REQUIRE(*shared == "test");
  REQUIRE(shared.use_count() == 1);
}

// Test algorithm results
TEST_CASE("Algorithm assertions", "[algorithms]") {
  std::vector<int> v = {5, 2, 8, 1, 9, 3};

  SECTION("Min/Max") {
    auto min_it = std::min_element(v.begin(), v.end());
    auto max_it = std::max_element(v.begin(), v.end());

    REQUIRE(*min_it == 1);
    REQUIRE(*max_it == 9);
  }

  SECTION("Count") {
    v.push_back(2);
    auto count = std::count(v.begin(), v.end(), 2);
    REQUIRE(count == 2);
  }

  SECTION("Sort") {
    std::sort(v.begin(), v.end());
    REQUIRE(std::is_sorted(v.begin(), v.end()));
    REQUIRE(v[0] == 1);
    REQUIRE(v[v.size() - 1] == 9);
  }
}

int main() {
  auto logger = get_logger("test_test");
  logger->info("=== Testing the Test Framework ===\n");

  test_runner runner;
  auto results = runner.run_all();

  logger->info("\n=== All tests completed ===");

  // Verify results
  if (runner.failed_count() == 0) {
    logger->info("✓ Test framework is working correctly!");
    return 0;
  } else {
    logger->error("✗ Test framework has issues");
    return 1;
  }
}
