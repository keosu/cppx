-- ===========================================================================
-- Examples - 简化配置使用辅助函数
-- ===========================================================================
 
-- Helper function to create example targets
function add_example(name, source_file, default_build)
    extra_modules = extra_modules or {}
    default_build = default_build or true
    
    target(name)
        set_kind("binary")
        set_default(default_build)
        set_group("examples")
        set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/examples")
         
        add_files(source_file)
        add_deps("cppx")
        
        -- Module support
        set_policy("build.c++.modules", true)
        add_cxxflags("/utf-8", {tools = "cl"})
    target_end()
end

-- Define all examples with their specific module dependencies
add_example("error_demo", "error_demo.cpp")
add_example("string_utils_demo", "string_utils_demo.cpp")
add_example("file_system_demo", "file_system_demo.cpp")
add_example("time_utils_demo", "time_utils_demo.cpp")
add_example("concurrent_demo", "concurrent_demo.cpp")
add_example("math_utils_demo", "math_utils_demo.cpp")
add_example("logging_demo", "logging_demo.cpp")
add_example("cli_demo", "cli_demo.cpp")
add_example("test_demo", "test_demo.cpp")
