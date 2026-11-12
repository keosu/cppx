"""Rebuild a common API consumer against baseline -> workspace -> baseline, retaining data."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tarfile

from verify import ROOT, Run, copy_inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", default="HEAD", help="Local Git commit/tag to restore on rollback")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    revision = subprocess.check_output(["git", "rev-parse", "--verify", args.baseline + "^{commit}"],
                                        cwd=ROOT, text=True).strip()
    run = Run(args.output)
    run.report["baseline_commit"] = revision
    archive = run.directory / "baseline.tar"
    run.step("baseline-archive", ["git", "archive", "--format=tar", "-o", archive,
                                revision, "xmake.lua", "src", "LICENSE"], cwd=ROOT)
    baseline = run.directory / "baseline"
    with tarfile.open(archive) as source:
        for member in source:
            if member.isdir(): continue
            target = (baseline / member.name).resolve()
            if not member.isfile() or not target.is_relative_to(baseline):
                raise RuntimeError("Unexpected Git archive member")
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(source.extractfile(member).read())
    candidate = run.directory / "candidate"
    run.report["candidate_inputs_sha256"] = copy_inputs(candidate)
    xmake = shutil.which("xmake")
    for index, (name, library) in enumerate([("baseline", baseline), ("upgrade", candidate),
                                           ("rollback", baseline)], start=1):
        project = run.directory / (name + "-consumer")
        shutil.copytree(ROOT / "tests/upgrade", project)
        run.step(name + "-configure", [xmake, "f", "-P", project, "-c", "-y", "-m", "release",
                                        "-o", project / "build", "--cppx_source=" + str(library)])
        run.step(name + "-build", [xmake, "build", "-P", project, "-r", "-j", "2", "-v"], timeout=600)
        run.step(name + "-run", [xmake, "run", "-P", project, "upgrade", run.directory / "state.json", str(index)])
    run.report["passed"] = True
    run.save()


if __name__ == "__main__":
    main()
