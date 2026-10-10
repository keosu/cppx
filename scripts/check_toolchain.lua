function main()
    import("core.base.option")
    import("check_support", {rootdir = os.scriptdir(), alias = "checks"})

    local mode = option.get("mode")
    local required = option.get("require")
    assert(mode == "debug" or mode == "release", "mode must be debug or release")
    assert(required == "all" or required == "cpp23" or required == "none",
           "require must be all, cpp23 or none")

    local directory, reportfile = checks.workspace(option.get("output"), "toolchain-check")
    local report = {
        schema_version = 1, host = os.host(), arch = os.arch(), mode = mode,
        toolchain = option.get("toolchain") or "auto", sdk = option.get("sdk"),
        runtimes = option.get("runtimes"), required = required, capabilities = {}
    }
    report.xmake = checks.run_step(directory, "xmake-version", {"--version"})
    local passed = true
    for _, feature in ipairs({"cpp23", "named_module", "import_std"}) do
        local projectdir = path.join(directory, feature)
        checks.copy_project(path.join(os.projectdir(), "tests", "toolchain"), projectdir)
        local arguments = checks.configuration(projectdir, mode)
        table.insert(arguments, "--feature=" .. feature)
        for _, name in ipairs({"toolchain", "sdk", "runtimes"}) do
            if option.get(name) then
                table.insert(arguments, "--" .. name .. "=" .. option.get(name))
            end
        end
        local result = checks.verify_project(projectdir, arguments)
        report.capabilities[feature] = result
        print("%s: %s", feature, result.passed and "passed" or "failed (see logs)")
        if not result.passed and (required == "all" or required == feature) then
            passed = false
        end
    end
    report.passed = passed
    checks.report(reportfile, report)
    if not passed then
        raise("Required toolchain capabilities failed; inspect " .. reportfile)
    end
end
