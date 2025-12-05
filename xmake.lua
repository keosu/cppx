-- cppx: Modern c++23 Module-based Utility Library
-- Project configuration

set_project("cppx")
set_version("0.1.0")
set_xmakever("2.8.0")

-- Set c++23 standard with modules support
set_languages("c++23", "cxx23")

-- Add build modes
add_rules("mode.debug", "mode.release")

-- MSVC-only configuration for c++23 modules with import std

if is_mode("debug") then
    set_optimize("none")
    set_symbols("debug")
else
    set_optimize("fastest")
end

-- Main library target
target("cppx")
    set_kind("static")
    
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
    
    -- Install headers
    add_installfiles("src/*.cppm", {prefixdir = "include/cppx"})
    add_installfiles("src/*.h", {prefixdir = "include/cppx"})
    
    -- MSVC flags for c++23 modules and import std
    add_cxxflags("/utf-8", {tools = "cl"})
    add_syslinks("ws2_32", "user32")

    -- Enable warnings
    set_warnings("all")

target_end()

-- Optional: shared library version
target("cppx_shared")
    set_kind("shared")
    add_files("src/*.cppm", {public = true})
    set_policy("build.c++.modules", true)
    add_includedirs("src", {public = true})
    
    -- MSVC flags for c++23 modules and import std
    add_cxxflags("/utf-8", {tools = "cl"})
    add_syslinks("ws2_32", "user32")
    
    set_warnings("all")
    set_enabled(false) -- Disabled by default, enable with: xmake f --cppx_shared=y
target_end()

-- ===========================================================================
-- Tests and Examples (from subdirectories)
-- ===========================================================================

includes("tests")
includes("examples")

