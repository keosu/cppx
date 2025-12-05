-- Format task: format all source files using clang-format
-- Usage:
--   xmake format          - Format all files
--   xmake format --check  - Check formatting without modifying files

function main()
    import("core.base.option")
    import("lib.detect.find_tool")
    
    -- Find clang-format
    local clang_format = find_tool("clang-format")
    if not clang_format then
        raise("clang-format not found! Please install LLVM/clang-format first.\n" ..
              "  Windows: choco install llvm  or  scoop install llvm\n" ..
              "  macOS:   brew install llvm\n" ..
              "  Linux:   sudo apt-get install clang-format")
    end
    
    print("Found clang-format: %s", clang_format.program)
    
    -- Check mode or format mode
    local check_mode = option.get("check")
    
    -- Find all source files
    local sourcefiles = {}
    for _, dir in ipairs({"src", "tests", "examples"}) do
        local files = os.files(path.join(dir, "**.cpp"))
        table.join2(sourcefiles, files)
        files = os.files(path.join(dir, "**.cppm"))
        table.join2(sourcefiles, files)
        files = os.files(path.join(dir, "**.h"))
        table.join2(sourcefiles, files)
    end
    
    if #sourcefiles == 0 then
        print("No source files found")
        return
    end
    
    print("Found %d file(s) to %s", #sourcefiles, check_mode and "check" or "format")
    print("")
    
    local needs_format = 0
    local formatted = 0
    
    for _, file in ipairs(sourcefiles) do
        local relpath = path.relative(file, os.projectdir())
        
        if check_mode then
            -- Check mode: dry-run to see if formatting is needed
            local outdata, errdata = os.iorunv(clang_format.program, {"--dry-run", "--Werror", file})
            if errdata and #errdata > 0 then
                cprint("  ${red}✗ %s - needs formatting", relpath)
                needs_format = needs_format + 1
            else
                cprint("  ${green}✓ %s", relpath)
            end
        else
            -- Format mode: apply formatting
            cprint("  ${dim}→ %s", relpath)
            os.execv(clang_format.program, {"-i", file})
            formatted = formatted + 1
        end
    end
    
    print("")
    print("=== Summary ===")
    
    if check_mode then
        if needs_format > 0 then
            cprint("${red}❌ %d file(s) need formatting", needs_format)
            print("   Run: xmake format  (without --check)")
            raise("Code formatting check failed")
        else
            cprint("${green}✓ All files are properly formatted!")
        end
    else
        cprint("${green}✓ Formatted %d file(s)", formatted)
        print("")
        print("💡 Tip: Use 'xmake format --check' to verify formatting")
    end
end

