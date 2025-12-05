-- ===========================================================================
-- Tests - 简化配置使用辅助函数
-- ===========================================================================

-- Helper function to create test targets
function add_cppx_test(name, source_file, default_build)
    default_build = default_build or true
    
    target(name)
        set_kind("binary")
        set_default(default_build)
        set_group("tests")
        set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
        
        add_files(source_file)
        add_deps("cppx")
        
        -- Module support
        set_policy("build.c++.modules", true)
        add_cxxflags("/utf-8", {tools = "cl"})
        
        -- Configure test runner
        add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
    target_end()
end

-- Define all tests with their corresponding module
add_cppx_test("test_error", "test_error.cpp")
add_cppx_test("test_string_utils", "test_string_utils.cpp")
add_cppx_test("test_file_system", "test_file_system.cpp")
add_cppx_test("test_time_utils", "test_time_utils.cpp")
add_cppx_test("test_concurrent", "test_concurrent.cpp")
add_cppx_test("test_math_utils", "test_math_utils.cpp")
add_cppx_test("test_logging", "test_logging.cpp")
add_cppx_test("test_cli", "test_cli.cpp")
add_cppx_test("test_test", "test_test.cpp")
add_cppx_test("test_serialization", "test_serialization_simple.cpp")
