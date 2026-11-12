"""Isolated builds, bounded tests and machine-readable evidence. Python 3.10+."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import shutil
import signal
import subprocess
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
INPUTS = ["xmake.lua", "LICENSE", "src", "tests", "examples", "quality", "scripts"]


def copy_inputs(destination: Path) -> dict[str, str]:
    hashes = {}
    for entry in INPUTS:
        source = ROOT / entry
        files = sorted(source.rglob("*")) if source.is_dir() else [source]
        for file in files:
            if not file.is_file() or "__pycache__" in file.parts:
                continue
            relative = file.relative_to(ROOT)
            target = destination / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(file, target)
            hashes[relative.as_posix()] = hashlib.sha256(file.read_bytes()).hexdigest()
    return hashes


def differential_inputs(directory: Path, seed: int):
    randomizer = random.Random(seed)
    def value(depth=0):
        kind = randomizer.randrange(5 if depth >= 4 else 7)
        if kind == 0: return None
        if kind == 1: return bool(randomizer.randrange(2))
        if kind == 2: return randomizer.randint(-(2**63), 2**64 - 1)
        if kind == 3: return randomizer.uniform(-1e200, 1e200)
        if kind == 4:
            return "".join(randomizer.choice('abc中文😀\\\"\n\t\x00') for _ in range(randomizer.randrange(30)))
        if kind == 5: return [value(depth + 1) for _ in range(randomizer.randrange(7))]
        return {"key" + str(i): value(depth + 1) for i in range(randomizer.randrange(7))}
    cases = [value() for _ in range(2000)] + [-(2**63), 2**64 - 1, 2**53 + 1, -0.0]
    source = directory / "differential-input.jsonl"
    source.write_text("\n".join(json.dumps(item, ensure_ascii=True, allow_nan=False) for item in cases),
                      encoding="utf-8")
    return source, cases


class Run:
    def __init__(self, directory: Path):
        self.directory = directory.resolve()
        self.directory.mkdir(parents=True, exist_ok=False)
        self.report = {"schema_version": 1, "platform": platform.platform(),
                       "machine": platform.machine(), "python": platform.python_version(),
                       "steps": [], "passed": False}

    def save(self):
        (self.directory / "report.json").write_text(
            json.dumps(self.report, indent=2), encoding="utf-8")
        suite = ET.Element("testsuite", name="cppx-verification",
                           tests=str(len(self.report["steps"])),
                           failures=str(sum(not s["passed"] for s in self.report["steps"])))
        for step in self.report["steps"]:
            case = ET.SubElement(suite, "testcase", name=step["name"],
                                 time=str(step["seconds"]))
            if not step["passed"]:
                ET.SubElement(case, "failure", message="See " + step["log"])
        ET.ElementTree(suite).write(self.directory / "junit.xml", encoding="utf-8",
                                    xml_declaration=True)

    def step(self, name, arguments, *, cwd=None, timeout=300, expected_diagnostic=None):
        arguments = [str(a) for a in arguments]
        log = self.directory / (name + ".log")
        env = dict(os.environ, XMAKE_COLORTERM="nocolor",
                   XMAKE_CONFIGDIR=str(self.directory / "config"),
                   XMAKE_PKG_INSTALLDIR=str(self.directory / "packages"),
                   XMAKE_PKG_CACHEDIR=str(self.directory / "package-cache"),
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
                   TSAN_OPTIONS="halt_on_error=1")
        start = time.monotonic()
        timed_out = False
        with log.open("wb") as output:
            kwargs = ({"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP}
                      if os.name == "nt" else {"start_new_session": True})
            process = subprocess.Popen(arguments, cwd=cwd or self.directory, env=env,
                                       stdout=output, stderr=subprocess.STDOUT, **kwargs)
            try:
                code = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                if os.name == "nt":
                    subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
                else:
                    os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                code = -1
        content = log.read_text(encoding="utf-8", errors="replace")
        passed = code == 0 and not timed_out
        if expected_diagnostic:
            passed = code != 0 and not timed_out and expected_diagnostic in content
        # A stale standard-library BMI is not a valid successful build.
        if "C5050" in content:
            passed = False
        step = {"name": name, "command": arguments, "exit_code": code,
                "seconds": round(time.monotonic() - start, 3), "timed_out": timed_out,
                "passed": passed, "log": str(log), "expected_diagnostic": expected_diagnostic}
        self.report["steps"].append(step)
        self.save()
        print(f"{name}: {'passed' if passed else 'FAILED'} ({step['seconds']}s)", flush=True)
        if not passed:
            raise RuntimeError(f"{name} failed; inspect {log}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--mode", choices=["debug", "release"], default="release")
    parser.add_argument("--sanitizer", choices=["none", "address", "undefined", "thread"], default="none")
    parser.add_argument("--toolchain")
    parser.add_argument("--sdk")
    parser.add_argument("--runtimes")
    parser.add_argument("--seconds", type=int, default=30)
    parser.add_argument("--iterations", type=int, default=10000)
    parser.add_argument("--seed", type=int, default=20261012)
    args = parser.parse_args()
    if args.seconds < 1 or args.iterations < 1 or args.seed < 0:
        parser.error("seconds, iterations must be positive; seed must be nonnegative")
    directory = args.output or ROOT / "build" / ("verify-" + time.strftime("%Y%m%d-%H%M%S"))
    run = Run(directory)
    run.report["options"] = {k: str(v) if isinstance(v, Path) else v for k, v in vars(args).items()}
    project = run.directory / "project"
    run.report["inputs_sha256"] = copy_inputs(project)
    xmake = shutil.which("xmake")
    if not xmake:
        raise RuntimeError("xmake is required")
    try:
        run.step("xmake-version", [xmake, "--version"])
        configure = [xmake, "f", "-P", project, "-c", "-y", "-m", args.mode,
                     "-o", project / "build", "--cppx_build_examples=n", "--cppx_build_quality=y",
                     "--cppx_fault_injection=y", "--cppx_sanitizer=" + args.sanitizer,
                     "--cppx_build_benchmarks=n"]
        for name in ["toolchain", "sdk", "runtimes"]:
            if value := getattr(args, name):
                configure.append("--" + name + "=" + value)
        run.step("configure", configure)
        run.step("build", [xmake, "build", "-P", project, "-r", "-j", "2", "-v"], timeout=1200)
        def execute(name, *arguments, **kwargs):
            run.step(name, [xmake, "run", "-P", project, name, *arguments], **kwargs)
        execute("sanitizer_probe")
        run.step("reject-fault-package", [xmake, "package", "-P", project, "-o",
                 run.directory / "rejected-package", "cppx"], expected_diagnostic="Cannot package a fault-injection build")
        run.step("reject-fault-install", [xmake, "install", "-P", project, "-o",
                 run.directory / "rejected-install", "cppx"], expected_diagnostic="Cannot install a fault-injection build")
        if args.sanitizer != "none":
            diagnostic = {"address": "AddressSanitizer: heap-buffer-overflow",
                          "undefined": "runtime error: signed integer overflow",
                          "thread": "ThreadSanitizer: data race"}[args.sanitizer]
            run.step("sanitizer-negative-control",
                     [xmake, "run", "-P", project, "sanitizer_probe", args.sanitizer],
                     expected_diagnostic=diagnostic)
        for name in sorted(re.findall(r'add_cppx_test\("([^"]+)"',
                                      (project / "tests/xmake.lua").read_text(encoding="utf-8"))):
            execute(name, timeout=90)
        execute("fuzz_inputs", project / "quality/corpus", str(args.iterations), str(args.seed),
                run.directory / "fuzz-reproducer.bin", timeout=max(180, args.iterations // 50))
        source, expected = differential_inputs(run.directory, args.seed)
        output = run.directory / "differential-output.jsonl"
        execute("json_roundtrip", source, output)
        actual = [json.loads(line) for line in output.read_text(encoding="utf-8").splitlines()]
        if actual != expected:
            raise RuntimeError("cppx JSON differs from the Python standard-library oracle")
        run.report["json_differential"] = {"cases": len(expected), "oracle": "Python json", "passed": True}
        execute("stress", str(args.seconds), run.directory / "resources.jsonl", timeout=args.seconds + 120)
        samples = [json.loads(line) for line in (run.directory / "resources.jsonl").read_text().splitlines()]
        run.report["resources"] = {key: {"min": min(s[key] for s in samples),
                                         "max": max(s[key] for s in samples), "last": samples[-1][key]}
                                    for key in ["resident_bytes", "open_handles", "threads"]}
        run.report["resource_samples"] = len(samples)
        run.report["passed"] = True
        run.save()
    except Exception as error:
        run.report["error"] = str(error)
        run.save()
        raise
    print("Report:", run.directory / "report.json")


if __name__ == "__main__":
    main()
