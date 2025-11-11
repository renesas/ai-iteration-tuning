#!/usr/bin/env python3
import os
import sys
import tempfile
import subprocess
import platform
import time
import signal
import re
from pathlib import Path

# ---------- CONFIG (override via env) ----------
DEFAULT_GDB = str(Path(r"C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin\arm-none-eabi-gdb.exe"))
ELF_PATH  = os.environ.get("FW_ELF", r"build/baseline_project/ra8p1_ek/llvm/build_baseline_project_typical_debug/build_baseline_project_typical_debug.elf")

# Board / Device
DEVICE_RAW = os.environ.get("JLINK_DEVICE", "R7KA8P1KFLCAC")
CORE       = os.environ.get("CORE", "CPU0").upper()  # CPU0 or CPU1
INTERFACE  = os.environ.get("JLINK_IF", "SWD")
SPEED_KHZ  = os.environ.get("JLINK_SPEED", "4000")

# Tools
IS_WIN = platform.system() == "Windows"
JLINK_EXE = os.environ.get("JLINK_EXE", "JLink.exe" if IS_WIN else "JLinkExe")
JLINK_GDBSERVER_EXE = os.environ.get("JLINK_GDBSERVER", "JLinkGDBServerCL.exe" if IS_WIN else "JLinkGDBServerCLExe")
GDB_EXE   = os.environ.get("GDB", DEFAULT_GDB)

# Ports
GDB_PORT    = int(os.environ.get("GDB_PORT", "2331"))
TELNET_PORT = int(os.environ.get("JLINK_TELNET_PORT", "2333"))

# Behavior
USE_JLINK_FLASH = os.environ.get("USE_JLINK_FLASH", "1") != "0"  # 0 => flash via GDB 'load'
# -----------------------------------------------------------

def normalize_device(dev: str, core: str) -> str:
    """Map package P/N (e.g., R7KA8P1KFLCAC) to J-Link's canonical device (e.g., R7KA8P1KF_CPU0)."""
    dev = dev.strip().upper()
    # Already canonical?
    if re.fullmatch(r"R7[JK]A8P1[A-Z]{2}(_CPU[01])?", dev):
        if "_CPU" not in dev:
            return f"{dev}_{core}"
        return dev

    # Extract base like R7KA8P1KF from longer P/N
    m = re.match(r"^(R7[JK]A8P1)([A-Z]{2})", dev)
    if m:
        base = f"{m.group(1)}{m.group(2)}"
        return f"{base}_{core}"

    return dev

DEVICE = normalize_device(DEVICE_RAW, CORE)

def run_list(args, **kwargs):
    """Run a subprocess with argument list (cross-platform safe)."""
    print("\n$ " + " ".join([f"\"{a}\"" if " " in str(a) else str(a) for a in args]))
    return subprocess.run(args, check=True, **kwargs)

def wait_for_gdbserver_ready(proc, timeout=15.0):
    """Wait until J-Link GDB Server announces it's listening for GDB."""
    start = time.time()
    ready = False
    while time.time() - start < timeout:
        line = proc.stdout.readline()
        if not line:
            time.sleep(0.05)
            continue
        line = line.rstrip("\r\n")
        print("[JLinkGDBServer] " + line)
        if ("Listening on TCP/IP port" in line) or ("Waiting for GDB connection" in line):
            ready = True
            break
        if ("Could not connect" in line) or ("ERROR" in line):
            break
    if not ready:
        print("J-Link GDB Server did not become ready in time.")
    return ready

def main():
    # 0) Sanity checks
    if not os.path.exists(ELF_PATH):
        print(f"ERROR: ELF not found: {ELF_PATH}", file=sys.stderr)
        sys.exit(1)

    print(f"Resolved DEVICE: {DEVICE}  (from '{DEVICE_RAW}', core={CORE})")

    # 1) Optional flash via J-Link Commander
    if USE_JLINK_FLASH:
        with tempfile.TemporaryDirectory() as td:
            flash_script = os.path.join(td, "flash.jlink")
            script_text = (
                f"device {DEVICE}\n"
                f"if {INTERFACE}\n"
                f"speed {SPEED_KHZ}\n"
                "r\n"
                "h\n"
                f"loadfile {ELF_PATH}\n"
                "verify\n"
                "r\n"
                "q\n"
            )
            with open(flash_script, "w", encoding="utf-8") as f:
                f.write(script_text)

            print(f"\n[flash.jlink path] {flash_script}")
            print("[flash.jlink content]\n" + script_text)
            run_list([JLINK_EXE, "-CommandFile", flash_script])

    # 2) Launch J-Link GDB Server
    print("\nStarting J-Link GDB Server...")
    gdbserver_args = [
        JLINK_GDBSERVER_EXE,
        "-device", DEVICE,
        "-if", INTERFACE,
        "-speed", str(SPEED_KHZ),
        "-port", str(GDB_PORT),
        "-telnetport", str(TELNET_PORT),
        "-autoconnect", "1",
        "-singlerun",
        "-strict",
        "-vd",
    ]
    print("\n$ " + " ".join([f"\"{a}\"" if " " in str(a) else str(a) for a in gdbserver_args]))

    creationflags = subprocess.CREATE_NEW_PROCESS_GROUP if IS_WIN else 0
    gdbserver = subprocess.Popen(
        gdbserver_args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        creationflags=creationflags
    )

    if not wait_for_gdbserver_ready(gdbserver, timeout=15.0):
        print("ERROR: GDB server failed to start or bind the port.", file=sys.stderr)
        try:
            gdbserver.terminate()
        except Exception:
            pass
        sys.exit(2)

    # 3) Launch GDB with a small command file (FIXED: build lines then join)
    with tempfile.TemporaryDirectory() as td:
        gdb_cmds = os.path.join(td, "debug.gdb")

        gdb_lines = [
            f"target extended-remote :{GDB_PORT}",
            "monitor reset halt",
        ]
        if not USE_JLINK_FLASH:
            gdb_lines.append("load")
        gdb_lines += [
            "monitor reset halt",
            "break main",
            "set confirm off",
            # "continue",  # uncomment to run to main and stay attached
            "quit",
        ]
        gdb_text = "\n".join(gdb_lines) + "\n"

        with open(gdb_cmds, "w", encoding="utf-8") as f:
            f.write(gdb_text)

        print(f"\n[debug.gdb path] {gdb_cmds}")
        print("[debug.gdb content]\n" + gdb_text)

        try:
            run_list([GDB_EXE, "-x", gdb_cmds, ELF_PATH])
        finally:
            # 4) Cleanup GDB server
            print("\nShutting down J-Link GDB Server...")
            if gdbserver.poll() is None:
                try:
                    if IS_WIN:
                        gdbserver.terminate()
                    else:
                        os.kill(gdbserver.pid, signal.SIGINT)
                        time.sleep(0.5)
                        if gdbserver.poll() is None:
                            gdbserver.terminate()
                except Exception:
                    pass
            try:
                out, _ = gdbserver.communicate(timeout=2)
                if out:
                    print("\n[JLinkGDBServer output]\n" + out)
            except Exception:
                pass

if __name__ == "__main__":
    main()
