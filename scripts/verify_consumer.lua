function main()
    import("core.base.option")
    import("check_support", {rootdir = os.scriptdir(), alias = "checks"})

    local mode = option.get("mode")
    assert(mode == "debug" or mode == "release", "mode must be debug or release")
    local directory, reportfile = checks.workspace(option.get("output"), "consumer-check")
    local report = {schema_version = 1, host = os.host(), arch = os.arch(), mode = mode}
    local root = os.projectdir()

    -- Copy only distributable inputs. This build cannot use the checkout's build cache.
    local library = path.join(directory, "library")
    os.mkdir(library)
    os.cp(path.join(root, "xmake.lua"), library)
    os.cp(path.join(root, "LICENSE"), library)
    os.cp(path.join(root, "src"), library)

    local source_consumer = path.join(directory, "source-consumer")
    checks.copy_project(path.join(root, "tests", "consumer"), source_consumer)
    local source_config = checks.configuration(source_consumer, mode)
    table.insert(source_config, "--cppx_source=" .. library)
    report.source = checks.verify_project(source_consumer, source_config)
    print("Source consumer: %s", report.source.passed and "passed" or "failed (see logs)")

    local package_output = path.join(directory, "local-repository")
    local library_config = checks.configuration(library, mode)
    table.insert(library_config, "--cppx_build_tests=n")
    table.insert(library_config, "--cppx_build_examples=n")
    local package_steps = {checks.run_step(library, "configure", library_config)}
    if package_steps[#package_steps].passed then
        table.insert(package_steps, checks.run_step(library, "package",
            {"package", "-P", library, "-o", package_output, "cppx"}))
    end
    report.package = {passed = package_steps[#package_steps].passed, steps = package_steps}

    if report.package.passed then
        local package_consumer = path.join(directory, "package-consumer")
        checks.copy_project(path.join(root, "tests", "consumer"), package_consumer)
        local package_config = checks.configuration(package_consumer, mode)
        table.insert(package_config, "--cppx_packages=" .. package_output)
        report.package_consumer = checks.verify_project(package_consumer, package_config)
        print("Package consumer: %s", report.package_consumer.passed and "passed" or "failed (see logs)")
    end
    report.passed = report.source.passed and report.package.passed and
                    report.package_consumer and report.package_consumer.passed or false
    checks.report(reportfile, report)
    if not report.passed then
        raise("Consumer verification failed; inspect " .. reportfile)
    end
end
