-- ===========================================================================
-- Examples - 简化配置使用辅助函数
-- ===========================================================================

-- 获取核心库的模块文件列表（不包括特定模块，按需添加）
local function get_core_modules(...)
    local modules = {}
    for _, mod in ipairs({...}) do
        table.insert(modules, "../src/" .. mod .. ".cppm")
    end
    -- 总是包含基础模块
    table.insert(modules, "../src/logging.cppm")
    table.insert(modules, "../src/string_utils.cppm")
    table.insert(modules, "../src/error.cppm")
    return modules
end

-- Helper function to create example targets
function add_example(name, source_file, extra_modules, default_build)
    extra_modules = extra_modules or {}
    default_build = default_build or true
    
    target(name)
        set_kind("binary")
        set_default(default_build)
        set_group("examples")
        set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/examples")
        
        -- Add source file and required module files
        local files = {source_file}
        for _, mod in ipairs(get_core_modules(table.unpack(extra_modules))) do
            table.insert(files, mod)
        end
        add_files(table.unpack(files))
        
        -- Module support
        set_policy("build.c++.modules", true)
        add_cxxflags("/utf-8", {tools = "cl"})
    target_end()
end

-- Define all examples with their specific module dependencies
add_example("error_demo", "error_demo.cpp", {})
add_example("string_utils_demo", "string_utils_demo.cpp", {})
add_example("file_system_demo", "file_system_demo.cpp", {"file_system"})
add_example("time_utils_demo", "time_utils_demo.cpp", {"time_utils"})
add_example("concurrent_demo", "concurrent_demo.cpp", {"concurrent"})
add_example("math_utils_demo", "math_utils_demo.cpp", {"math_utils"})
add_example("logging_demo", "logging_demo.cpp", {})
add_example("cli_demo", "cli_demo.cpp", {"cli"})
add_example("test_demo", "test_demo.cpp", {"test"})
