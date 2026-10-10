import("core.base.json")

function workspace(output, name)
    local directory = path.absolute(output or path.join("build", name), os.projectdir())
    -- Each invocation gets fresh project caches and cannot consume stale probe binaries.
    local run_directory = path.join(directory, os.date("%Y%m%d-%H%M%S") .. "-" .. os.getpid())
    assert(not os.exists(run_directory), "Verification directory already exists: " .. run_directory)
    os.mkdir(run_directory)
    return run_directory, path.join(directory, "report.json")
end

function copy_project(source, destination)
    os.mkdir(destination)
    for _, file in ipairs(os.files(path.join(source, "*"))) do
        os.cp(file, path.join(destination, path.filename(file)))
    end
end

function run_step(directory, label, arguments, timeout)
    os.mkdir(directory)
    local stdout = path.join(directory, label .. ".stdout.log")
    local stderr = path.join(directory, label .. ".stderr.log")
    local code, errors = os.execv(os.programfile(), arguments, {
        try = true, timeout = timeout or 300000,
        stdout = stdout, stderr = stderr, curdir = directory,
        envs = {XMAKE_COLORTERM = "nocolor", XMAKE_CONFIGDIR = directory}
    })
    return {
        name = label, arguments = arguments, exit_code = code or -1,
        passed = code == 0, error = errors and tostring(errors) or nil,
        stdout = stdout, stderr = stderr
    }
end

function configuration(directory, mode)
    return {"f", "-P", directory, "-c", "-y", "-m", mode, "-o", path.join(directory, "build")}
end

function verify_project(directory, arguments)
    local steps = {}
    table.insert(steps, run_step(directory, "configure", arguments))
    if steps[#steps].passed then
        table.insert(steps, run_step(directory, "build", {"build", "-P", directory, "-j", "2", "-v"}))
    end
    if steps[#steps].passed then
        table.insert(steps, run_step(directory, "run", {"run", "-P", directory}, 60000))
    end
    return {passed = steps[#steps].passed, steps = steps}
end

function report(filepath, data)
    json.savefile(filepath, data, {indent = 2})
    print("Report: %s", filepath)
end
