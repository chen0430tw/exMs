# PFC

PFC = 页面快速缓存 / 快路径调度层。

原则：
- strict fast-first
- main minimal survival
- main 不得长期挂死污染队列
- TTL + expire + cooldown + requeue

当前甜点区经验点：
- rescue_interval = 4
- rescue_quota = 3
- main_ttl = 5
- cooldown = 1
