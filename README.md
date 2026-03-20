# exMs Skeleton

exMs = 定制 XP/PE 外壳 + Linux 兼容子系统 + PDMF/PFC 运行时骨架。

这个骨架包的目标不是直接可运行，而是先把工程边界、模块职责、目录结构、包格式与接口占位定下来，
方便后续逐步填入：

- ELF loader
- Linux syscall dispatcher
- PDMF (分页动态映射框架)
- PFC (页面快速缓存 / 快路径调度层)
- exm 包格式与安装逻辑

## 目录总览

- `boot/`：启动与初始装载
- `core/`：核心模块
- `runtime/`：运行时对象层
- `compat/linux/`：Linux ABI/ELF/syscall 兼容层
- `pkg/`：`.exmpkg/.exmsys/.exmapp/.exmdll` 处理
- `apps/`：exMs 内部应用与演示程序
- `tools/`：开发、运行、打包脚本
- `tests/`：PDMF/PFC/ABI/ELF 测试
- `docs/`：架构与规范文档

## 建议开发顺序

1. `boot/exboot` 最小启动
2. `core/exloader` 最小装载器
3. `compat/linux/elf` 静态 ELF loader
4. `compat/linux/syscall` syscall dispatcher 雏形
5. `core/expdmf` 最小 page/mapping/process/event 对象宇宙
6. `core/expfc` fast/main 调度与 TTL/requeue
7. 跑通几个最小 Linux 风格程序
