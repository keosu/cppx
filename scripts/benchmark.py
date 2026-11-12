"""Measure production builds without sanitizer/fault hooks; never imposes a noisy CI threshold."""
import argparse
import json
from pathlib import Path
import shutil
import statistics

from verify import ROOT, Run, copy_inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--operations", type=int, default=20000)
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--toolchain")
    parser.add_argument("--sdk")
    parser.add_argument("--runtimes")
    args = parser.parse_args()
    if args.operations < 100 or args.repeats < 3:
        parser.error("At least 100 operations and 3 repeats are required")
    run = Run(args.output)
    project = run.directory / "project"
    run.report["inputs_sha256"] = copy_inputs(project)
    run.report["measurement"] = {"sanitizer": "none", "fault_injection": False,
                                  "mode": "release", "operations": args.operations,
                                  "repeats": args.repeats, "regression_threshold": None}
    xmake = shutil.which("xmake")
    config = [xmake, "f", "-P", project, "-c", "-y", "-m", "release", "-o", project / "build",
              "--cppx_build_tests=n", "--cppx_build_examples=n", "--cppx_build_benchmarks=y"]
    for name in ["toolchain", "sdk", "runtimes"]:
        if value := getattr(args, name): config.append("--" + name + "=" + value)
    run.step("configure", config)
    run.step("build", [xmake, "build", "-P", project, "-j", "2", "-v"], timeout=600)
    results = {}
    for index in range(args.repeats):
        output = run.directory / f"measurement-{index}.json"
        run.step(f"measurement-{index}", [xmake, "run", "-P", project, "benchmark",
                                          str(args.operations), output])
        report = json.loads(output.read_text(encoding="utf-8"))
        run.report["compiler"] = report["compiler"]
        run.report["hardware_threads"] = report["hardware_threads"]
        for result in report["results"]:
            results.setdefault(result["name"], []).append(result["operations_per_second"])
    run.report["throughput"] = {name: {"min": min(values), "max": max(values),
                                       "median": statistics.median(values),
                                       "relative_span": (max(values) - min(values)) / statistics.median(values)}
                                 for name, values in results.items()}
    run.report["passed"] = True
    run.save()


if __name__ == "__main__":
    main()
