# PDMF

PDMF = 分页动态映射框架。

统一对象：
- process
- page
- mapping
- shm
- event
- ready

统一语义：
- fork = mapping clone + COW
- shm = shared mapping
- epoll = watch set + ready projection
