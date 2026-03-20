# TODO: compat/linux/elf

- [x] 解析 ELF64 header
- [x] 扫描 PT_LOAD
- [ ] 构造最小 image plan
- [ ] 统一 segment permission flags
- [ ] 计算 image base / span
- [ ] 预留 stack/auxv/argv 布局接口
- [ ] 为静态 ELF 加 smoke test
