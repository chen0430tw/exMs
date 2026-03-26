#!/usr/bin/env python3
"""
Simple build script for exMs using Windows MSVC.
Finds and uses MSVC compiler automatically.
"""

import os
import sys
import subprocess
import glob
from pathlib import Path

def find_msvc():
    """Find MSVC installation."""
    vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if os.path.exists(vswhere):
        result = subprocess.run(
            [vswhere, "-latest", "-property", "installationPath"],
            capture_output=True, text=True
        )
        if result.returncode == 0:
            path = result.stdout.strip()
            vcvars = os.path.join(path, "VC", "Auxiliary", "Build", "vcvars64.bat")
            if os.path.exists(vcvars):
                return vcvars

    # Fallback to known paths
    for vcvars in [
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    ]:
        if os.path.exists(vcvars):
            return vcvars

    return None

def build_exms():
    """Build exMs project."""
    print("[*] exMs Build Script")
    print()

    # Find MSVC
    vcvars = find_msvc()
    if not vcvars:
        print("[!] MSVC not found. Please install Visual Studio Build Tools.")
        return False

    print(f"[+] Found MSVC: {vcvars}")

    # Source files
    root = Path(__file__).parent.parent
    src_files = [
        # Runtime
        "runtime/mem/mem.cpp",
        "runtime/proc/proc.cpp",
        "runtime/fd/fd.cpp",
        "runtime/shm/shm.cpp",
        "runtime/event/event.cpp",
        # Core
        "core/exabi/src/exabi.cpp",
        "core/exloader/src/exloader.cpp",
        "core/expdmf/src/expdmf.cpp",
        "core/expfc/src/expfc.cpp",
        # Compat
        "compat/linux/elf/elf_loader.cpp",
        "compat/linux/syscall/dispatcher.cpp",
    ]

    # Include directories
    includes = [
        "runtime/mem",
        "runtime/proc",
        "runtime/fd",
        "runtime/shm",
        "runtime/event",
        "core/exabi/include",
        "core/exloader/include",
        "core/expdmf/include",
        "core/expfc/include",
        "compat/linux/elf",
        "compat/linux/syscall",
    ]

    # Build include string
    include_args = []
    for inc in includes:
        inc_path = root / inc
        if inc_path.exists():
            include_args.append(f"/I{inc_path}")

    # Build command
    cmd = f'cmd.exe /c ""{vcvars}" && cl.exe /EHsc /std:c++17 /Fe:demo_echo.exe '
    cmd += " ".join(include_args)
    for src in src_files:
        src_path = root / src
        if src_path.exists():
            cmd += f" {src_path}"
        else:
            print(f"[!] Source not found: {src_path}")

    cmd += f" {root / 'apps/demo_echo/main.cpp'}"

    print(f"[*] Building...")
    print(f"[*] Command: {cmd[:100]}...")

    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    print(result.stdout)
    if result.stderr:
        print(result.stderr)

    if result.returncode == 0:
        print("[+] Build successful!")
        return True
    else:
        print(f"[!] Build failed with code {result.returncode}")
        return False

if __name__ == "__main__":
    success = build_exms()
    sys.exit(0 if success else 1)
