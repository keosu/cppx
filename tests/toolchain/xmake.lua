set_project("cppx_toolchain_probe")
set_xmakever("3.1.1")
set_languages("c++23")
add_rules("mode.debug", "mode.release")

option("feature")
    set_default("cpp23")
    set_values("cpp23", "named_module", "import_std")
option_end()

target("probe")
    set_kind("binary")
    set_targetdir("$(builddir)/bin")
    add_cxxflags("/utf-8", "/Zc:preprocessor", {tools = "cl"})
    if has_config("feature") and get_config("feature") == "named_module" then
        set_policy("build.c++.modules", true)
        -- This probe must not depend on standard-library module availability.
        set_policy("build.c++.modules.std", false)
        add_files("answer.cppm", "named_module.cpp")
    elseif get_config("feature") == "import_std" then
        set_policy("build.c++.modules", true)
        add_files("import_std.cpp")
    else
        add_files("cpp23.cpp")
    end
target_end()
