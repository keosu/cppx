set_project("cppx_consumer")
set_version("9.8.7")
set_xmakever("3.1.1")
add_rules("mode.debug", "mode.release")

option("cppx_source")
    set_showmenu(true)
    set_description("Path to a cppx source checkout")
option_end()

option("cppx_version")
    set_showmenu(true)
    set_description("Exact source package version (omit for local binary packages)")
option_end()

option("cppx_packages")
    set_showmenu(true)
    set_description("Path to the repository produced by xmake package")
option_end()

if get_config("cppx_source") then
    includes(path.join(get_config("cppx_source"), "xmake.lua"))
elseif get_config("cppx_packages") then
    add_repositories("cppx-local .", {rootdir = get_config("cppx_packages")})
    local configs = {debug = is_mode("debug")}
    if get_config("toolchain") then configs.toolchains = get_config("toolchain") end
    if get_config("runtimes") then configs.runtimes = get_config("runtimes") end
    local version = get_config("cppx_version")
    add_requires("cppx" .. (version and (" " .. version) or ""), {system = false, configs = configs})
end

target("consumer")
    set_kind("binary")
    set_languages("c++23")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", "/Zc:preprocessor", {tools = "cl"})
    add_files("main.cpp", "worker.cpp")
    if get_config("cppx_source") then
        add_deps("cppx")
    else
        add_packages("cppx")
    end
    add_tests("default", {run_timeout = 60000})
    on_config(function (target)
        import("core.project.project")
        assert(project.name() == "cppx_consumer", "cppx overwrote the host project name")
        assert(project.version() == "9.8.7", "cppx overwrote the host project version")
        local expected_count = target:pkg("cppx") and 1 or 2
        local count = 0
        for _ in pairs(project.targets()) do count = count + 1 end
        assert(count == expected_count, "cppx introduced unexpected tests/examples/targets")
    end)
target_end()
