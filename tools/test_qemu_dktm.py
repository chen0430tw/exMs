#!/usr/bin/env python3
r"""
基于DKTM的test_qemu.py，添加exMs测试文件
"""

import sys
import os
import subprocess
import shutil
from pathlib import Path

if sys.stdout and hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

# ── 路径 ──────────────────────────────────────────────────────────────────────

QEMU_EXE   = r"C:\Program Files\qemu\qemu-system-x86_64.exe"
OVMF_CODE  = r"C:\Program Files\qemu\share\edk2-x86_64-code.fd"
OVMF_VARS_SIZE = 528 * 1024  # 4MB total - 3.5MB code = 528KB vars

WORK_DIR   = Path(r"C:\exMs_temp_qemu")
ISO_PATH   = WORK_DIR / "exMs_test.iso"
VARS_COPY  = WORK_DIR / "ovmf_vars.fd"
SERIAL_LOG = WORK_DIR / "serial.log"

# 使用DKTM的WinPE media
PE_MEDIA   = Path(r"C:\DKTM_temp_dbg\media")
EXMS_BUILD = Path(r"C:\Users\Administrator\exMs\build")

# exMs WinPE startnet.cmd
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

def check() -> None:
    errors = []
    if not Path(QEMU_EXE).exists():
        errors.append(f"QEMU 不存在: {QEMU_EXE}")
    if not Path(OVMF_CODE).exists():
        errors.append(f"OVMF code 不存在: {OVMF_CODE}")
    if not PE_MEDIA.exists():
        errors.append(f"WinPE media 不存在: {PE_MEDIA}")
    for exe in ["demo_echo.exe", "stress_test.exe", "elf_test.exe"]:
        if not (EXMS_BUILD / exe).exists():
            errors.append(f"exMs 可执行文件不存在: {EXMS_BUILD / exe}")
    if errors:
        for e in errors:
            print(f"[X] {e}")
        sys.exit(1)
    print("[OK] 所有工具均存在")

def prepare_vars() -> Path:
    WORK_DIR.mkdir(exist_ok=True)
    VARS_COPY.write_bytes(b"\x00" * OVMF_VARS_SIZE)
    print(f"[OK] 空白 OVMF vars ({OVMF_VARS_SIZE // 1024}KB) → {VARS_COPY}")
    return VARS_COPY

def inject_exms_and_build_iso() -> Path:
    """复制DKTM的WinPE media，注入exMs文件，构建ISO。"""
    # 清理旧挂载
    subprocess.run(["dism", "/Cleanup-Wim"], capture_output=True)
    mount_dir = WORK_DIR / "mount"
    if mount_dir.exists():
        subprocess.run(["dism", "/Unmount-Image", f"/MountDir:{mount_dir}", "/Discard"],
                      capture_output=True)

    WORK_DIR.mkdir(exist_ok=True)

    # 复制DKTM的media到工作目录
    media_dst = WORK_DIR / "media"
    if media_dst.exists():
        shutil.rmtree(media_dst)
    shutil.copytree(PE_MEDIA, media_dst)
    print(f"[*] 复制 WinPE media → {media_dst}")

    # 挂载boot.wim
    mount_dir = WORK_DIR / "mount"
    mount_dir.mkdir(exist_ok=True)

    print("[*] 挂载 boot.wim...")
    wim_path = media_dst / "sources" / "boot.wim"
    result = subprocess.run(["dism", "/Mount-Image",
                           f"/ImageFile:{wim_path}",
                           "/Index:1",
                           f"/MountDir:{mount_dir}"],
                          capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[!] DISM 错误: {result.stderr}")
        raise RuntimeError("挂载 boot.wim 失败")
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

    # 卸载并提交
    print("[*] 卸载 boot.wim...")
    result = subprocess.run(["dism", "/Unmount-Image",
                           f"/MountDir:{mount_dir}",
                           "/Commit"],
                          capture_output=True)
    if result.returncode != 0:
        raise RuntimeError("卸载失败")
    print("    ✓ boot.wim 已卸载")

    # 用 MakeWinPEMedia 创建 ISO
    print("[*] 创建 ISO...")
    make_pe_media = (r"C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit"
                     r"\Windows Preinstallation Environment\MakeWinPEMedia.cmd")

    if ISO_PATH.exists():
        ISO_PATH.unlink()

    adk_root = r"C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit"
    env = os.environ.copy()
    env["WinPERoot"]   = adk_root + r"\Windows Preinstallation Environment"
    env["OSCDImgRoot"] = adk_root + r"\Deployment Tools\amd64\Oscdimg"
    env["DISMRoot"]    = adk_root + r"\Deployment Tools\amd64\DISM"
    env["PATH"] = adk_root + r"\Deployment Tools\amd64\Oscdimg;" + env.get("PATH", "")

    cmd = ["cmd", "/c", make_pe_media, "/ISO", "/f", str(WORK_DIR), str(ISO_PATH)]
    print(f"    cmd: {' '.join(cmd)}")

    result = subprocess.run(cmd, capture_output=True, text=True,
                          encoding="utf-8", errors="replace", env=env)

    if not ISO_PATH.exists():
        print(f"[!] MakeWinPEMedia 输出:")
        print(result.stdout + result.stderr)
        raise RuntimeError(f"MakeWinPEMedia 失败")

    sz = ISO_PATH.stat().st_size // (1024 * 1024)
    print(f"    ✓ ISO: {ISO_PATH} ({sz} MB)")
    return ISO_PATH

def build_cmd(iso: Path, vars_fd: Path) -> list:
    return [
        QEMU_EXE,
        "-machine", "q35",
        "-accel",   "whpx",
        "-cpu",     "host",
        "-m",       "1G",
        "-smp",     "2",
        "-drive",   f"if=pflash,format=raw,readonly=on,file={OVMF_CODE}",
        "-drive",   f"if=pflash,format=raw,file={vars_fd}",
        "-drive",   f"file={iso},media=cdrom,readonly=on,if=ide",
        "-vga",     "std",
        "-chardev", f"file,id=ser0,path={SERIAL_LOG}",
        "-serial",  "chardev:ser0",
    ]

def main():
    dry = "--dry" in sys.argv

    print("=" * 60)
    print("    exMs QEMU/WinPE Test (基于DKTM)")
    print("=" * 60)

    check()

    print("[*] 准备 OVMF vars...")
    vars_fd = prepare_vars()

    print("[*] 构建 exMs WinPE ISO...")
    if dry:
        iso = ISO_PATH
        print("    (dry mode)")
    else:
        iso = inject_exms_and_build_iso()

    cmd = build_cmd(iso, vars_fd)

    print()
    print("[*] QEMU 命令:")
    print("    " + "\n    ".join(cmd))
    print()

    if dry:
        print("[dry] 不执行")
        return

    if SERIAL_LOG.exists():
        SERIAL_LOG.unlink()

    print("[*] 启动 QEMU...")
    print(f"    串口日志: {SERIAL_LOG}")
    print()

    try:
        subprocess.run(cmd)
    except KeyboardInterrupt:
        print("\n[*] QEMU 已退出")

    if SERIAL_LOG.exists() and SERIAL_LOG.stat().st_size > 0:
        print(f"\n[串口日志] {SERIAL_LOG}:")
        print("─" * 60)
        content = SERIAL_LOG.read_text(encoding="utf-8", errors="replace")
        print(content[-4000:] if len(content) > 4000 else content)
    else:
        print(f"\n[!] 串口日志为空")

if __name__ == "__main__":
    main()
