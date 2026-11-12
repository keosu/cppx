"""Verify minimal module dependency closure, isolation and configuration rejection."""
import argparse
from pathlib import Path
import shutil

from verify import ROOT, Run, copy_inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run = Run(args.output)
    library = run.directory / "library"
    run.report["inputs_sha256"] = copy_inputs(library)
    consumer = run.directory / "consumer"
    consumer.mkdir()
    (consumer / "xmake.lua").write_text('''set_project("minimal_consumer")
set_xmakever("3.1.1")
add_rules("mode.release")
includes("../library")
target("minimal")
    set_kind("binary")
    set_languages("c++23")
    add_files("main.cpp")
    add_deps("cppx")
    on_config(function (target)
        import("core.project.project")
        local expected = {error=true, string_utils=true, time_utils=true, logging=true, serialization=true}
        local library = project.target("cppx")
        local count = 0
        for _, file in ipairs(library:sourcefiles()) do
            assert(expected[path.basename(file)], "Unexpected component: " .. file)
            count = count + 1
        end
        assert(count == 5, "Missing component dependency")
        assert(project.name() == "minimal_consumer", "Host project name overwritten")
        count = 0
        for _ in pairs(project.targets()) do count = count + 1 end
        assert(count == 2, "Developer tools enabled in consumer")
    end)
target_end()
''', encoding="utf-8")
    (consumer / "main.cpp").write_text('''import std;
import cppx.logging;
import cppx.serialization;
int main() {
    auto log = cppx::create_logger("minimal");
    log->remove_console_sink();
    return cppx::json::parse("42").as_int() == 42 ? 0 : 1;
}
''', encoding="utf-8")
    xmake = shutil.which("xmake")
    config = [xmake, "f", "-P", consumer, "-c", "-y", "-m", "release",
              "--cppx_components=logging,serialization"]
    run.step("configure", config)
    run.step("build", [xmake, "build", "-P", consumer, "-j", "2", "-v"], timeout=600)
    run.step("run", [xmake, "run", "-P", consumer])
    run.step("reject-unknown", config[:-1] + ["--cppx_components=unknown"],
             expected_diagnostic="Unknown cppx component: unknown")
    run.report["passed"] = True
    run.save()


if __name__ == "__main__":
    main()
