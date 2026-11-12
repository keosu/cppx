-- cppx: Modern c++23 Module-based Utility Library
-- Project configuration

set_xmakever("3.1.1")

local cppx_main_project = os.scriptdir() == os.projectdir()
if cppx_main_project then
    set_project("cppx")
    set_version("0.1.0")
    add_rules("mode.debug", "mode.release")
end

option("cppx_build_tests")
    set_default(cppx_main_project)
    set_showmenu(true)
    set_description("Build cppx tests (off when embedded in another project)")
option_end()

option("cppx_build_examples")
    set_default(cppx_main_project)
    set_showmenu(true)
    set_description("Build cppx examples (off when embedded in another project)")
option_end()

option("cppx_components")
    set_default("all")
    set_showmenu(true)
    set_description("Comma-separated modules; dependencies are included automatically")
option_end()

option("cppx_build_quality")
    set_default(false)
    set_showmenu(true)
    set_description("Build deterministic fuzz and concurrency stress tools")
option_end()

option("cppx_build_benchmarks")
    set_default(false)
    set_showmenu(true)
    set_description("Build performance measurement tools")
option_end()

option("cppx_fault_injection")
    set_default(false)
    set_showmenu(true)
    set_description("Test-only failure injection; installation and packaging are forbidden")
option_end()

option("cppx_sanitizer")
    set_default("none")
    set_values("none", "address", "undefined", "thread")
    set_showmenu(true)
option_end()

rule("cppx.instrument")
    on_load(function (target)
        import("core.project.config")
        local sanitizer = config.get("cppx_sanitizer")
        if sanitizer and sanitizer ~= "none" then
            assert(sanitizer == "address" or not target:is_plat("windows"),
                   "MSVC supports address sanitizer only")
            target:set("policy", "build.sanitizer." .. sanitizer, true)
        end
    end)
rule_end()

local dependencies = {
    error = {}, serialization = {}, string_utils = {"error"},
    time_utils = {"error"}, file_system = {"error"}, concurrent = {"error"},
    math_utils = {"error"}, cli = {"error", "string_utils"},
    logging = {"error", "string_utils", "time_utils"}, test = {"string_utils", "logging"}
}
local selected = {}
local configuration_error
local function select_component(name)
    if not dependencies[name] then
        configuration_error = "Unknown cppx component: " .. name
        return
    end
    if selected[name] then return end
    selected[name] = true
    for _, dependency in ipairs(dependencies[name]) do select_component(dependency) end
end
local components = get_config("cppx_components") or "all"
if components == "all" then
    for name in pairs(dependencies) do select_component(name) end
else
    for _, name in ipairs(components:split(",")) do select_component(name:trim()) end
    if has_config("cppx_build_tests") or has_config("cppx_build_examples") or
       has_config("cppx_build_quality") or has_config("cppx_build_benchmarks") then
        configuration_error = "Partial components require tests, examples, quality tools and benchmarks disabled"
    end
end

-- Main library target
target("cppx")
    set_kind("static")
    set_version("0.1.0")
    set_license("MIT")
    set_languages("c++23")
    set_policy("build.fence", true)
    
    -- Enable conforming preprocessor for better macro expansion (VS2019 16.5+)
    add_cxxflags("/Zc:preprocessor", {tools = "cl"})
    add_cxxflags("/utf-8", {tools = "cl"})
    
    -- Add module files as public so dependent targets can use them
    for name in pairs(selected) do add_files("src/" .. name .. ".cppm", {public = true}) end
    add_rules("cppx.instrument")
    if has_config("cppx_fault_injection") then
        if not cppx_main_project or not has_config("cppx_build_tests") then
            configuration_error = "Fault injection is only available in a standalone test build"
        end
        add_defines("CPPX_FAULT_INJECTION", {public = true})
        add_files("tests/support/fault_injection.cppm", {public = true})
        before_install(function () raise("Cannot install a fault-injection build") end)
    end
    on_load(function () assert(not configuration_error, configuration_error) end)
    
    -- Set module directory
    set_policy("build.c++.modules", true)
    
    -- Export include and module directories for dependent targets
    add_includedirs("src", {public = true})
    
    -- Add header files for macros
    add_headerfiles("src/*.h", {public = true})
    
    -- xmake installs public modules with the metadata needed by add_packages.
    add_installfiles("LICENSE", {prefixdir = "share/licenses/cppx"})
    
    -- Windows system libraries
    if is_plat("windows") then
        add_syslinks("ws2_32", "user32")
    end

    -- Enable warnings
    set_warnings("all")

    before_package(function (target)
        import("core.base.option")
        import("core.project.config")
        assert(not config.get("cppx_fault_injection"), "Cannot package a fault-injection build")
        if option.get("format") ~= "local" then return end
        -- xmake 3.1.1's local packager bypasses rule install hooks and does not
        -- pass its destination to installfiles(). Prepare both explicitly.
        target:set("installdir", path.join(target:packagedir(), target:plat(), target:arch(), config.mode()))
        for _, rule in ipairs(target:orderules()) do
            local before_install = rule:script("install_before")
            if before_install then before_install(target) end
        end
    end)

target_end()

-- ===========================================================================
-- Tests and Examples (from subdirectories)
-- ===========================================================================

if has_config("cppx_build_tests") then
    includes("tests")
end
if has_config("cppx_build_examples") then
    includes("examples")
end
if has_config("cppx_build_quality") or has_config("cppx_build_benchmarks") then
    includes("quality")
end

-- ============================================================================
-- Custom tasks
-- ============================================================================

-- Format task: format all source files using clang-format
-- Implementation in scripts/format.lua
if cppx_main_project then
    task("format")
        set_category("plugin")
        on_run("scripts.format")

        set_menu {
            usage = "xmake format [options]",
            description = "Format C++ source files using clang-format",
            options = {
                {'c', "check", "k", nil, "Check formatting without modifying files"}
            }
        }
    task_end()

    task("check-toolchain")
        set_category("plugin")
        on_run("scripts.check_toolchain")
        set_menu {
            usage = "xmake check-toolchain [options]",
            description = "Compile and run isolated C++23 and module capability probes",
            options = {
                {nil, "toolchain", "kv", nil, "xmake toolchain name"},
                {nil, "sdk", "kv", nil, "Toolchain SDK path"},
                {nil, "runtimes", "kv", nil, "Runtime/standard library selection"},
                {nil, "cc", "kv", nil, "Explicit C compiler"},
                {nil, "cxx", "kv", nil, "Explicit C++ compiler"},
                {nil, "mode", "kv", "release", "Build mode: debug or release"},
                {nil, "output", "kv", nil, "Directory for projects, logs and report.json"},
                {nil, "require", "kv", "all", "Required capabilities: all, cpp23 or none"}
            }
        }
    task_end()

    task("verify-consumer")
        set_category("plugin")
        on_run("scripts.verify_consumer")
        set_menu {
            usage = "xmake verify-consumer [options]",
            description = "Verify isolated xmake source and local-package consumers",
            options = {
                {nil, "mode", "kv", "release", "Build mode: debug or release"},
                {nil, "toolchain", "kv", nil, "xmake toolchain name"},
                {nil, "sdk", "kv", nil, "Toolchain SDK path"},
                {nil, "runtimes", "kv", nil, "Runtime/standard library selection"},
                {nil, "cc", "kv", nil, "Explicit C compiler"},
                {nil, "cxx", "kv", nil, "Explicit C++ compiler"},
                {nil, "output", "kv", nil, "Directory for isolated consumers and package"}
            }
        }
    task_end()
end

