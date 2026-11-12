"""Create a reproducible source archive, manifest and pinned private xrepo recipe."""
from __future__ import annotations

import argparse
import functools
import gzip
import hashlib
import http.server
import io
import json
from pathlib import Path
import re
import shutil
import tarfile
import threading

from verify import ROOT, Run

ENTRIES = ["xmake.lua", "LICENSE", "README.md", "CONTRIBUTING.md", "CHANGELOG.md",
           ".clang-format", "src", "docs", "scripts", "tests", "examples", "quality", "packaging"]


def archive_bytes() -> tuple[str, bytes, dict]:
    versions = re.findall(r'set_version\("([^"]+)"\)', (ROOT / "xmake.lua").read_text(encoding="utf-8"))
    if not versions or len(set(versions)) != 1:
        raise ValueError("Project and target versions must agree")
    version = versions[0]
    manifest = {}
    content = io.BytesIO()
    with gzip.GzipFile(fileobj=content, mode="wb", filename="", mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode="w", format=tarfile.PAX_FORMAT) as archive:
            files = []
            for entry in ENTRIES:
                source = ROOT / entry
                files.extend(source.rglob("*") if source.is_dir() else [source])
            for file in sorted(files):
                if not file.is_file() or "__pycache__" in file.parts:
                    continue
                if file.is_symlink():
                    raise ValueError(f"Distribution must not contain symlinks: {file}")
                data = file.read_bytes()
                relative = file.relative_to(ROOT).as_posix()
                manifest[relative] = hashlib.sha256(data).hexdigest()
                info = tarfile.TarInfo(f"cppx-{version}/{relative}")
                info.size, info.mode, info.mtime = len(data), 0o644, 0
                archive.addfile(info, io.BytesIO(data))
    return version, content.getvalue(), manifest


def recipe(directory: Path, version: str, digest: str, url: str):
    # Lua long-string syntax is unnecessary: URLs are restricted and cannot inject code.
    if any(char in url for char in ['"', "\\", "\r", "\n"]):
        raise ValueError("URL contains characters unsafe for a Lua quoted string")
    target = directory / "packages/c/cppx/xmake.lua"
    target.parent.mkdir(parents=True, exist_ok=True)
    template = (ROOT / "packaging/xmake.lua.in").read_text(encoding="utf-8")
    target.write_bytes(template.replace("@VERSION@", version).replace("@SHA256@", digest)
                       .replace("@URL@", url).encode("utf-8"))


class QuietServer(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *_):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True, help="Fresh output directory")
    parser.add_argument("--base-url", help="Release directory URL; artifacts are never published")
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--mode", choices=["debug", "release"], default="release")
    parser.add_argument("--toolchain")
    parser.add_argument("--sdk")
    parser.add_argument("--runtimes")
    args = parser.parse_args()
    run = Run(args.output)
    version, data, manifest = archive_bytes()
    if archive_bytes()[1] != data:
        raise RuntimeError("Archive is not reproducible or inputs changed during packaging")
    digest = hashlib.sha256(data).hexdigest()
    filename = f"cppx-{version}.tar.gz"
    (run.directory / filename).write_bytes(data)
    (run.directory / "SHA256SUMS").write_text(f"{digest}  {filename}\n", encoding="ascii")
    (run.directory / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    base_url = args.base_url or f"https://github.com/keosu/cppx/releases/download/v{version}"
    recipe(run.directory / "repository", version, digest, base_url.rstrip("/") + "/" + filename)
    run.report.update(version=version, archive=filename, sha256=digest, reproducible=True,
                      consumer_verified=False, mode=args.mode)
    run.save()
    if args.verify:
        # Serve exactly this archive on loopback. The package manager downloads it and verifies SHA-256.
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0),
                  functools.partial(QuietServer, directory=str(run.directory)))
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            repository = run.directory / "verification-repository"
            recipe(repository, version, digest, f"http://127.0.0.1:{server.server_port}/{filename}")
            consumer = run.directory / "consumer"
            shutil.copytree(ROOT / "tests/consumer", consumer)
            xmake = shutil.which("xmake")
            if not xmake:
                raise RuntimeError("xmake is required")
            config = [xmake, "f", "-P", consumer, "-c", "-y", "-m", args.mode,
                      "-o", consumer / "build", "--cppx_packages=" + str(repository),
                      "--cppx_version=" + version]
            for name in ["toolchain", "sdk", "runtimes"]:
                if value := getattr(args, name): config.append("--" + name + "=" + value)
            run.step("consumer-configure", config, timeout=1200)
            run.step("consumer-build", [xmake, "build", "-P", consumer, "-j", "2", "-v"], timeout=1200)
            run.step("consumer-run", [xmake, "run", "-P", consumer], timeout=90)
            # Check installed sources against the archive, not just a process exit code.
            modules = list((run.directory / "packages").rglob("*.cppm"))
            expected = {Path(name).name: digest for name, digest in manifest.items()
                        if name.startswith("src/") and name.endswith(".cppm")}
            actual = {file.name: hashlib.sha256(file.read_bytes()).hexdigest() for file in modules}
            if len(modules) != len(expected) or actual != expected:
                raise RuntimeError("Installed module sources differ from the release manifest")
            run.report["consumer_verified"] = True
        finally:
            server.shutdown()
            server.server_close()
            thread.join()
    run.report["passed"] = True
    run.save()
    print("Source package:", run.directory / filename)


if __name__ == "__main__":
    main()
