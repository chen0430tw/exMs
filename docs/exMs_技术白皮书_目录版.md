# exMs 技术白皮书（正式发布稿目录版）

版本：v0.1  
状态：发布稿目录版  
定位：面向项目立项、架构沟通、后续研发接手

---

## 封面信息

- 项目名称：**exMs（ELF-cross-MS）**
- 项目定位：**NT/PE 宿主上的 ELF-first 异构执行架构**
- 核心主张：**把 Windows 看成承载层，把 ELF 看成可直接召唤的执行实体**
- 关键词：ELF Loader、Linux ABI、PDMF、PFC、NTFS、WST、Registry

---

## 目录

### 1. 摘要
1.1 exMs 的基本定义  
1.2 exMs 的核心目标  
1.3 exMs 与传统路线的根本区别  
1.4 exMs 的总体技术定位  

### 2. 背景与问题定义
2.1 Windows 上运行 Linux 程序的传统路线  
2.2 现有方案的结构性限制  
2.3 exMs 的问题意识  
2.4 exMs 试图重写的问题模型  

### 3. exMs 的定义与命名
3.1 exMs 的正式定义  
3.2 exMs 的命名来源  
3.3 为什么写作 exMs 而不是 exms  
3.4 M 大写所表达的宿主轴心含义  

### 4. 技术定位
4.1 exMs 不是什么  
4.2 exMs 更接近什么  
4.3 exMs 的平台属性  
4.4 exMs 的子系统属性  
4.5 exMs 的运行时属性  

### 5. 与相关路线的关系
5.1 与 WSL1（lxss.sys / lxcore.sys）的关系  
5.2 与 Midipix 的关系  
5.3 与 Cygwin 的关系  
5.4 与虚拟化路线的关系  
5.5 exMs 的独特位置  

### 6. 核心设计原则
6.1 宿主与客体分离  
6.2 ELF-first 原则  
6.3 Runtime-first 原则  
6.4 PDMF 是底板，PFC 是增强层  
6.5 数据调度优先于道德调度  
6.6 热路径短、冷路径厚  
6.7 先静态 ELF，后动态 .so  

### 7. 总体架构
7.1 总体层级结构  
7.2 Host NT/PE 层  
7.3 exMs Core 层  
7.4 Linux ABI Layer  
7.5 exMs Apps / 工具层  
7.6 模块间关系图  

### 8. 核心模块设计
8.1 exboot  
8.2 exloader  
8.3 exabi  
8.4 expdmf  
8.5 expfc  
8.6 exsvc  

### 9. 运行机制
9.1 ELF 装载机制  
9.2 Linux syscall 分发机制  
9.3 PDMF 对象统一机制  
9.4 PFC 快慢路径机制  
9.5 最小 Linux 子集运行闭环  
9.6 运行时状态流转  

### 10. 编号原则
10.1 exUid：用户编号原则  
10.2 exGid：组编号原则  
10.3 exDid：域编号原则  
10.4 exCaps：能力位原则  
10.5 对象编号原则  
10.6 编号体系的总原则  

### 11. 文件系统与存储策略
11.1 为什么底层选择 NTFS  
11.2 为什么语义层不能直接等于 NTFS ACL  
11.3 exMs 的双层文件系统观  
11.4 宿主物理目录树  
11.5 客体语义视图  
11.6 元数据策略  
11.7 mode bits 与 Linux 兼容感  
11.8 “盘用 NTFS，规矩别用 NTFS 那套” 的含义  

### 12. 包格式体系
12.1 `.exmpkg`  
12.2 `.exmsys`  
12.3 `.exmapp`  
12.4 `.exmdll`  
12.5 Debian/APT 与 exm 运行格式的关系  
12.6 包安装、展开与激活流程  

### 13. 注册表与状态中枢
13.1 exMs 为什么仍需要 registry  
13.2 exMs registry 不是什么  
13.3 Package Registry  
13.4 Identity / Policy Registry  
13.5 Runtime Registry  
13.6 WST-facing Registry  
13.7 registry 与文件树、PDMF、WST 的边界  

### 14. WST 与 exMs 的关系
14.1 WST 的定位  
14.2 exMs 与 WST 的分层  
14.3 WST 在包格式层面的归属  
14.4 WST 对 registry/runtime service 的依赖  
14.5 为什么 WST 不能反客为主  

### 15. PDMF 与 PFC 在 exMs 中的位置
15.1 PDMF 的职责  
15.2 PFC 的职责  
15.3 fork / shm / epoll 的统一落地  
15.4 fast / main 分流  
15.5 TTL / expire / cooldown / requeue  
15.6 PFC 甜点区的工程意义  

### 16. 开发路线
16.1 语言选择  
16.2 第一阶段目标  
16.3 第二阶段目标  
16.4 第三阶段目标  
16.5 为什么先做 ELF / syscall  
16.6 为什么先不做 GUI / 完整 procfs / 大而全动态链接  

### 17. 验收指标
17.1 ELF 层指标  
17.2 ABI 层指标  
17.3 PDMF 层指标  
17.4 PFC 层指标  
17.5 WST / Registry / Package 层指标  
17.6 阶段验收方法  

### 18. 风险与边界
18.1 不应一开始做成胖操作系统  
18.2 不应把 exMs 变成纯工具壳  
18.3 不应让 WST 反过来定义 exMs 本体  
18.4 不应让 Registry 变成垃圾场  
18.5 不应把 NTFS ACL 直接当 exMs 权限模型  

### 19. 结论
19.1 exMs 的最终技术主张  
19.2 exMs 与现有路线的根本区别  
19.3 exMs 的长期扩展方向  
19.4 exMs 的一句话总结  

---

## 附录建议

### 附录 A：术语表
- ELF
- PE
- NT
- ABI
- PDMF
- PFC
- WST
- Registry
- exUid / exGid / exDid / exCaps

### 附录 B：最小目录树
### 附录 C：最小包格式样例
### 附录 D：最小 syscall 子集
### 附录 E：PFC 参数甜点区实验摘要

---

## 发布稿说明

这份“目录版”用于：
- 项目白皮书正式排版前的结构冻结
- 给 Claude / 开发者 / 协作者统一章节边界
- 后续扩写时防止文档继续发散

建议下一步按本目录逐章填充正文，并保持：
- 先架构，后实现
- 先原则，后细节
- 先边界，后扩张
