# ELF smoke test

目标：
- 读取 ELF64 header
- 验证 magic / class / endian
- 提取 entry 和 PT_LOAD program headers

当前状态：
- 只支持 ELF64 little-endian
- 只做静态解析，不做真实内存装载
