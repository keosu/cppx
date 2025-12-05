-- ===========================================================================
-- Tests
-- ===========================================================================

target("test_error")
    set_kind("binary")
    set_default(true)
    set_group("tests")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
    add_files("test_error.cpp", "../src/error.cppm", "../src/logging.cppm", "../src/string_utils.cppm")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", {tools = "cl"})
    
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
target_end()

target("test_string_utils")
    set_kind("binary")
    set_default(true)
    set_group("tests")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
    add_files("test_string_utils.cpp", "../src/string_utils.cppm", "../src/error.cppm", "../src/logging.cppm")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", {tools = "cl"})
    
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
target_end()

target("test_file_system")
    set_kind("binary")
    set_default(true)
    set_group("tests")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
    add_files("test_file_system.cpp", "../src/file_system.cppm", "../src/error.cppm", "../src/logging.cppm", "../src/string_utils.cppm")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", {tools = "cl"})
    
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
target_end()

target("test_logging")
    set_kind("binary")
    set_default(true)
    set_group("tests")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
    add_files("test_logging.cpp", "../src/logging.cppm", "../src/string_utils.cppm", "../src/error.cppm")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", {tools = "cl"})
    
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
target_end()

target("test_time_utils")
    set_kind("binary")
    set_default(true)
    set_group("tests")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
    add_files("test_time_utils.cpp", "../src/time_utils.cppm", "../src/error.cppm", "../src/logging.cppm", "../src/string_utils.cppm")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", {tools = "cl"})
    
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
target_end()

target("test_concurrent")
    set_kind("binary")
    set_default(true)
    set_group("tests")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
    add_files("test_concurrent.cpp", "../src/concurrent.cppm", "../src/error.cppm", "../src/logging.cppm", "../src/string_utils.cppm")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", {tools = "cl"})
    
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*", timeout = 10000})
target_end()

target("test_math_utils")
    set_kind("binary")
    set_default(true)
    set_group("tests")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
    add_files("test_math_utils.cpp", "../src/math_utils.cppm", "../src/error.cppm", "../src/logging.cppm", "../src/string_utils.cppm")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", {tools = "cl"})
    
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
target_end()

target("test_cli")
    set_kind("binary")
    set_default(true)
    set_group("tests")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/tests")
    add_files("test_cli.cpp", "../src/cli.cppm", "../src/string_utils.cppm", "../src/error.cppm", "../src/logging.cppm")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", {tools = "cl"})
    
    add_tests("default", {runargs = {}, pass_outputs = ".*passed.*"})
target_end()
