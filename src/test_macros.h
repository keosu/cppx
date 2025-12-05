#pragma once

// Test macros for cppx.test framework
// These macros must be in a header file, not in a module

#define CPPX_CONCAT_IMPL(a, b) a##b
#define CPPX_CONCAT(a, b) CPPX_CONCAT_IMPL(a, b)
#define CPPX_UNIQUE_NAME(base) CPPX_CONCAT(base, __LINE__)

// TEST_CASE macro - defines a test case
#define TEST_CASE(name, ...) \
    static void CPPX_UNIQUE_NAME(test_func_)(); \
    namespace { \
        struct CPPX_UNIQUE_NAME(test_registrar_type_) { \
            CPPX_UNIQUE_NAME(test_registrar_type_)() { \
                ::cppx::register_test_case(name, #__VA_ARGS__, \
                    CPPX_UNIQUE_NAME(test_func_), __FILE__, __LINE__); \
            } \
        }; \
        static CPPX_UNIQUE_NAME(test_registrar_type_) CPPX_UNIQUE_NAME(test_registrar_); \
    } \
    static void CPPX_UNIQUE_NAME(test_func_)()

// SECTION macro - defines a section within a test
#define SECTION(name) \
    if (::cppx::section_guard sg_##__LINE__(name, [&]() { \
        /* Section body will be here */ \
    }); sg_##__LINE__)

// Assertion macros
#define REQUIRE(expr) \
    ::cppx::record_assertion((expr), #expr, __FILE__, __LINE__)

#define REQUIRE_MSG(expr, msg) \
    ::cppx::record_assertion((expr), #expr, __FILE__, __LINE__, msg)

#define CHECK(expr) \
    ::cppx::record_check((expr), #expr, __FILE__, __LINE__)

#define CHECK_MSG(expr, msg) \
    ::cppx::record_check((expr), #expr, __FILE__, __LINE__, msg)

#define REQUIRE_THROWS(expr) \
    do { \
        bool threw = false; \
        try { \
            (expr); \
        } catch (...) { \
            threw = true; \
        } \
        ::cppx::record_assertion(threw, #expr " throws", __FILE__, __LINE__); \
    } while(0)

#define REQUIRE_NOTHROW(expr) \
    do { \
        bool threw = false; \
        try { \
            (expr); \
        } catch (...) { \
            threw = true; \
        } \
        ::cppx::record_assertion(!threw, #expr " doesn't throw", __FILE__, __LINE__); \
    } while(0)

// Comparison macros
#define REQUIRE_EQ(a, b) REQUIRE((a) == (b))
#define REQUIRE_NE(a, b) REQUIRE((a) != (b))
#define REQUIRE_LT(a, b) REQUIRE((a) < (b))
#define REQUIRE_LE(a, b) REQUIRE((a) <= (b))
#define REQUIRE_GT(a, b) REQUIRE((a) > (b))
#define REQUIRE_GE(a, b) REQUIRE((a) >= (b))

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_LT(a, b) CHECK((a) < (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))

