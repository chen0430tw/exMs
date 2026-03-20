# Claude 接手说明

## 这份骨架的定位

这不是成品系统，而是 **exMs 的 Claude-ready 骨架**。目标是让后续开发者能在最短时间内看懂：

- exMs 是什么
- 当前哪些模块已经有最小实现
- 下一步先填哪里最划算
- 哪些东西现在不要乱扩张

## exMs 一句话定义

**exMs = NT/PE 宿主上的 Linux 兼容子系统骨架，底层运行时用 PDMF 统一对象与映射，用 PFC 做 fast/main 快慢路径调度。**

---

## 当前已经填好的模块

### 1. `compat/linux/elf`
已有：
- ELF64 little-endian 头部解析
- `PT_LOAD` program header 扫描
- entry、machine、type 等元信息读取

没有：
- 真正的内存映射装载
- 重定位
- 初始栈构造
- 跳转执行

### 2. `compat/linux/syscall`
已有：
- syscall number -> route 分流
- 初步分到：
  - `exabi.fs`
  - `expdmf.mem`
  - `expdmf.proc`
  - `expfc.ready`

没有：
- 真正 syscall 语义实现
- errno 映射
- fd table
- host I/O bridge

### 3. `core/expdmf`
已有：
- process/page/mapping 最小对象
- `fork_process()`
- `create_shared_page()`
- `map_page()`

没有：
- 真实对象 store
- page 权限位
- observe/event 对象
- COW fault 细节
- fd/event 统一对象层

### 4. `core/expfc`
已有：
- fast/main 双队列
- strict fast-first
- TTL expire
- cooldown
- rescue interval/quota

没有：
- ready object 接入
- metrics collector
- queue contamination tracing
- sweet spot sweep interface

---

## 接下来最优先的开发顺序

### Phase A：先把 ELF 从“解析器”推进到“最小装载器”
原因：
- Linux 程序先得“进门”
- 没有最小装载器，后面的 syscall/runtime 只能空转
- Claude 后续更容易围绕“可装载程序”继续填实现

### Phase B：再把 syscall dispatcher 接到最小 fd/mmap/epoll 语义
先别贪多，只做最小闭环：
- `read`
- `write`
- `openat`
- `close`
- `mmap`
- `munmap`
- `brk`
- `epoll_create1`
- `epoll_ctl`
- `epoll_wait`
- `exit`

### Phase C：最后再扩 `expdmf` / `expfc` 的真实运行时细节

---

## 当前最重要的原则

1. **先静态 ELF，后动态 .so**
2. **先最小 syscall 闭环，后大而全 ABI**
3. **PDMF 是底板，PFC 是增强层**
4. **不要一开始把 exMs 做成胖操作系统**
5. **热路径短，冷路径厚**

---

## Claude 不应该先做什么

不要优先做这些：
- GUI 前端
- 包管理 UI
- 完整 `/proc`
- 完整 signal 子系统
- 全功能动态链接器
- 大量配置系统
- “看起来很完整”的空壳目录扩张

---

## 建议 Claude 第一刀直接改哪里

### 第一推荐
`compat/linux/elf/elf_loader.cpp`

目标：
- 从“能读 header”推进到“能构造最小 image layout”
- 输出：
  - image base
  - load segments
  - entry
  - memory span plan

### 第二推荐
`compat/linux/syscall/dispatcher.cpp`

目标：
- 把 route-only 升级成最小 mock implementation
- 先返回结构化结果，而不是只有 `ENOSYS`

---

## 验收标准（短版）

### ELF 层
- 能正确识别 ELF64 little-endian
- 能枚举 `PT_LOAD`
- 能构造最小 load plan

### syscall 层
- 最小 syscall 集有 route + mock 结果
- 不再全是统一占位返回

### PDMF 层
- `fork` 仍保持 mapping clone 语义
- `shm` 仍保持 shared mapping 语义

### PFC 层
- strict fast-first 不被破坏
- main 不可长期挂死
- TTL / cooldown 逻辑还在

---

## 最后一句

**先让 Linux 程序“进门”，再让它“说话”，最后再让它“跑快”。**
