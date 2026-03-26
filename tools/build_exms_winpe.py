#!/usr/bin/env python3
"""
exMs WinPE Builder
==================

Builds a custom WinPE ISO with exMs test files for QEMU testing.

Usage:
    python tools/build_exms_winpe.py

Requirements:
    - Windows ADK with WinPE add-on
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

# Paths
ADK_PATH = Path(r"C:\Program Files (x86)\Windows Kits\10")
WORK_DIR = Path(r"D:\exMs_winpe")
ISO_OUTPUT = Path(r"C:\exMs_test.iso")
EXMS_BUILD = Path(r"C:\Users\Administrator\exMs\build")

def run_cmd(cmd, check=True, env=None):
    """Run command and return result."""
    print(f"[CMD] {' '.join(cmd)}")
    if env:
        result = subprocess.run(cmd, capture_output=True, text=True, check=check, env=env)
    else:
        result = subprocess.run(cmd, capture_output=True, text=True, check=check)
    if result.stdout:
        print(result.stdout)
    if result.stderr and result.returncode != 0:
        print(f"[STDERR] {result.stderr}")
    return result

def build_winpe():
    """Build exMs WinPE ISO."""
    print("=" * 60)
    print("    exMs WinPE Builder")
    print("=" * 60)

    # Check ADK
    copype = ADK_PATH / "Assessment and Deployment Kit" / "Windows Preinstallation Environment" / "copype.cmd"
    if not copype.exists():
        print(f"[X] ADK not found at {copype}")
        return False

    print(f"[OK] ADK found")

    # Clean and create work directory
    if WORK_DIR.exists():
        print(f"[*] Cleaning {WORK_DIR}")
        shutil.rmtree(WORK_DIR)

    # Run copype
    print(f"\n[*] Creating WinPE base at {WORK_DIR}")
    adk_root = str(ADK_PATH / "Assessment and Deployment Kit")
    env = os.environ.copy()
    env["WinPERoot"] = adk_root + r"\Windows Preinstallation Environment"
    env["OSCDImgRoot"] = adk_root + r"\Deployment Tools\amd64\Oscdimg"
    env["DISMRoot"] = adk_root + r"\Deployment Tools\amd64\DISM"

    result = run_cmd(["cmd", "/c", str(copype), "amd64", str(WORK_DIR)], env=env)
    if result.returncode != 0:
        print("[X] copype failed")
        return False

    # Create mount point
    mount_dir = WORK_DIR / "mount"
    mount_dir.mkdir(exist_ok=True)
    print(f"[OK] Mount point created")

    # Mount boot.wim
    print("\n[*] Mounting boot.wim...")
    wim_path = WORK_DIR / "media" / "sources" / "boot.wim"
    result = run_cmd(["dism", "/Mount-Image",
                      f"/ImageFile:{wim_path}",
                      "/Index:1",
                      f"/MountDir:{mount_dir}"])
    if result.returncode != 0:
        print("[X] Mount failed")
        return False

    # Create exMs directory in WinPE
    exms_dir = mount_dir / "exms"
    exms_dir.mkdir(exist_ok=True)
    print(f"[OK] Created {exms_dir}")

    # Copy exMs executables
    print("\n[*] Copying exMs files...")
    for exe in ["demo_echo.exe", "stress_test.exe", "elf_test.exe"]:
        src = EXMS_BUILD / exe
        if src.exists():
            shutil.copy(src, exms_dir / exe)
            print(f"  [OK] {exe}")
        else:
            print(f"  [?] {exe} not found, skipping")

    # Create test script
    test_script = exms_dir / "run_tests.cmd"
    test_script.write_text("""@echo off
echo ========================================
echo   exMs WinPE Stability Test
echo ========================================
echo.
echo [*] Testing exMs in WinPE environment...
echo.

echo [1/3] Running demo_echo...
X:\\\\exms\\\\demo_echo.exe
echo.

echo [2/3] Running stress_test...
X:\\\\exms\\\\stress_test.exe
echo.

echo [3/3] Running elf_test...
X:\\\\exms\\\\elf_test.exe
echo.

echo ========================================
echo   All tests completed!
echo ========================================
echo.
echo Press any key to reboot...
pause > nul
wpeutil reboot
""", encoding="mbcs")
    print(f"[OK] Created test script")

    # Configure startnet.cmd
    print("\n[*] Configuring startnet.cmd...")
    startnet = mount_dir / "Windows" / "System32" / "startnet.cmd"
    startnet_content = """@echo off
echo ========================================
echo   exMs WinPE Test Environment
echo ========================================
echo.
wpeinit
echo.
echo [*] Starting exMs tests...
X:\\\\exms\\\\run_tests.cmd
"""
    startnet.write_text(startnet_content, encoding="mbcs")
    print(f"[OK] Configured startnet.cmd")

    # Unmount and commit
    print("\n[*] Unmounting boot.wim...")
    result = run_cmd(["dism", "/Unmount-Image",
                      f"/MountDir:{mount_dir}",
                      "/Commit"])
    if result.returncode != 0:
        print("[X] Unmount failed")
        return False

    # Create ISO using MakeWinPEMedia
    print("\n[*] Creating ISO...")
    make_pe_media = ADK_PATH / "Assessment and Deployment Kit" / "Windows Preinstallation Environment" / "MakeWinPEMedia.cmd"
    if not make_pe_media.exists():
        print(f"[X] MakeWinPEMedia.cmd not found at {make_pe_media}")
        return False

    if ISO_OUTPUT.exists():
        ISO_OUTPUT.unlink()

    # Set up environment for MakeWinPEMedia
    adk_root = str(ADK_PATH / "Assessment and Deployment Kit")
    env = os.environ.copy()
    env["WinPERoot"] = adk_root + r"\Windows Preinstallation Environment"
    env["OSCDImgRoot"] = adk_root + r"\Deployment Tools\amd64\Oscdimg"
    env["DISMRoot"] = adk_root + r"\Deployment Tools\amd64\DISM"
    env["PATH"] = adk_root + r"\Deployment Tools\amd64\Oscdimg;" + env.get("PATH", "")

    result = run_cmd(["cmd", "/c", str(make_pe_media), "/ISO", "/f",
                      str(WORK_DIR), str(ISO_OUTPUT)], env=env, check=False)

    if ISO_OUTPUT.exists():
        sz = ISO_OUTPUT.stat().st_size // (1024 * 1024)
        print(f"\n[OK] ISO created: {ISO_OUTPUT} ({sz} MB)")
        return True
    else:
        print("[X] ISO creation failed")
        return False

def main():
    success = build_winpe()

    print("\n" + "=" * 60)
    if success:
        print("  SUCCESS!")
        print("=" * 60)
        print(f"\nISO: {ISO_OUTPUT}")
        print("\nTo test in QEMU:")
        print('  qemu-system-x86_64.exe -machine q35 -m 1G \\')
        print('    -drive if=pflash,readonly=on,file="C:\\Program Files\\qemu\\share\\edk2-x86_64-code.fd" \\')
        print(f'    -drive file={ISO_OUTPUT},media=cdrom,readonly=on')
        return 0
    else:
        print("  FAILED!")
        print("=" * 60)
        return 1

if __name__ == "__main__":
    sys.exit(main())
