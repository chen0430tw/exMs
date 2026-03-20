# exMs 模块契约

## exboot
输入：
- `exms.toml`
- 宿主目录布局

输出：
- exMs runtime 初始状态
- system/apps/lib/pkgs/state/registry 挂载结果

## exloader
输入：
- `.exmsys/.exmapp/.exmdll`
- ELF / PE 路径

输出：
- 模块加载计划
- image layout
- 入口点信息

## exabi
输入：
- Linux syscall 编号与参数

输出：
- route
- result / errno
- runtime side-effect

## expdmf
输入：
- process/page/map/shm/fork 请求

输出：
- 对象与映射状态变更

## expfc
输入：
- ready objects

输出：
- fast/main 调度结果
- TTL/expire/requeue 行为
