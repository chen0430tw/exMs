# exMs QEMU 测试指南

## 概述

exMs 使用 QEMU + OVMF (UEFI) 在 WinPE 环境中进行稳定性测试。
正确方式是将 WinPE 文件打包成 **FAT32 VHD**，以 USB 设备挂载到 QEMU，
而非 ISO CD-ROM——后者在 QEMU 11.x 的 OVMF 上会触发 "Press any key to boot
from CD or DVD" 超时问题，导致无法自动进入 PE。

---

## 快速开始

```bash
# 需要管理员权限
python tools/test_qemu_vhd.py
```

第一次运行会：
1. 用 copype 构建 WinPE base（约 2-3 分钟）
2. 挂载 boot.wim，注入 exMs 可执行文件与 startnet.cmd
3. 用 diskpart 创建 700MB FAT32 VHD，复制 WinPE 文件
4. 以 USB 设备启动 QEMU，进入 exMs WinPE 稳定性测试

再次运行时若 media 已存在会跳过 copype，仅重建 VHD。

```bash
# 跳过 VHD 构建，直接用上次的 VHD 启动
python tools/test_qemu_vhd.py --run
```

---

## 前置需求

| 组件 | 路径 | 说明 |
|------|------|------|
| QEMU | `C:\Program Files\qemu\qemu-system-x86_64.exe` | 11.x+ |
| OVMF | `C:\Program Files\qemu\share\edk2-x86_64-code.fd` | 随 QEMU 附带 |
| Windows ADK | `C:\Program Files (x86)\Windows Kits\10\...` | 含 WinPE 附加组件 |
| exMs build | `C:\Users\Administrator\exMs\build\` | demo_echo.exe 等 |

---

## 工作原理

### 为什么用 VHD + USB，不用 ISO

| 方式 | 问题 |
|------|------|
| ISO (`media=cdrom,if=ide`) | OVMF 找到 bootx64.efi 后，Windows Boot Manager 显示 "Press any key to boot from CD or DVD"，无人按键则返回 EFI_TIMEOUT，QEMU 循环重启 |
| VHD (`usb-storage`) | OVMF 走 EFI fallback 直接执行 `\EFI\Boot\bootx64.efi`，无需按键，与实机 USB 启动行为一致 |

### QEMU 参数要点

```
-machine q35
-cpu Nehalem        # 比 qemu64 更接近真实 CPU，WinPE 兼容性更好
-m 2G               # boot.wim ramdisk ~400MB，需要 2GB
-device usb-ehci    # USB 控制器
-device usb-storage # VHD 作为 USB 存储设备
-no-reboot          # 防止 WinPE 异常时无限重启
```

### WinPE 启动流程

```
OVMF → Boot0001 (UEFI QEMU USB HARDDRIVE)
  → \EFI\Boot\bootx64.efi (Windows Boot Manager)
  → boot.wim (WinPE ramdisk)
  → startnet.cmd
      wpeinit
      X:\exms\demo_echo.exe
      X:\exms\stress_test.exe
      X:\exms\elf_test.exe
      wpeutil shutdown
```

---

## 文件说明

| 文件 | 说明 |
|------|------|
| `tools/test_qemu_vhd.py` | **主测试脚本**，VHD USB 方式（推荐） |
| `tools/test_qemu_exms.py` | ISO 方式（不推荐，QEMU 11.x 有兼容性问题） |
| `tools/test_qemu_dktm.py` | 复用 DKTM media 的 ISO 方式（同上） |
| `tools/test_qemu_simple.py` | 最简 ISO 方式（同上） |

---

## 参考

本方案参考 DKTM 项目的 `make_usb_vhd.py`，原理与实机通过 EFI BootNext
从 D: 硬盘启动 WinPE 一致。
