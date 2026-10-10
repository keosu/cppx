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
    add_files("src/*.cppm", {public = true})
    
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
                {nil, "output", "kv", nil, "Directory for isolated consumers and package"}
            }
        }
    task_end()
end

