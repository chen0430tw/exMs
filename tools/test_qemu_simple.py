#!/usr/bin/env python3
"""
直接在DKTM的WinPE上添加exMs，然后启动QEMU
"""

import sys
import subprocess
import shutil
from pathlib import Path

if sys.stdout and hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

QEMU_EXE   = r"C:\Program Files\qemu\qemu-system-x86_64.exe"
OVMF_CODE  = r"C:\Program Files\qemu\share\edk2-x86_64-code.fd"

WORK_DIR   = Path(r"C:\exMs_temp_qemu")
VARS_COPY  = WORK_DIR / "ovmf_vars.fd"
SERIAL_LOG = WORK_DIR / "serial.log"

PE_MEDIA   = Path(r"C:\DKTM_temp_dbg\media")
EXMS_BUILD = Path(r"C:\Users\Administrator\exMs\build")

EXMS_STARTNET = r"""@echo off
title exMs WinPE Test
color 0B
echo ========================================
echo   exMs WinPE Stability Test
echo ========================================
echo.
wpeinit
echo.
echo [*] Running exMs tests...
echo.
echo [1/3] demo_echo.exe
X:\exms\demo_echo.exe
echo.
echo [2/3] stress_test.exe
X:\exms\stress_test.exe
echo.
echo [3/3] elf_test.exe
X:\exms\elf_test.exe
echo.
echo ========================================
echo   Tests done! Shutting down in 30s...
echo ========================================
ping -n 31 127.0.0.1 > nul
wpeutil shutdown
"""

print("=" * 60)
print("    exMs QEMU/WinPE Test")
print("=" * 60)

# 清理旧挂载
print("[*] 清理 DISM...")
subprocess.run(["dism", "/Cleanup-Wim"], capture_output=True)

mount_dir = WORK_DIR / "mount"
if mount_dir.exists():
    subprocess.run(["dism", "/Unmount-Image", f"/MountDir:{mount_dir}", "/Discard"],
                  capture_output=True)
    shutil.rmtree(mount_dir, ignore_errors=True)

WORK_DIR.mkdir(exist_ok=True)

# 准备vars
VARS_COPY.write_bytes(b"\x00" * (528 * 1024))  # 4MB total - 3.5MB code = 528KB vars
print(f"[OK] OVMF vars 准备完成")

# 挂载DKTM的boot.wim
print("[*] 挂载 DKTM boot.wim...")
mount_dir.mkdir(exist_ok=True)
wim_path = PE_MEDIA / "sources" / "boot.wim"

result = subprocess.run(["dism", "/Mount-Image",
                       f"/ImageFile:{wim_path}",
                       "/Index:1",
                       f"/MountDir:{mount_dir}"],
                      capture_output=True, text=True)

if result.returncode != 0:
    print(f"[!] 挂载失败: {result.stderr}")
    sys.exit(1)

print("    ✓ boot.wim 已挂载")

# 复制 exMs 文件
print("[*] 复制 exMs 文件...")
exms_dir = mount_dir / "exms"
exms_dir.mkdir(exist_ok=True)

for exe in ["demo_echo.exe", "stress_test.exe", "elf_test.exe"]:
    src = EXMS_BUILD / exe
    if src.exists():
        shutil.copy(src, exms_dir / exe)
        print(f"    ✓ {exe}")

# 写入 startnet.cmd
print("[*] 写入 startnet.cmd...")
startnet = mount_dir / "Windows" / "System32" / "startnet.cmd"
startnet.write_text(EXMS_STARTNET, encoding="ascii")
print("    ✓ startnet.cmd 已写入")

# 卸载
print("[*] 卸载 boot.wim...")
result = subprocess.run(["dism", "/Unmount-Image",
                       f"/MountDir:{mount_dir}",
                       "/Commit"],
                      capture_output=True)

if result.returncode != 0:
    print(f"[!] 卸载失败: {result.stderr}")
    sys.exit(1)

print("    ✓ boot.wim 已卸载")

# 用DKTM方式构建ISO
print("[*] 创建 ISO...")
make_pe_media = (r"C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit"
                 r"\Windows Preinstallation Environment\MakeWinPEMedia.cmd")

iso_path = WORK_DIR / "exMs_test.iso"
if iso_path.exists():
    iso_path.unlink()

import os
adk_root = r"C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit"
env = os.environ.copy()
env["WinPERoot"]   = adk_root + r"\Windows Preinstallation Environment"
env["OSCDImgRoot"] = adk_root + r"\Deployment Tools\amd64\Oscdimg"
env["DISMRoot"]    = adk_root + r"\Deployment Tools\amd64\DISM"
env["PATH"] = adk_root + r"\Deployment Tools\amd64\Oscdimg;" + env.get("PATH", "")

# PE_MEDIA 的父目录
pe_working_dir = PE_MEDIA.parent  # C:\DKTM_temp_dbg

cmd = ["cmd", "/c", make_pe_media, "/ISO", "/f", str(pe_working_dir), str(iso_path)]
print(f"    cmd: {' '.join(cmd)}")

result = subprocess.run(cmd, capture_output=True, text=True,
                      encoding="utf-8", errors="replace", env=env)

if not iso_path.exists():
    print(f"[!] MakeWinPEMedia 失败")
    print(result.stdout + result.stderr)
    sys.exit(1)

sz = iso_path.stat().st_size // (1024 * 1024)
print(f"    ✓ ISO: {iso_path} ({sz} MB)")

# QEMU命令
print()
print("[*] 启动 QEMU...")
cmd = [
    QEMU_EXE,
    "-machine", "q35",
    "-accel", "whpx",
    "-cpu", "host",
    "-m", "1G",
    "-smp", "2",
    "-drive", f"if=pflash,format=raw,readonly=on,file={OVMF_CODE}",
    "-drive", f"if=pflash,format=raw,file={VARS_COPY}",
    "-drive", f"file={iso_path},media=cdrom,readonly=on,if=ide",
    "-vga", "std",
    "-chardev", f"file,id=ser0,path={SERIAL_LOG}",
    "-serial", "chardev:ser0",
]

print("    " + " ".join(cmd))
print()

try:
    subprocess.run(cmd)
except KeyboardInterrupt:
    print("\n[*] QEMU 已退出")
