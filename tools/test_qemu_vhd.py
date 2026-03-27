#!/usr/bin/env python3
r"""
test_qemu_vhd.py — 用 VHD USB 方式在 QEMU 里测试 exMs WinPE

参考 DKTM 的 make_usb_vhd.py，用 GPT+FAT32 VHD 挂载为 USB 设备，
OVMF 直接走 EFI fallback 找 \EFI\Boot\bootx64.efi，不走 ISO CD 路径。

用法：
  python tools/test_qemu_vhd.py        # 建 VHD + 启动 QEMU
  python tools/test_qemu_vhd.py --run  # 跳过建 VHD，直接启动
"""

import sys, os, subprocess, shutil, time, ctypes
from pathlib import Path

if sys.stdout and hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

WORK_DIR   = Path(r"C:\exMs_temp_qemu")
VHD_PATH   = WORK_DIR / "exms_usb.vhd"
VHD_SIZE   = 700   # MB
PE_MEDIA   = WORK_DIR / "media"
EXMS_BUILD = Path(r"C:\Users\Administrator\exMs\build")
QEMU_EXE   = r"C:\Program Files\qemu\qemu-system-x86_64.exe"
OVMF_CODE  = r"C:\Program Files\qemu\share\edk2-x86_64-code.fd"
VARS_COPY  = WORK_DIR / "ovmf_vars.fd"
SERIAL_LOG = WORK_DIR / "serial.log"
OVMF_VARS_SIZE = 512 * 1024

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
echo Shutting down in 30 seconds...
ping -n 31 127.0.0.1 > nul
wpeutil shutdown
"""


def run(cmd, **kw):
    print(f"    > {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    return subprocess.run(cmd, **kw)


def build_winpe():
    """用 copype + DISM 建 WinPE media（若 media 已存在则跳过）。"""
    if PE_MEDIA.exists() and (PE_MEDIA / "sources" / "boot.wim").exists():
        print(f"[*] WinPE media 已存在，跳过 copype")
        return

    copype = (r"C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit"
              r"\Windows Preinstallation Environment\copype.cmd")
    adk_root = r"C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit"
    env = os.environ.copy()
    env["WinPERoot"]   = adk_root + r"\Windows Preinstallation Environment"
    env["OSCDImgRoot"] = adk_root + r"\Deployment Tools\amd64\Oscdimg"
    env["DISMRoot"]    = adk_root + r"\Deployment Tools\amd64\DISM"

    subprocess.run(["dism", "/Cleanup-Wim"], capture_output=True)
    mount_dir = WORK_DIR / "mount"
    if mount_dir.exists():
        subprocess.run(["dism", "/Unmount-Image", f"/MountDir:{mount_dir}", "/Discard"],
                       capture_output=True)
        shutil.rmtree(mount_dir, ignore_errors=True)
    if WORK_DIR.exists():
        shutil.rmtree(WORK_DIR, ignore_errors=True)

    print("[*] 运行 copype...")
    r = subprocess.run(["cmd", "/c", copype, "amd64", str(WORK_DIR)],
                       capture_output=True, text=True, env=env)
    if r.returncode != 0:
        raise RuntimeError(f"copype 失败: {r.stderr}")
    print("    ✓ copype 完成")

    mount_dir = WORK_DIR / "mount"
    mount_dir.mkdir(exist_ok=True)

    print("[*] 挂载 boot.wim...")
    wim = PE_MEDIA / "sources" / "boot.wim"
    r = subprocess.run(["dism", "/Mount-Image", f"/ImageFile:{wim}",
                        "/Index:1", f"/MountDir:{mount_dir}"],
                       capture_output=True)
    if r.returncode != 0:
        raise RuntimeError("挂载失败")

    print("[*] 注入 exMs 文件...")
    exms_dir = mount_dir / "exms"
    exms_dir.mkdir(exist_ok=True)
    for exe in ["demo_echo.exe", "stress_test.exe", "elf_test.exe"]:
        src = EXMS_BUILD / exe
        if src.exists():
            shutil.copy(src, exms_dir / exe)
            print(f"    ✓ {exe}")

    print("[*] 写入 startnet.cmd...")
    (mount_dir / "Windows" / "System32" / "startnet.cmd").write_text(
        EXMS_STARTNET, encoding="ascii")

    print("[*] 卸载 boot.wim...")
    r = subprocess.run(["dism", "/Unmount-Image", f"/MountDir:{mount_dir}", "/Commit"],
                       capture_output=True)
    if r.returncode != 0:
        raise RuntimeError("卸载失败")
    print("    ✓ WinPE 构建完成")


def create_vhd():
    """用 diskpart 建 GPT+FAT32 VHD。"""
    WORK_DIR.mkdir(exist_ok=True)

    if VHD_PATH.exists():
        VHD_PATH.unlink()

    script = (f'create vdisk file="{VHD_PATH}" maximum={VHD_SIZE} type=fixed\n'
              f'select vdisk file="{VHD_PATH}"\n'
              f'attach vdisk\n'
              f'create partition primary\n'
              f'format fs=fat32 quick label=EXMS_PE\n'
              f'exit\n')
    script_path = WORK_DIR / "dp_create.txt"
    script_path.write_text(script, encoding="utf-8")

    print("[*] 创建 VHD (diskpart)...")
    r = run(["diskpart", "/s", str(script_path)],
            capture_output=True, text=True, encoding="utf-8", errors="replace")
    if r.returncode != 0:
        raise RuntimeError(f"diskpart 失败: {r.stderr}")
    print(r.stdout[-500:] if r.stdout else "")

    time.sleep(2)
    print("[*] 获取卷 GUID 路径...")
    r2 = subprocess.run(
        ["powershell", "-NoProfile", "-Command",
         "Get-Volume | Where-Object { $_.FileSystemLabel -eq 'EXMS_PE' } "
         "| Select-Object -ExpandProperty Path"],
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    vol_path = r2.stdout.strip()
    if not vol_path:
        raise RuntimeError("找不到 EXMS_PE 卷")
    print(f"    ✓ 卷路径: {vol_path}")
    return vol_path


def copy_winpe(vol_path: str):
    """把 WinPE media 复制到 VHD。"""
    print(f"[*] 复制 WinPE 文件到 VHD...")
    root = Path(vol_path)
    for t in ["EFI", "Boot", "sources", "bootmgr", "bootmgr.efi"]:
        src = PE_MEDIA / t
        dst = root / t
        if not src.exists():
            continue
        if src.is_dir():
            if dst.exists():
                shutil.rmtree(dst)
            shutil.copytree(src, dst)
        else:
            shutil.copy2(src, dst)
        print(f"    ✓ {t}")

    for f in ["EFI/Boot/bootx64.efi", "EFI/Microsoft/Boot/BCD",
              "sources/boot.wim", "Boot/boot.sdi"]:
        p = root / f
        print(f"    {'✓' if p.exists() else '✗ 缺失'} {f}")


def detach_vhd():
    script_path = WORK_DIR / "dp_detach.txt"
    script_path.write_text(
        f'select vdisk file="{VHD_PATH}"\ndetach vdisk\nexit\n',
        encoding="utf-8")
    print("[*] 卸载 VHD...")
    run(["diskpart", "/s", str(script_path)],
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    print("    ✓ VHD 已卸载")


def prepare_vars():
    WORK_DIR.mkdir(exist_ok=True)
    VARS_COPY.write_bytes(b"\x00" * OVMF_VARS_SIZE)
    print(f"    ✓ OVMF vars ({OVMF_VARS_SIZE//1024}KB)")
    return VARS_COPY


def run_qemu():
    global SERIAL_LOG
    try:
        if SERIAL_LOG.exists():
            SERIAL_LOG.unlink()
    except PermissionError:
        SERIAL_LOG = WORK_DIR / f"serial_{int(time.time())}.log"

    cmd = [
        QEMU_EXE,
        "-machine", "q35",
        "-cpu",     "Nehalem",
        "-m",       "2G",
        "-smp",     "2",
        "-drive",   f"if=pflash,format=raw,readonly=on,file={OVMF_CODE}",
        "-drive",   f"if=pflash,format=raw,file={VARS_COPY}",
        "-drive",   f"file={VHD_PATH},format=vpc,if=none,id=usbdisk",
        "-device",  "usb-ehci,id=ehci",
        "-device",  "usb-storage,bus=ehci.0,drive=usbdisk",
        "-vga",     "std",
        "-chardev", f"file,id=ser0,path={SERIAL_LOG}",
        "-serial",  "chardev:ser0",
        "-no-reboot",
    ]

    print("\n[*] 启动 QEMU (USB VHD 模式, 2GB RAM, Nehalem)...")
    print(f"    串口日志: {SERIAL_LOG}\n")
    try:
        subprocess.run(cmd)
    except KeyboardInterrupt:
        print("\n[*] QEMU 已退出")

    if SERIAL_LOG.exists() and SERIAL_LOG.stat().st_size > 0:
        content = SERIAL_LOG.read_text(encoding="utf-8", errors="replace")
        print(f"\n[串口日志]\n{'─'*60}")
        print(content[-3000:] if len(content) > 3000 else content)
    else:
        print(f"\n[!] 串口日志为空")


def main():
    run_only = "--run" in sys.argv

    if sys.platform != "win32":
        print("仅 Windows"); sys.exit(1)
    if not ctypes.windll.shell32.IsUserAnAdmin():
        print("需要管理员权限"); sys.exit(1)

    if not run_only:
        build_winpe()
        vol_path = create_vhd()
        copy_winpe(vol_path)
        detach_vhd()
        print(f"\n[✓] VHD 已创建: {VHD_PATH}")

    prepare_vars()
    run_qemu()


if __name__ == "__main__":
    main()
