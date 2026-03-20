# Claude 下一步任务单

## 任务 1：补最小 ELF load plan

目标文件：
- `compat/linux/elf/elf_loader.hpp`
- `compat/linux/elf/elf_loader.cpp`

新增建议：
- `ElfSegmentPlan`
- `ElfImagePlan`
- `build_image_plan(const std::string& path)`

输出至少包含：
- entry
- loadable segments
- image low/high span
- per-segment permissions / sizes

---

## 任务 2：补 syscall 最小 mock 层

目标文件：
- `compat/linux/syscall/dispatcher.hpp`
- `compat/linux/syscall/dispatcher.cpp`

建议：
- 增加 `SyscallRoute` enum
- 增加结构化字段：
  - `errno_value`
  - `implemented`
  - `route`
  - `note`

最小优先 syscall：
- openat
- close
- read
- write
- mmap
- munmap
- brk
- epoll_create1
- epoll_ctl
- epoll_wait
- exit

---

## 任务 3：补 smoke test

目标：
- 让 `apps/demo_echo` 至少能打印：
  - ELF parse/load plan 摘要
  - syscall dispatch 摘要
  - PDMF/PFC 摘要

---

## 明确禁止

不要先做：
- 完整 glibc 兼容
- 动态链接 `.so`
- GUI
- 完整 procfs
- 大量宏大配置系统
