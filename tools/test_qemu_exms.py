#!/usr/bin/env python3
r"""
test_qemu_exms.py — 用 QEMU + WinPE 测试 exMs 稳定性

基于 DKTM 的 test_qemu.py 方法：
1. 用 copype 创建 WinPE base
2. 挂载 boot.wim，添加 exMs 测试文件
3. 用 MakeWinPEMedia 创建 ISO
4. 启动 QEMU 测试

用法：
  python tools/test_qemu_exms.py
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

# exMs build 目录
EXMS_BUILD = Path(r"C:\Users\Administrator\exMs\build")

# exMs WinPE startnet.cmd 内容
EXMS_STARTNET = r"""@echo off
title exMs WinPE Stability Test
color 0B

echo ========================================
echo   exMs WinPE Stability Test
echo ========================================
echo.

wpeinit

echo [*] Waiting for drive initialization...
ping -n 3 127.0.0.1 > nul

echo.
echo [*] Running exMs tests...
echo.

echo [1/3] Running demo_echo.exe...
if exist X:\exms\demo_echo.exe (
    X:\exms\demo_echo.exe
) else (
    echo [!] demo_echo.exe not found
)
echo.

echo [2/3] Running stress_test.exe...
if exist X:\exms\stress_test.exe (
    X:\exms\stress_test.exe
) else (
    echo [!] stress_test.exe not found
)
echo.

echo [3/3] Running elf_test.exe...
if exist X:\exms\elf_test.exe (
    X:\exms\elf_test.exe
) else (
    echo [!] elf_test.exe not found
)
echo.

echo ========================================
echo   exMs tests completed!
echo ========================================
echo.
echo Pausing for 30 seconds...
ping -n 31 127.0.0.1 > nul
echo Shutting down...
wpeutil shutdown
"""

# ── 检查 ──────────────────────────────────────────────────────────────────────

def check() -> None:
    errors = []
    if not Path(QEMU_EXE).exists():
        errors.append(f"QEMU 不存在: {QEMU_EXE}")
    if not Path(OVMF_CODE).exists():
        errors.append(f"OVMF code 不存在: {OVMF_CODE}")

    # 检查 exMs 可执行文件
    for exe in ["demo_echo.exe", "stress_test.exe", "elf_test.exe"]:
        if not (EXMS_BUILD / exe).exists():
            errors.append(f"exMs 可执行文件不存在: {EXMS_BUILD / exe}")

    if errors:
        for e in errors:
            print(f"[X] {e}")
        sys.exit(1)
    print("[OK] 所有工具均存在")

# ── 构建 WinPE ─────────────────────────────────────────────────────────────────

def build_winpe_with_exms() -> Path:
    """构建包含 exMs 的 WinPE ISO。"""
    WORK_DIR.mkdir(exist_ok=True)

    copype = (r"C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit"
              r"\Windows Preinstallation Environment\copype.cmd")
    if not Path(copype).exists():
        raise RuntimeError(f"copype.cmd 不存在: {copype}")

    # 保留 vars 文件
    vars_backup = None
    if VARS_COPY.exists():
        vars_backup = VARS_COPY.read_bytes()

    # 清理旧构建
    if WORK_DIR.exists():
        print(f"[*] 清理 {WORK_DIR}")
        # 先清理 DISM 挂载
        subprocess.run(["dism", "/Cleanup-Wim"], capture_output=True)
        mount_dir = WORK_DIR / "mount"
        if mount_dir.exists():
            subprocess.run(["dism", "/Unmount-Image",
                          f"/MountDir:{mount_dir}", "/Discard"],
                          capture_output=True)
        shutil.rmtree(WORK_DIR, ignore_errors=True)

    # 运行 copype（会创建 WORK_DIR）
    print("[*] 运行 copype...")
    adk_root = r"C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit"
    env = os.environ.copy()
    env["WinPERoot"]   = adk_root + r"\Windows Preinstallation Environment"
    env["OSCDImgRoot"] = adk_root + r"\Deployment Tools\amd64\Oscdimg"
    env["DISMRoot"]    = adk_root + r"\Deployment Tools\amd64\DISM"

    result = subprocess.run(["cmd", "/c", copype, "amd64", str(WORK_DIR)],
                           capture_output=True, text=True, env=env)
    if result.returncode != 0:
        raise RuntimeError(f"copype 失败: {result.stderr}")

    # 恢复 vars 文件
    if vars_backup:
        VARS_COPY.write_bytes(vars_backup)
    print("    ✓ copype 完成")

    # 创建挂载点
    mount_dir = WORK_DIR / "mount"
    mount_dir.mkdir(exist_ok=True)

    # 挂载 boot.wim
    print("[*] 挂载 boot.wim...")
    wim_path = WORK_DIR / "media" / "sources" / "boot.wim"
    result = subprocess.run(["dism", "/Mount-Image",
                           f"/ImageFile:{wim_path}",
                           "/Index:1",
                           f"/MountDir:{mount_dir}"],
                          capture_output=True)
    if result.returncode != 0:
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

    env["PATH"] = adk_root + r"\Deployment Tools\amd64\Oscdimg;" + env.get("PATH", "")

    result = subprocess.run(["cmd", "/c", make_pe_media, "/ISO", "/f",
                           str(WORK_DIR), str(ISO_PATH)],
                          capture_output=True, text=True,
                          encoding="utf-8", errors="replace", env=env)

    if not ISO_PATH.exists():
        raise RuntimeError(f"MakeWinPEMedia 失败: {result.stderr}")

    sz = ISO_PATH.stat().st_size // (1024 * 1024)
    print(f"    ✓ ISO: {ISO_PATH} ({sz} MB)")

    return ISO_PATH

# ── 准备 OVMF vars ────────────────────────────────────────────────────────────

def prepare_vars() -> Path:
    WORK_DIR.mkdir(exist_ok=True)
    VARS_COPY.write_bytes(b"\x00" * OVMF_VARS_SIZE)
    print(f"[OK] 空白 OVMF vars ({OVMF_VARS_SIZE // 1024}KB) → {VARS_COPY}")
    return VARS_COPY

# ── 构造 QEMU 命令 ────────────────────────────────────────────────────────────

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
        "-drive",   f"file={iso},media=cdrom,readonly=on,if=ide",  # WinPE ISO → IDE CDROM
        "-vga",     "std",
        "-chardev", f"file,id=ser0,path={SERIAL_LOG}",
        "-serial",  "chardev:ser0",
    ]

# ── 主流程 ────────────────────────────────────────────────────────────────────

def main():
    dry = "--dry" in sys.argv

    print("=" * 60)
    print("    exMs QEMU/WinPE Stability Test")
    print("=" * 60)

    check()

    print("[*] 准备 OVMF vars...")
    vars_fd = prepare_vars()

    print("[*] 构建 exMs WinPE ISO...")
    if dry:
        print("    (dry mode)")
        iso = ISO_PATH
    else:
        iso = build_winpe_with_exms()

    cmd = build_cmd(iso, vars_fd)

    print()
    print("[*] QEMU 命令:")
    print("    " + "\n    ".join(cmd))
    print()

    if dry:
        print("[dry] 不执行")
        return

    # 清空旧串口日志
    if SERIAL_LOG.exists():
        SERIAL_LOG.unlink()

    print("[*] 启动 QEMU...")
    print("    WinPE 会自动运行 exMs 测试，约 60 秒后重启")
    print(f"    串口日志: {SERIAL_LOG}")
    print()

    try:
        subprocess.run(cmd)
    except KeyboardInterrupt:
        print("\n[*] QEMU 已退出")

    # 读串口日志
    if SERIAL_LOG.exists() and SERIAL_LOG.stat().st_size > 0:
        print(f"\n[串口日志] {SERIAL_LOG}:")
        print("─" * 60)
        content = SERIAL_LOG.read_text(encoding="utf-8", errors="replace")
        print(content[-4000:] if len(content) > 4000 else content)
    else:
        print(f"\n[!] 串口日志为空")

if __name__ == "__main__":
    main()
