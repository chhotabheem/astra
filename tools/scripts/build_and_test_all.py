#!/usr/bin/env python3
"""
Run all cmake preset builds and tests.
No options. No arguments. Just runs everything.
"""

import os
import subprocess
import sys
import time
from pathlib import Path

PRESETS = [
    "gcc-debug",
    "gcc-release",
    "gcc-asan",
    "gcc-tsan",
    "clang-debug",
    "clang-release",
    "clang-asan",
    "clang-tsan",
    "clang-ubsan",
]

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
LOG_DIR = PROJECT_ROOT / "build" / "logs"


def run(cmd, log_path):
    """Run command, log output, return exit code."""
    with open(log_path, "w") as f:
        p = subprocess.Popen(cmd, shell=True, stdout=f, stderr=subprocess.STDOUT, cwd=PROJECT_ROOT)
        p.wait()
    return p.returncode


def main():
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    results = {}

    for preset in PRESETS:
        print(f"Running {preset}...", flush=True)
        start = time.time()
        res = {"config": "PENDING", "build": "PENDING", "test": "PENDING", "time": 0}

        # Configure
        if run(f"cmake --preset {preset}", LOG_DIR / f"{preset}_config.log") != 0:
            res["config"] = "FAILED"
            results[preset] = res
            continue
        res["config"] = "PASSED"

        # Build
        if run(f"cmake --build --preset {preset} -j2", LOG_DIR / f"{preset}_build.log") != 0:
            res["build"] = "FAILED"
            results[preset] = res
            continue
        res["build"] = "PASSED"

        # Test
        if run(f"ctest --preset {preset}", LOG_DIR / f"{preset}_test.log") != 0:
            res["test"] = "FAILED"
            results[preset] = res
            continue
        res["test"] = "PASSED"

        res["time"] = time.time() - start
        results[preset] = res

    # Valgrind
    print("Running valgrind...", flush=True)
    valgrind_status = "SKIPPED"
    build_dir = PROJECT_ROOT / "build" / "gcc-debug"
    if build_dir.exists():
        if run(f"ctest -T memcheck --test-dir {build_dir}", LOG_DIR / "valgrind.log") != 0:
            valgrind_status = "FAILED"
        else:
            valgrind_status = "PASSED"

    # Report
    print()
    print("=" * 70)
    print("                    VERIFICATION REPORT")
    print("=" * 70)
    print(f"{'PRESET':<15} | {'CONFIG':<8} | {'BUILD':<8} | {'TEST':<8} | {'TIME':<10}")
    print("-" * 70)

    failed = False
    for preset in PRESETS:
        r = results.get(preset, {})
        cfg = r.get("config", "-")
        bld = r.get("build", "-")
        tst = r.get("test", "-")
        t = r.get("time", 0)
        time_str = f"{t:.1f}s" if t > 0 else "-"
        print(f"{preset:<15} | {cfg:<8} | {bld:<8} | {tst:<8} | {time_str:<10}")
        if "FAILED" in [cfg, bld, tst]:
            failed = True

    print("-" * 70)
    print(f"{'Valgrind':<15} | {valgrind_status:<39}")
    if valgrind_status == "FAILED":
        failed = True

    print("=" * 70)
    print(f"Logs directory: {LOG_DIR}")
    print("=" * 70)

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
