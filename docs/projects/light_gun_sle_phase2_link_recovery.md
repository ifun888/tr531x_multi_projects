# 光枪 SLE 替代无线阶段2：建链握手与掉线回搜

## 1. 阶段目标

本阶段聚焦把 `projects/light_gun_260517` 与 `projects/light_gun_dongle_260517` 的 SLE 无线链路从“能编译、能收发”推进到“有明确建链握手、掉线回搜、可观测诊断”的状态。

目标边界：

- 光枪与 dongle 继续使用既定 `of_wireless_pkt` 业务封包。
- 不引入跳频、MAC 交换、信道交换、USB 身份交换。
- SLE 连接失败或业务握手超时后，直接回到广播/搜索。
- 不改动 `sdk_root_dir`，只在项目侧 `sdk_overlay/custom` 收口。

## 2. 本阶段整改位置

### 2.1 光枪工程

- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_sle_link.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/of_link_io.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/transport_rx_hooks.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/transport_sle_sdk_hook.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/app/of_entry.c`
- `projects/light_gun_260517/sdk_overlay/custom/CMakeLists.txt`

### 2.2 Dongle 工程

- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/drivers/drv_sle_link.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/transport/of_link_io.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/transport/transport_rx_hooks.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/transport/transport_sle_sdk_hook.c`

## 3. 开发方案

### 3.1 链路状态分层

链路分两层：

- `drv_sle_link` 负责物理连接态与“是否在搜索中”。
- `of_link_io` 负责业务握手态，也就是 `HELLO / HELLO_ACK / ready`。

这样可以把“已经 SLE 连上了”和“业务层允许开始透传”分开处理。

### 3.2 建链与回搜策略

策略如下：

1. SLE 物理连接建立后，业务层定时发送 `LINK_HELLO`。
2. dongle 收到 `LINK_HELLO` 后回复 `LINK_HELLO_ACK`。
3. 光枪收到 `LINK_HELLO_ACK` 后把链路置为 `ready`。
4. 若 `ready` 在超时时间内未建立，则请求重新搜索。
5. 若连续发送失败达到阈值，则主动断开并重新进入搜索/广播。

### 3.3 诊断要求

本阶段必须能从串口日志判断以下事件：

- SLE open
- connected / disconnected
- search start
- `HELLO`
- `HELLO_ACK`
- ready established
- ready timeout fallback
- repeated TX fault fallback

## 4. 验收标准

- 两个工程均能编译通过。
- 不新增 `sdk_root_dir` 修改。
- 上板后可通过日志确认上述关键状态流转。
- 掉线或握手失败后，链路会回到搜索/广播状态，而不是卡死在半连接态。

## 5. 完成记录

### 5.1 实现结果

本阶段已完成以下收口：

- `drv_sle_link` 增加 `opened / connected / searching` 本地状态。
- 发送失败不再假成功，而是累计 TX fault 并触发回搜。
- `of_link_io` 增加 `HELLO / HELLO_ACK` 业务握手和 `ready timeout` 回退。
- 光枪端显式把 `sle_uart_client.c` 拉入 overlay 侧编译，避免 client 控制路径只剩 weak stub。
- 光枪与 dongle 两端都补齐了接收回调到 `drv_sle_link_push_rx()` 的路径。
- 本阶段补充了建链、回搜、握手、TX fault 的最小诊断日志。

### 5.2 验证记录

- `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 产物：`projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`8bef5205f021a27f19793b7827a6416633b2afb86829870e83717a9928fc4f6e`
- `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 产物：`projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`82210edf4999d6611041aa9d2158d46ab64eb832d4e809cfd3f22bad7a061cd3`

补充说明：

- 两个工程的构建脚本会共享切换 `sdk_root_dir/application/samples/custom` 软链接，因此构建验证需要串行执行，不能并行跑两个工程。
- 本阶段新增改动均位于项目侧 overlay 和 `docs/projects`，没有继续修改 `sdk_root_dir`。
