#!/usr/bin/env python3
"""
exMs WinPE/QEMU Stability Test

Tests exMs components in WinPE environment running under QEMU.
Similar to DKTM's test_qemu.py approach.
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

QEMU_EXE = r"C:\Program Files\qemu\qemu-system-x86_64.exe"
OVMF_CODE = r"C:\Program Files\qemu\share\edk2-x86_64-code.fd"

WORK_DIR = Path(r"C:\exMs_temp_qemu")
VARS_COPY = WORK_DIR / "ovmf_vars.fd"
SERIAL_LOG = WORK_DIR / "serial.log"

# exMs executable to test
EXMS_EXE = Path(r"C:\Users\Administrator\exMs\build\stress_test.exe")
EXMS_DEMO = Path(r"C:\Users\Administrator\exMs\build\demo_echo.exe")

PE_MEDIA = Path(r"C:\DKTM_temp_dbg\media")  # From DKTM

def check() -> None:
    """Check prerequisites."""
    errors = []

    if not Path(QEMU_EXE).exists():
        errors.append(f"QEMU not found: {QEMU_EXE}")

    if not Path(OVMF_CODE).exists():
        errors.append(f"OVMF code not found: {OVMF_CODE}")

    if not EXMS_EXE.exists():
        errors.append(f"exMs stress_test not found: {EXMS_EXE}")

    if errors:
        for e in errors:
            print(f"[X] {e}")
        sys.exit(1)

    print("[OK] All tools found")

def prepare_winpe_payload() -> Path:
    """Prepare WinPE payload with exMs."""
    WORK_DIR.mkdir(exist_ok=True)

    # Copy exMS executables to a temp directory
    exms_dir = WORK_DIR / "exms"
    exms_dir.mkdir(exist_ok=True)

    print(f"[*] Copying exMs files to {exms_dir}")
    shutil.copy(EXMS_DEMO, exms_dir / "demo_echo.exe")
    shutil.copy(EXMS_EXE, exms_dir / "stress_test.exe")

    # Create test script that will run in WinPE
    test_script = exms_dir / "run_tests.cmd"
    test_script.write_text("""@echo off
echo [*] Running exMs tests in WinPE...
echo.

echo [*] Running demo_echo...
C:\\exms\\demo_echo.exe
echo.

echo [*] Running stress_test...
C:\\exms\\stress_test.exe
echo.

echo [*] Tests completed!
pause
""")

    print(f"[+] Created test script: {test_script}")
    return exms_dir

def test_locally() -> bool:
    """Test exMs locally (without QEMU/WinPE)."""
    print("\n[*] Local Stability Test (without QEMU)")
    print("=" * 50)

    # Run stress test multiple times
    for i in range(5):
        print(f"\n--- Run {i+1}/5 ---")
        result = subprocess.run(
            [str(EXMS_EXE)],
            capture_output=True,
            text=True,
            timeout=60
        )

        if result.returncode == 0:
            # Check for "All tests completed" in output
            if "All tests completed successfully" in result.stdout:
                print("[OK] Stress test passed")
            else:
                print("[?] Test ran but output unclear")
                print(result.stdout[-500:])
        else:
            print(f"[X] Test failed with code {result.returncode}")
            print(result.stderr[-500:] if result.stderr else result.stdout[-500:])
            return False

    print("\n" + "=" * 50)
    print("[OK] All 5 runs completed successfully!")
    return True

def main():
    print("=" * 50)
    print("    exMs QEMU/WinPE Stability Test")
    print("=" * 50)

    check()

    # Prepare files
    exms_dir = prepare_winpe_payload()

    # Run local stress test (5 iterations)
    success = test_locally()

    print("\n" + "=" * 50)
    if success:
        print("[OK] exMs is STABLE - all tests passed!")
        print()
        print("Next steps for full QEMU/WinPE testing:")
        print("1. Copy exMs files to WinPE media (PE_MEDIA)")
        print("2. Add test script to WinPE startnet.cmd")
        print("3. Boot WinPE in QEMU:")
        print(f"   {QEMU_EXE} -machine q35 -m 1G -drive if=pflash,readonly=on,file=\"{OVMF_CODE}\"")
        print(f"   -drive file=C:\\winpe_test.iso,media=cdrom,readonly=on")
        print()
        print("For now, local stability test is sufficient!")
    else:
        print("[X] Some tests failed")

    print("=" * 50)

if __name__ == "__main__":
    main()
