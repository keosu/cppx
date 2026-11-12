function quality_target(name, source)
    target(name)
        set_kind("binary")
        set_languages("c++23")
        set_group("quality")
        set_targetdir("$(builddir)/quality")
        add_files(source)
        add_deps("cppx")
        add_rules("cppx.instrument")
        add_cxxflags("/utf-8", "/Zc:preprocessor", {tools = "cl"})
        set_policy("build.c++.modules", true)
    target_end()
end
if has_config("cppx_build_quality") then
    quality_target("fuzz_inputs", "fuzz_inputs.cpp")
    quality_target("json_roundtrip", "json_roundtrip.cpp")
    quality_target("stress", "stress.cpp")
    target("stress")
        if is_plat("windows") then add_syslinks("psapi") end
    target_end()
    quality_target("sanitizer_probe", "sanitizer_probe.cpp")
end
if has_config("cppx_build_benchmarks") then
    quality_target("benchmark", "benchmark.cpp")
    target("benchmark")
        if is_plat("windows") then add_syslinks("psapi") end
    target_end()
end
