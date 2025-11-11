#!/usr/bin/env python3
import argparse
import subprocess
from pathlib import Path
import re
import sys
from typing import Tuple

BUILD_LOG = Path("build.log")

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Clean + build with scons, log output, and summarize errors/warnings."
    )
    p.add_argument("--build-xml", default="env_cfg/baseline_project.xml",
                   help="Path to the project XML passed to --build (default: %(default)s)")
    p.add_argument("--board", default="ra8p1_ek",
                   help="Board passed to --board (default: %(default)s)")
    p.add_argument("--compiler", default="llvm",
                   help="Compiler for the build step (default: %(default)s)")
    p.add_argument("--clean-compiler", default=None,
                   help="Compiler for the clean step (default: same as --compiler)")
    p.add_argument("--keep-log", action="store_true",
                   help="Do not delete build.log before running")
    p.add_argument("--no-clean", action="store_true",
                   help="Skip the clean step")
    return p.parse_args()

def delete_log(keep: bool):
    if keep:
        return
    if BUILD_LOG.exists():
        try:
            BUILD_LOG.unlink()
            print("Deleted build.log")
        except Exception as e:
            print(f"Warning: couldn't delete build.log: {e}")

def scons_cmd(base_flags, build_xml, compiler, board):
    return ["scons", *base_flags, f"--build={build_xml}", f"--compiler={compiler}", f"--board={board}"]

def run_clean(build_xml: str, compiler: str, board: str) -> int:
    print(f"Running clean… (compiler={compiler})")
    cmd = scons_cmd(["-Q", "-c"], build_xml, compiler, board)
    print("$ " + " ".join(cmd))
    result = subprocess.run(cmd)
    return result.returncode

def run_build(build_xml: str, compiler: str, board: str) -> int:
    print(f"Running build… (compiler={compiler}) -> logging to build.log")
    cmd = scons_cmd(["-k"], build_xml, compiler, board)
    print("$ " + " ".join(cmd) + " >> build.log 2>&1")
    with BUILD_LOG.open("w", encoding="utf-8", errors="replace") as f:
        result = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)
    return result.returncode

# ------- Log parsing (errors & warnings) -------

# Error patterns: GCC/Clang/MSVC-ish + SCons hard failures
ERROR_PATTERNS = [
    re.compile(r"\bfatal error\b", re.IGNORECASE),
    re.compile(r"(?<!no )\berror\b\s*:?", re.IGNORECASE),   # matches "error:" or "error"
    re.compile(r"\berrors?\s+generated\b", re.IGNORECASE),  # clang summary
    re.compile(r"^scons:\s+\*\*\*", re.IGNORECASE),         # scons stops on task failure
]

# Warning patterns: GCC/Clang/MSVC + SCons + linkers
WARNING_PATTERNS = [
    re.compile(r"(?<!no )\bwarning\b\s*:?", re.IGNORECASE),             # "warning:" or "warning"
    re.compile(r"\bwarning\s*C\d{4}\b", re.IGNORECASE),                 # MSVC "warning C4996"
    re.compile(r"\bdeprecated\b", re.IGNORECASE),                       # some toolchains omit "warning:"
    re.compile(r"\b[Cc]aution\b", re.IGNORECASE),                       # some linkers/tools
    re.compile(r"^scons:\s+warning:", re.IGNORECASE),
]

# Reduce common false positives
NEGATIVE_HINTS = [
    re.compile(r"\b0\s+errors?\b", re.IGNORECASE),
    re.compile(r"\bno\s+error(s)?\b", re.IGNORECASE),
]
NEGATIVE_HINTS_WARN = [
    re.compile(r"\b0\s+warnings?\b", re.IGNORECASE),
    re.compile(r"\bno\s+warnings?\b", re.IGNORECASE),
    re.compile(r"\b\d+\s+warnings?\s+generated\b", re.IGNORECASE),  # clang summary (avoid double count)
]

def count_issues_in_log(path: Path) -> Tuple[int, int]:
    if not path.exists():
        return 0, 0

    errors = 0
    warnings = 0

    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for raw in f:
                line = raw.strip()

                # --- errors ---
                if not any(p.search(line) for p in NEGATIVE_HINTS):
                    if any(p.search(line) for p in ERROR_PATTERNS):
                        errors += 1

                # --- warnings ---
                if not any(p.search(line) for p in NEGATIVE_HINTS_WARN):
                    # Avoid counting compiler/linker summary lines as individual warnings
                    if any(p.search(line) for p in WARNING_PATTERNS) and not re.search(r"\b(warnings?|errors?)\s+generated\b", line, re.IGNORECASE):
                        warnings += 1

        # If the log has a clear clang summary like "N warnings generated", prefer that as the warning count
        with path.open("r", encoding="utf-8", errors="replace") as f2:
            summary_max = None
            for line in f2:
                m = re.search(r"\b(\d+)\s+warnings?\s+generated\b", line, re.IGNORECASE)
                if m:
                    n = int(m.group(1))
                    summary_max = n if summary_max is None else max(summary_max, n)
            if summary_max is not None:
                warnings = max(warnings, summary_max)

    except Exception as e:
        print(f"Warning: couldn't read log for parsing: {e}")

    return errors, warnings

def main():
    args = parse_args()
    delete_log(keep=args.keep_log)

    clean_comp = args.clean_compiler or args.compiler

    if not args.no_clean:
        rc_clean = run_clean(args.build_xml, clean_comp, args.board)
        if rc_clean != 0:
            print(f"Clean step returned non-zero exit code: {rc_clean}")

    rc_build = run_build(args.build_xml, args.compiler, args.board)

    err_cnt, warn_cnt = count_issues_in_log(BUILD_LOG)

    if rc_build != 0 or err_cnt > 0:
        status = "FAILED"
        exit_code = 1
    else:
        status = "SUCCEEDED"
        exit_code = 0

    print(f"{status}: {err_cnt} ERROR(S), {warn_cnt} WARNING(S)")
    if BUILD_LOG.exists():
        print(f"(See {BUILD_LOG} for details)")
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
