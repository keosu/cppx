-- ===========================================================================
-- Examples - 通过依赖核心库，不再需要手动添加模块文件
-- ===========================================================================

-- Helper function to create example targets
function add_example(name, source_file, default_build)
    default_build = default_build or true
    
    target(name)
        set_kind("binary")
        set_default(default_build)
        set_group("examples")
        set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/examples")
        
        -- Add source file
        add_files(source_file)
        
        -- 依赖核心库（应该自动继承模块和链接）
        add_deps("cppx")
        
        -- Module support
        set_policy("build.c++.modules", true)
        add_cxxflags("/utf-8", {tools = "cl"})
    target_end()
end

-- Define all examples - 现在只需要指定源文件！
add_example("error_demo", "error_demo.cpp")
add_example("string_utils_demo", "string_utils_demo.cpp")
add_example("file_system_demo", "file_system_demo.cpp")
add_example("time_utils_demo", "time_utils_demo.cpp")
add_example("concurrent_demo", "concurrent_demo.cpp")
add_example("math_utils_demo", "math_utils_demo.cpp")
add_example("logging_demo", "logging_demo.cpp")
add_example("cli_demo", "cli_demo.cpp")
add_example("test_demo", "test_demo.cpp")
