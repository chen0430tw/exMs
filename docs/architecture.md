# exMs 架构概览

## 总体层级

Host NT/PE
-> exMs Core
-> Linux ABI Layer / exMs Apps

## 核心模块

- exboot：启动与初始挂载
- exloader：PE/ELF/exm 包装载器
- exabi：Linux syscall/路径/fd 兼容层
- expdmf：分页动态映射框架
- expfc：快慢路径调度层
- exsvc：broker/registry/package/runtime 服务层

## 设计原则

1. 核心要小
2. 热路径要直
3. 对象要复用
4. 冷功能外挂化
5. 先静态 ELF，后动态 .so
