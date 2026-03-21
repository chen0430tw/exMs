# exMs 技术白皮书

## 摘要

**exMs（ELF-cross-MS）** 是一种面向 **Windows NT 宿主环境** 的概念性执行架构，其目标不是在 Windows 中复制一套完整 Linux 系统，也不是仅仅提供一个 Unix 风格工具壳，而是尝试建立一条更直接的执行路径：

\[
\text{ELF Program} \rightarrow \text{exMs Runtime} \rightarrow \text{NT Host Substrate}
\]

在这条路径中，Windows NT 被视为**承载层**，ELF 程序被视为**可直接召唤的执行实体**，中间由 exMs runtime 负责完成：

- ELF 装载
- Linux ABI / syscall 分发
- 统一对象与映射语义
- 高价值快路径调度
- 包格式与组件装载

因此，exMs 的技术定位不是“Linux 终端 for Windows”，而是**在 NT/PE 上构造一套 ELF-first 的异构执行骨架**。

---

## 1. 背景与问题定义

### 1.1 传统路线的局限

当今在 Windows 上承接 Linux 程序，常见路线主要有四类：

1. **完整虚拟化路线**  
   直接运行完整 Linux 内核与用户空间，隔离强，但路径长、层级厚、运行代价较重。

2. **API/环境兼容路线**  
   通过兼容层重建一部分 POSIX/Unix 环境，让程序在 Windows 上可编译或可部分运行。

3. **内核兼容实现路线**  
   由宿主内核侧直接补 Linux 兼容语义，使 Linux user-mode 程序在 NT 上成立。

4. **翻译或重构路线**  
   把源程序改造成更符合 Win32/NT 语义的程序，再在 Windows 上运行。

这些路线各有价值，但也各有代价。  
问题并不只是“程序能不能跑”，而在于：

- ELF 身份能否被保留
- Linux 程序是否必须先被装进一个完整客体系统
- Windows 是否只能作为“不得不绕过的另一种世界观”
- 运行时语义是否能统一到一套更干净的对象骨架里

### 1.2 exMs 的问题意识

exMs 的出发点是：

> **不要先把 Linux 程序当成必须被包裹的客体，而是先把它看成可直接进入执行路径的实体。**

因此 exMs 试图把问题改写成：

- Windows NT：作为**宿主承载层**
- ELF：作为**程序本体**
- exMs Runtime：作为**语义桥与对象骨架**

也就是说，exMs 并不优先关心“终端像不像 Linux”，而是优先关心：

- ELF 能否被直接装载
- Linux syscall 能否被接住
- `fork / shm / epoll` 能否在统一对象层里成立
- 高价值路径能否被保护

---

## 2. exMs 的定义与命名

## 2.1 exMs 的定义

**exMs（ELF-cross-MS）** 是一种以 **NT/PE 为承载底座**、以 **ELF 程序直接装载与 Linux 语义重建** 为核心目标的执行架构。

它的目标不是在 Windows 内复制一套完整 Linux 系统，而是通过：

- ELF loader
- Linux ABI dispatcher
- 统一运行时对象层
- 快慢路径调度层

把 ELF 程序拉近到 NT 的执行面前。

### 2.2 M 为什么大写

`exMs` 中的 **M** 必须大写，因为它不是普通字符串中的随手字母，而是整个名称结构中的第二主轴：

\[
\text{exMs} = \text{ELF-cross-MS}
\]

其中：

- `ex`：来自 ELF / execution / cross-execution 这一侧
- `Ms`：来自 Microsoft / MS / Windows NT 宿主侧

大写 `M` 的原因有三点：

1. **显式保留 MS 语义**  
   如果写成 `exms`，`MS` 宿主来源会被吞没；写成 `exMs`，可以直观看到这是一个跨向 MS/NT 的执行架构。

2. **保持双极结构**  
   `exMs` 的视觉结构天然包含两极：
   - `ex`
   - `Ms`

   这符合它的本质：不是单边系统，而是 ELF 与 MS 之间的桥梁。

3. **避免误读成普通工具名**  
   全小写 `exms` 更像普通命令、服务或工具；`exMs` 更像架构名、平台名、执行模型名。

---

## 3. 技术定位

### 3.1 exMs 不是这些东西

exMs 不是：

- 一个完整 Linux 发行版
- 一个简单的 Unix shell for Windows
- 一个单纯的 API 翻译层
- 一个只改外观的定制 Windows
- 一个完整虚拟机管理器

### 3.2 exMs 更接近这些东西的组合

从外观感受上，它像：

- 定制 XP / 定制 PE 的系统壳
- 在宿主系统上再盖一层自己的运行环境

从技术层级上，它更接近：

- ELF-first loader
- Linux ABI bridge
- 自定义运行时对象系统
- 轻量子系统骨架

因此可以把 exMs 概括为：

\[
\text{exMs} \approx \text{定制宿主壳} + \text{ELF 装载层} + \text{Linux ABI 桥} + \text{运行时对象骨架}
\]

---

## 4. 与相关路线的关系

## 4.1 与 WSL1（lxss.sys / lxcore.sys）的关系

WSL1 的经典思路，是通过 NT 上的 Linux 兼容实现，让 Linux 用户态程序可以在宿主 Windows 上成立。  
如果从问题意识看，exMs 和这条路线有明显共鸣：

- 都想让 Linux 程序在 NT 上成立
- 都不把完整虚拟 Linux 作为唯一答案
- 都在试图压缩“程序到宿主”的执行距离

但 exMs 和 WSL1 仍然有关键差异。

### WSL1 路线的特征
- 更偏向 **内核侧 Linux ABI 支撑**
- 更强调宿主系统内部补出 Linux 语义
- 重点在 syscall / 内核行为兼容

### exMs 路线的特征
- 更偏向 **ELF-first 执行架构**
- 强调 ELF loader + runtime object model
- 不预设自己必须复制某一种既有内核兼容实现路径
- 更容易把 PDMF/PFC 这样自定义运行时骨架纳入核心设计

所以，exMs 可以被理解为：

> **在问题意识上接近 WSL1，但在结构上更强调 ELF 装载、运行时对象统一和执行路径重构。**

## 4.2 与 Midipix 的关系

Midipix 更像是：

> **Windows 上的 POSIX / libc / toolchain 运行环境**

它的价值在于证明：

- 在 Windows 上认真做 POSIX 环境是可行的
- 兼容层不一定只能停留在脚本壳级别
- C 运行时、工具链、库层可以形成严肃工程体系

但 exMs 的重心并不在“先建一个 POSIX 开发环境”，而在：

- 把 ELF 当成执行本体
- 把 Linux syscall 视为运行时桥接对象
- 把 `fork / shm / epoll` 落到统一对象宇宙里
- 用 PDMF/PFC 统一运行时语义与调度

因此：

- **Midipix** 更像 “POSIX for Windows”
- **exMs** 更像 “ELF execution on NT”

## 4.3 与 Cygwin 的关系

Cygwin 主要价值在于：

- 给 Windows 提供 Unix 风格环境
- 重建一部分 POSIX 行为
- 提供较成熟的工具链与用户空间习惯

而 exMs 的目标更激进：

- 不只关心命令行环境
- 不只关心 POSIX API
- 更强调 ELF、ABI、对象层和调度层

所以 exMs 不是“再做一个 Cygwin”，而是：

> **比工具壳更底，比完整虚拟化更轻，比普通翻译层更偏运行时骨架。**

---

## 5. 核心设计原则

### 5.1 宿主与客体分离

在 exMs 中：

- Windows NT 负责承载
- ELF 程序负责表达业务和执行意图
- exMs runtime 负责桥接二者

也就是说：

\[
\text{Host} \neq \text{Guest Semantics}
\]

宿主并不自动拥有客体语义，客体也不必假装自己是宿主本地程序。

### 5.2 ELF-first

exMs 不把 ELF 视为必须先被翻译成 PE 的外来文件，而是把它视为第一类执行对象。  
这使得 exMs 的入口点天然是：

- ELF header
- Program headers
- Segment mapping
- Entry point

而不是 Win32 程序模板。

### 5.3 Runtime-first 而不是 shell-first

exMs 的重点不在“长得像不像 Linux”，而在于：

- 程序能否进入
- syscall 能否对话
- 对象能否落地
- 路径能否优化

也就是：

\[
\text{UI/表象} < \text{Runtime/语义}
\]

### 5.4 PDMF 是底板，PFC 是增强层

在 exMs 中：

- **PDMF** 负责统一 page / mapping / process / shm / event / ready 语义
- **PFC** 负责 fast/main 快慢路径调度

所以：

\[
\text{exMs} \supset \text{PDMF} \supset \text{PFC}
\]

这意味着 PDMF 是运行必要层，PFC 是性能增强层。

### 5.5 数据调度优先于道德调度

在 exMs 的快慢路径设计里，调度目标不是“照顾所有对象”，而是：

> **保护高价值路径、降低主路径干扰、避免低价值对象长期污染队列。**

这也是为什么 main 的处理逻辑应建立在：

- TTL
- expire
- cooldown
- requeue

之上，而不是长期挂队等待“公平救济”。

---

## 6. 总体架构

exMs 的总体架构可以写成：

\[
\text{Host NT/PE}
\rightarrow
\text{exMs Core}
\rightarrow
\text{Linux ABI Layer / exMs Apps}
\]

进一步拆开，可分成六个核心模块：

### 6.1 exboot
负责：
- 启动入口
- 初始目录挂载
- 系统组件激活
- runtime 初始状态建立

### 6.2 exloader
负责：
- `.exmsys / .exmapp / .exmdll` 装载
- ELF loader
- 模块绑定
- image layout 与入口点准备

### 6.3 exabi
负责：
- Linux syscall dispatcher
- fd / path / proc / basic ABI 门面
- Linux 与 exMs runtime 之间的最小语义桥

### 6.4 expdmf
负责：
- process/page/mapping
- fork/shm/event/ready 对象语义
- 统一运行时对象宇宙

### 6.5 expfc
负责：
- fast/main 分流
- strict fast-first
- TTL / expire / cooldown / rescue
- 快路径保护

### 6.6 exsvc
负责：
- broker
- registry
- package service
- runtime state service

---

## 7. 运行机制

## 7.1 ELF 装载

exMs 首先需要自己的 ELF loader。  
它至少应完成：

- ELF header 识别
- program header 扫描
- `PT_LOAD` 段提取
- image layout 规划
- entry point 确认

这一步的形式可写成：

\[
\text{ELF File} \rightarrow \text{Image Plan} \rightarrow \text{Executable Image}
\]

第一阶段建议先支持：

- 静态 ELF
- ELF64
- little-endian
- 最小 load plan

## 7.2 Linux syscall 分发

程序进入执行后，会不断发出 Linux syscall。  
因此 exMs 需要一个 dispatcher，把 syscall 转成不同 route：

- `exabi.fs`
- `expdmf.mem`
- `expdmf.proc`
- `expfc.ready`

即：

\[
syscall_{linux}
\rightarrow
dispatcher_{exMs}
\rightarrow
runtime\ route
\]

第一阶段最值得先支持的 syscall 子集包括：

- `openat`
- `close`
- `read`
- `write`
- `mmap`
- `munmap`
- `brk`
- `epoll_create1`
- `epoll_ctl`
- `epoll_wait`
- `exit`

## 7.3 PDMF 语义落地

在 exMs 中，`fork / shm / epoll` 不应各自发散，而应统一到 PDMF：

### fork
\[
fork = \text{mapping clone} + \text{COW}
\]

### shm
\[
shm = \text{shared mapping}
\]

### epoll
\[
epoll = \text{watch set} + \text{ready projection}
\]

这样 Linux 运行语义才能落到统一对象宇宙，而不是一组彼此无关的小翻译器。

## 7.4 PFC 快路径

在 PDMF 的 ready 投影层之上，PFC 负责快慢路径调度。

设 ready 集为：

\[
R = R_f \cup R_m
\]

其中：

- \(R_f\)：fast 区
- \(R_m\)：main 区

调度原则是：

\[
Q_f \neq \varnothing \Rightarrow \text{优先处理 } Q_f
\]

main 则不长期挂队，而采用：

\[
\text{enqueue} \rightarrow \text{TTL} \rightarrow \text{expire} \rightarrow \text{cooldown} \rightarrow \text{requeue}
\]

这样做的目的，不是“公平”，而是：

- 不让 main 污染主路径
- 不让 fast 长期交税
- 只给 main 最低生存，而不是持续干扰主路

---

## 8. 目录与包格式

## 8.1 宿主物理目录

建议的宿主物理目录为：

```text
/exms/
  boot/
  system/
  apps/
  lib/
  pkgs/
  state/
  registry/
  runtime/
  cache/
  logs/
  mounts/
```

其中：

- `system/`：`.exmsys` 展开后
- `apps/`：`.exmapp` 展开后
- `lib/`：`.exmdll` 展开后
- `pkgs/`：`.exmpkg` 本地缓存
- `registry/`：manifest / 权限 / 依赖索引
- `runtime/`：运行时临时态

## 8.2 运行格式

exMs 的运行格式定为：

- `.exmpkg`：总包 / 套件包
- `.exmsys`：系统包
- `.exmapp`：应用包
- `.exmdll`：动态模块

其中 Debian/APT 仍然负责分发层，exm 负责运行层：

\[
.deb \rightarrow .exmpkg \rightarrow \{.exmsys, .exmapp, .exmdll\}
\]

---

## 9. 数据与权限模型

## 9.1 文件系统

底层宿主数据格式优先选：

- **NTFS 作为物理承载层**

而 exMs 在其上投影：

- **ext/Linux-like 的语义视图**

也就是：

\[
\text{Host FS} = \text{NTFS}, \qquad
\text{Guest Semantics} = \text{ext-like}
\]

## 9.2 身份与权限

建议使用四元组：

\[
\text{Principal} = (exUid, exGid, exDid, exCaps)
\]

其中：

- `exUid`：用户编号
- `exGid`：组编号
- `exDid`：运行域 / 调度域编号
- `exCaps`：能力位

这比单纯沿用 NTFS ACL 或传统 Unix 三段权限更适合 exMs 的运行时模型。

---



## 9.3 编号原则

exMs 不适合只沿用宿主 NT 的对象编号，也不适合完全照抄传统 Unix 的单层 uid/gid 模型。  
原因是 exMs 同时面对四种不同维度：

- **身份维度**：这个主体是谁
- **组织维度**：它属于哪个组
- **运行维度**：它被放在哪个域里
- **能力维度**：它被允许做什么

因此，exMs 的编号应遵循：

\[
\text{Principal} = (exUid, exGid, exDid, exCaps)
\]

### 9.3.1 exUid：用户编号原则

`exUid` 用来表示主体身份，建议采用 **32-bit integer**，并保留 Linux 兼容感。

#### 编号分段建议

- `0`：root / system
- `1 ~ 99`：核心保留编号
- `100 ~ 999`：系统服务与内建账户
- `1000+`：普通用户 / 会话用户 / 外部主体

#### 设计原则

1. **低号保留给系统本体**
2. **普通用户从 1000 起跳**
3. **不要直接复用 NT SID 当 exUid**
4. **允许 exUid 与宿主 SID 建立映射表，但不能等同**

### 9.3.2 exGid：组编号原则

`exGid` 表示组织归属，建议也采用 **32-bit integer**。

#### 编号分段建议

- `0`：root/system group
- `1 ~ 99`：核心保留组
- `100 ~ 499`：系统功能组
- `500 ~ 999`：服务/应用功能组
- `1000+`：普通用户组 / session 组

#### 设计原则

1. **gid 用来表达组织归属，不用来表达调度优先级**
2. **组的职责应稳定，不随短期运行状态频繁变化**
3. **同一用户可属于多个组，但主组必须唯一**

### 9.3.3 exDid：域编号原则

`exDid` 表示运行域 / 隔离域 / 调度域。  
它不是“你是谁”，而是：

> **你被放在哪个运行域中。**

#### 推荐域划分

- `0`：sys-fast
- `1`：sys-main
- `2`：broker-fast
- `3`：broker-main
- `4`：service-mid
- `5`：app-main
- `6`：shared
- `7`：restricted
- `8`：maintenance

#### 设计原则

1. **did 决定运行位置，不决定身份**
2. **did 优先服务调度与隔离，不服务用户展示**
3. **对象域可细于用户组**
4. **PFC 的 fast/main 分层应主要依赖 did 与能力位，而不是 gid**

### 9.3.4 exCaps：能力位原则

`exCaps` 用于表达主体或对象可执行的动作集合。  
推荐实现为 **bitmask / capability set**。

#### 示例能力位

- `CAP_FAST_EVENT`
- `CAP_SHM_CREATE`
- `CAP_PROC_CLONE`
- `CAP_MAP_PRIV`
- `CAP_BROKER_BIND`
- `CAP_PKG_INSTALL`
- `CAP_SYS_MOUNT`
- `CAP_DEBUG_TRACE`

#### 设计原则

1. **能力位表达“可做什么”**
2. **身份编号表达“你是谁”**
3. **域编号表达“你被放在哪”**
4. **三者不可互相替代**

### 9.3.5 对象编号原则

除了主体编号，exMs 还需要对象编号：

- `page_id`
- `process_id`
- `event_id`
- `mapping_id`
- `package_id`

这些编号应遵循：

1. **内部唯一**
2. **允许宿主映射，但不直接等同宿主句柄**
3. **尽量采用单调递增 + generation/epoch 机制**
4. **用户可见 ID 与内部运行 ID 分离**

### 9.3.6 编号总原则

可以压成一句：

> **身份、组织、运行域、能力、对象键五者分离。**

也就是：

- `exUid`：是谁
- `exGid`：属哪组
- `exDid`：在哪个域跑
- `exCaps`：能做什么
- `ObjKey / object_id`：运行时对象是谁

## 9.4 NTFS 与文件系统策略

### 9.4.1 物理承载层选择

exMs 的宿主文件系统建议优先采用：

\[
\text{Host FS} = \text{NTFS}
\]

而不是直接把底层盘格式切换成 ext。  
原因是 exMs 运行在 **NT/PE** 宿主上，宿主工具链、驱动、恢复、挂载逻辑都天然围绕 NTFS。

因此，**物理承载层选 NTFS** 是最现实、最稳的路线。

### 9.4.2 语义层不是 NTFS 语义

虽然物理层是 NTFS，但 exMs 不能把 NTFS ACL、路径规则、对象语义直接当成自己的世界观。

也就是说：

\[
\text{NTFS ACL} \neq \text{exMs Permission Model}
\]

NTFS 在 exMs 中更接近：

- 宿主保护层
- 存储层
- 文件与目录承载层

而 exMs 自己要额外定义：

- ext-like 路径视图
- exUid/exGid/exDid/exCaps
- Linux 风格 mode bits
- 运行时对象宇宙

### 9.4.3 exMs 的双层文件系统观

#### 宿主物理视图

```text
/exms/
  boot/
  system/
  apps/
  lib/
  pkgs/
  state/
  registry/
  runtime/
  cache/
  logs/
  mounts/
```

#### 客体语义视图

```text
/
/bin
/usr
/etc
/tmp
/var
/proc
/dev
/home
```

这意味着 exMs 的目录树不等于宿主目录本身，而是：

\[
\text{Host Physical Tree} + \text{Guest Semantic Projection}
\]

### 9.4.4 元数据策略

exMs 的文件和对象，不能只靠 NTFS 原生属性表达。  
每个 exMs 文件/对象最好附带 exMs 自己的元数据，例如：

- `owner_uid`
- `owner_gid`
- `domain_id`
- `mode_bits`
- `cap_mask`
- `package_id`
- `object_class`
- `policy_flags`

### 9.4.5 mode bits 与 Linux 兼容感

在 exMs 的语义层，建议保留经典 Unix 风格权限位：

\[
rwxrwxrwx
\]

最终权限判定不只看 mode bits，还应满足：

\[
Perm_{final}
=
Perm_{basic}
\land
Perm_{domain}
\land
Perm_{cap}
\]

### 9.4.6 为什么不直接用 ext

底层不用 ext，不是因为 ext 不好，而是因为 exMs 的目标不是做一个“换盘格式的 Linux 仿生系统”，而是：

> **在 NT/PE 宿主上建立 Linux-like 语义投影。**

### 9.4.7 NTFS 部分的总原则

可以压成一句：

> **盘用 NTFS，规矩别用 NTFS 那套。**

也就是：

- 物理层：NTFS
- 客体视图：ext-like
- 权限模型：exMs 自己定义
- 调度与对象：PDMF/PFC 自己管理

## 9.5 WST 与 exMs 的关系

WST 不应被视为 exMs 的替代品，而应被视为：

> **跑在 exMs 之上的工作站工具层 / 操作层 / 管理层。**

两者的关系可以写成：

\[
\text{Host NT/PE}
\rightarrow
\text{exMs Core}
\rightarrow
\text{WST}
\]

其中：

- **exMs** 负责“让系统能运行”
- **WST** 负责“让人能操作这套系统”

### 9.5.1 exMs 的职责

exMs 本体负责：

- ELF 装载
- Linux ABI / syscall 分发
- PDMF 运行时对象层
- PFC 快慢路径调度
- exm 包格式与模块装载
- 身份、域、权限与对象管理

它更像：

> **平台骨架 / 子系统骨架 / 运行时底板**

### 9.5.2 WST 的职责

WST 负责：

- 工作站界面或控制台
- 调试与运维工具
- 包安装与管理前端
- trace / 监控 / 状态查看
- 开发与部署辅助工具
- 一组面向使用者的服务型应用

它更像：

> **控制室 / 管理器 / 工作站工具层**

### 9.5.3 包格式层面的关系

在 exm 体系中，WST 更适合被组织为：

- 一组 `.exmapp`
- 少量 `.exmsys`
- 若干 `.exmdll`

而 exMs 本体主要由：

- 核心 `.exmsys`
- 核心 `.exmdll`

组成。

因此：

- **exMs 是平台**
- **WST 是平台上的工作站套件**

### 9.5.4 设计原则

1. **WST 不反客为主，不定义 exMs 本体语义**
2. **WST 可以消费 exMs 的 registry、runtime state、package service**
3. **WST 可以作为 exMs 的默认操作环境，但不应成为 exMs 的唯一接口**
4. **去掉 WST 后，exMs 仍应保有最小运行能力**

## 9.6 注册表与 exMs 的关系

exMs 不能完全照抄 Windows Registry，也不能完全抛弃“注册表式元数据中心”。  
因此最合理的做法是：

> **保留“注册表”这个思想，但把它收缩成 exMs 自己的 runtime registry / manifest registry / policy registry。**

### 9.6.1 为什么 exMs 需要注册表

因为 exMs 不是单纯目录树系统，它还要管理：

- 包安装状态
- 模块依赖图
- 权限与能力策略
- 用户/组/域映射
- 运行时对象的命名索引
- Linux 语义投影配置
- WST 需要读取的状态与配置

这些东西如果全散在文件树里，会变得难以统一管理。

### 9.6.2 exMs 注册表不该是什么

它不该是：

- Windows Registry 的 1:1 复制品
- 任意程序都可无限写入的全局垃圾场
- 所有运行时状态的唯一真相来源

因为 exMs 已经有：

- 文件树
- PDMF 对象空间
- runtime state
- 包 manifest

所以 registry 应只是**中枢索引层**，不是“万物都往里塞”的黑洞。

### 9.6.3 exMs 注册表应管理什么

推荐至少分成四类：

#### 1. Package Registry
记录：
- 已安装 `.exmsys/.exmapp/.exmdll`
- 版本
- 依赖
- 激活状态
- 所属包与来源

#### 2. Identity / Policy Registry
记录：
- exUid / exGid / exDid 映射
- capability policy
- domain policy
- 默认权限模板

#### 3. Runtime Registry
记录：
- 当前运行时服务清单
- 已挂载视图
- broker/service 注册点
- 模块导出入口索引

#### 4. WST-facing Registry
记录：
- WST 需要展示的系统摘要
- package manager 状态
- trace/monitoring 元数据索引
- 工具层面可读的配置摘要

### 9.6.4 注册表与文件树的关系

最合理的关系是：

\[
\text{Registry} = \text{Index / Policy / State Hub}
\]

而不是：

\[
\text{Registry} = \text{Everything}
\]

也就是说：

- 文件本体在文件树
- 运行对象在 PDMF 对象空间
- 注册表负责把这些东西索引、命名、归类、施加策略

### 9.6.5 注册表与 WST 的关系

WST 最适合通过 registry 读取：

- 已安装包
- 已加载模块
- 当前域策略
- 当前运行时服务
- 监控摘要

因此 WST 不需要直接乱翻底层对象宇宙，而是：

\[
\text{WST} \rightarrow \text{Registry / Runtime Service} \rightarrow \text{exMs Core}
\]

这会比让 WST 直接碰底层对象层更干净。

### 9.6.6 注册表总原则

可以压成一句：

> **exMs 里的注册表不是宿主 Windows Registry 的翻版，而是 exMs 自己的“索引、策略与状态中枢”。**

它的职责是：
- 给系统一个统一元数据入口
- 给 WST 一个稳定读取界面
- 给包、身份、域、运行时服务一个集中索引点

而不是替代文件系统或替代运行时对象空间。


## 10. 开发路线

## 10.1 语言选择

建议：

- **C++**：主体开发语言
- **C**：少量底层胶水
- **Python**：原型、测试、扫参、trace
- **ASM**：极少量边界点

也就是：

\[
\text{exMs} = C++ + Python + 少量 C + 极少量 ASM
\]

## 10.2 开发顺序

### Phase 1
- ELF64 静态解析
- syscall route-only dispatcher
- PDMF / PFC 最小对象骨架

### Phase 2
- ELF image plan
- 最小 syscall mock 实现
- `fork / shm / epoll` 骨架闭环

### Phase 3
- 最小静态 Linux 程序装载实验
- fd/mmap/brk/epoll 初级闭环

### Phase 4
- event/observe object 完整化
- PFC sweet spot 参数化
- 运行时 profiling 与优化

---

## 11. 验收指标

### ELF 层
- ELF64 little-endian 识别成功率
- `PT_LOAD` 提取正确率
- entry/load plan 构造成功率

### ABI 层
- syscall route 覆盖率
- 最小 syscall 子集 mock 可用性
- errno/route 结构化输出正确率

### PDMF 层
- `fork` 映射克隆正确率
- `shm` 共享映射正确率
- ready 投影一致性

### PFC 层
- fast 路径平均延迟
- main 队列滞留率
- main 过期与重提率
- fast/main 干扰率
- 甜点区参数稳定性

---

## 12. 结论

**exMs（ELF-cross-MS）** 的核心，不是把 Windows 伪装成 Linux，也不是把 Linux 程序先塞进一个完整客体系统，而是试图建立一条更短的执行路径：

\[
\text{ELF} \rightarrow \text{exMs Runtime} \rightarrow \text{NT}
\]

在这条路径中：

- NT 是承载层
- ELF 是执行实体
- exMs runtime 是语义桥和对象骨架
- PDMF 负责统一对象宇宙
- PFC 负责高价值路径保护

从技术谱系看，exMs 与 WSL1 所解决的问题有相通之处，也能从 Midipix、Cygwin 这类兼容环境中吸收经验；但它的独特之处在于：  
**它不是以 shell、工具链或完整客体系统为中心，而是以 ELF-first、runtime-first、object-first 的方式，重新组织 Linux 程序在 NT 上的成立路径。**

这也是 exMs 的根本主张：

> **把 Windows 看成承载层，把 ELF 看成可直接召唤的实体，把中间差异压进一套可控制、可优化、可继续生长的执行骨架。**
