-- ===========================================================================
-- Tests - 简化配置使用辅助函数
-- ===========================================================================

-- 获取核心库的模块文件列表（按测试需要的模块）
local function get_core_modules(test_module)
    local modules = {
        "../src/" .. test_module .. ".cppm",
        "../src/logging.cppm",
        "../src/string_utils.cppm",
        "../src/error.cppm"
    }
    return modules
end

-- Helper function to create test targets
function add_cppx_test(name, source_file, test_module, default_build)
    default_build = default_build or false
    
    target(name)
        set_kind("binary")
        set_default(default_build)
        set_group("tests")
        set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
        
        -- Add source file and required module files
        local files = {source_file}
        for _, mod in ipairs(get_core_modules(test_module)) do
            table.insert(files, mod)
        end
        add_files(table.unpack(files))
        
        -- Module support
        set_policy("build.c++.modules", true)
        add_cxxflags("/utf-8", {tools = "cl"})
        
        -- Configure test runner
        add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
    target_end()
end

-- Define all tests with their corresponding module
add_cppx_test("test_error", "test_error.cpp", "error")
add_cppx_test("test_string_utils", "test_string_utils.cpp", "string_utils")
add_cppx_test("test_file_system", "test_file_system.cpp", "file_system")
add_cppx_test("test_time_utils", "test_time_utils.cpp", "time_utils")
add_cppx_test("test_concurrent", "test_concurrent.cpp", "concurrent")
add_cppx_test("test_math_utils", "test_math_utils.cpp", "math_utils")
add_cppx_test("test_logging", "test_logging.cpp", "logging")
add_cppx_test("test_cli", "test_cli.cpp", "cli")
add_cppx_test("test_test", "test_test.cpp", "test")
