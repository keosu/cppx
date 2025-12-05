# cppx.test - Modern C++20 Module-Based Testing Framework

A Catch2-inspired testing framework built with C++20 modules and `import std`.

## Features

- ✅ **Modern C++20**: Full module support with `import std`
- ✅ **Catch2-style syntax**: Familiar TEST_CASE, SECTION, REQUIRE/CHECK macros
- ✅ **Rich assertions**: Comparison operators, exception testing
- ✅ **Detailed reporting**: Colorful output with pass/fail statistics
- ✅ **Test organization**: Tag-based filtering and section support
- ✅ **Integrated logging**: Uses cppx.logging for consistent output

## Quick Start

### Basic Usage

```cpp
import std;
import cppx.test;
import cppx.logging;

#include "../src/test_macros.h"

using namespace cppx;

TEST_CASE("My first test", "[basic]") {
    REQUIRE(1 + 1 == 2);
    CHECK(true);
}

int main() {
    test_runner runner;
    auto results = runner.run_all();
    return runner.failed_count() == 0 ? 0 : 1;
}
```

### Assertions

#### Basic Assertions
```cpp
REQUIRE(condition);     // Stops test on failure
CHECK(condition);       // Continues test on failure
REQUIRE_MSG(condition, "Custom message");
CHECK_MSG(condition, "Custom message");
```

#### Comparison Assertions
```cpp
REQUIRE_EQ(a, b);      // a == b
REQUIRE_NE(a, b);      // a != b
REQUIRE_LT(a, b);      // a < b
REQUIRE_LE(a, b);      // a <= b
REQUIRE_GT(a, b);      // a > b
REQUIRE_GE(a, b);      // a >= b
```

#### Exception Testing
```cpp
REQUIRE_THROWS(expression);        // Must throw
REQUIRE_NOTHROW(expression);       // Must not throw
```

### Test Organization

#### Test Cases with Tags
```cpp
TEST_CASE("Vector operations", "[vector][container]") {
    std::vector<int> v = {1, 2, 3};
    REQUIRE(v.size() == 3);
}
```

#### Sections
```cpp
TEST_CASE("Vector operations", "[vector]") {
    std::vector<int> v;
    
    REQUIRE(v.empty());
    
    SECTION("Adding elements") {
        v.push_back(1);
        REQUIRE(v.size() == 1);
    }
    
    SECTION("Clearing") {
        v.push_back(1);
        v.clear();
        REQUIRE(v.empty());
    }
}
```

### Running Tests

#### Run All Tests
```cpp
test_runner runner;
auto results = runner.run_all();
```

#### Filter Tests by Name or Tag
```cpp
test_runner runner;
auto results = runner.run_filtered("vector");  // Matches name or tags
```

#### Check Results
```cpp
test_runner runner;
runner.run_all();

std::cout << "Passed: " << runner.passed_count() << std::endl;
std::cout << "Failed: " << runner.failed_count() << std::endl;

return runner.failed_count() == 0 ? 0 : 1;
```

## Example Output

```
===============================================================================
Running 3 test case(s)
===============================================================================

-------------------------------------------------------------------------------
Basic assertions
-------------------------------------------------------------------------------
test.cpp:12
...............................................................................

✓ Test passed

-------------------------------------------------------------------------------
Vector operations
-------------------------------------------------------------------------------
test.cpp:19
...............................................................................

✓ Test passed

===============================================================================
Test Summary
===============================================================================
Total: 3
Passed: 3 (100%)
===============================================================================
✓ All tests passed!
```

## Building with xmake

### Add to your test target

```lua
target("my_test")
    set_kind("binary")
    add_files("my_test.cpp", "../src/test.cppm", "../src/logging.cppm", ...)
    set_policy("build.c++.modules", true)
    add_cxxflags("/std:c++latest", "/utf-8", {tools = "cl"})
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
target_end()
```

## Module Structure

- **`cppx.test`** - Core test framework module (src/test.cppm)
- **`test_macros.h`** - Macro definitions (required due to MSVC module limitations)

## Important Notes

### Module + Macro Hybrid Approach

Due to current MSVC limitations with macros in modules, test macros are defined in `test_macros.h`. You must include this header after importing the module:

```cpp
import cppx.test;
#include "../src/test_macros.h"  // Required for TEST_CASE, REQUIRE, etc.
```

### Difference Between REQUIRE and CHECK

- **`REQUIRE`**: Stops the current test on failure
- **`CHECK`**: Continues running the test even on failure

Use `REQUIRE` for critical assertions and `CHECK` for validations where you want to see all failures.

## Examples

See the complete examples in:
- **`examples/test_demo.cpp`** - Comprehensive demonstration of all features
- **`tests/test_test.cpp`** - The framework testing itself (meta!)

## Comparison with Catch2

### Similar Features
- TEST_CASE, SECTION macros
- REQUIRE/CHECK assertions
- Exception testing
- Test organization and filtering
- Detailed output

### Differences
- ✅ Pure C++20 modules
- ✅ Integrated with cppx.logging
- ✅ Simpler implementation (educational)
- ❌ No BDD-style SCENARIO/GIVEN/WHEN/THEN (yet)
- ❌ No matchers (yet)
- ❌ No generators (yet)

## Future Enhancements

Potential future features:
- BDD-style macros (SCENARIO, GIVEN, WHEN, THEN)
- Matchers for more expressive assertions
- Test fixtures
- Parameterized tests
- Benchmarking support
- XML/JSON output for CI integration

## License

Part of the cppx utility library. See main LICENSE file.

