# exMs 包格式

- `.exmpkg`：总包 / 套件包
- `.exmsys`：系统包
- `.exmapp`：应用包
- `.exmdll`：动态模块

关系：

`.deb` -> `.exmpkg` -> `{.exmsys, .exmapp, .exmdll}`

其中 Debian/APT 继续负责分发层，exm 系列负责运行层。
