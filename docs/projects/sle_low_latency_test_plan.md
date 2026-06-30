# SLE 低时延测试方案

## 1. 目的

针对 `projects/sle_low_latency_tx` 和 `projects/sle_low_latency_rx` 两个工程，建立一套可落地、可重复、可用示波器准确测量的 SLE 单向低时延测试方案，目标是验证端到端单向时延是否小于 `1 ms`。

这里定义的“单向时延”是：

- `TX` 侧应用代码发起一次发送动作的时刻
- 到 `RX` 侧应用代码收到该包并进入回调处理的时刻
- 这两个时刻之间的时间差

推荐使用 GPIO 电平跳变 + 双通道示波器测量，该方法比串口打印、软件计时更可靠，且不会明显扰动时延。

## 2. 当前工程状态核查

结合当前仓库内容，两个工程目前有以下事实：

1. `projects/sle_low_latency_tx/config/standard_tr5310_s.config` 当前是 `CLIENT` 角色，不是 `SERVER`。
2. `projects/sle_low_latency_rx/config/standard_tr5310_s.config` 当前也是 `CLIENT` 角色，不是 `SERVER`。
3. 两个工程当前都还是 `NORMAL_TYPE`，并没有打开 `LOW_LATENCY_TYPE`。
4. 两个工程已经使能了 GPIO 驱动，具备做 GPIO 打点的基础。
5. SDK 中真正的 SLE UART 业务路径在 `sdk_root_dir/application/samples/products/sle_uart/sle_uart.c`。

因此，如果要验证真正的低时延链路，至少需要先满足下面两个前提：

- 一端配置成 `SERVER`，另一端配置成 `CLIENT`
- 两端都打开 `CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE`

否则现在测到的只是普通模式链路，不是目标低时延链路。

## 3. 推荐测试思路

### 3.1 推荐方案

使用两块板和示波器双通道：

- `CH1` 接 `TX` 板测试 GPIO
- `CH2` 接 `RX` 板测试 GPIO
- 两块板 `GND` 与示波器地共地

测量方式：

- `TX` 板准备发送测试数据前，将测试 GPIO 拉高
- `RX` 板收到该包进入低时延接收回调后，将测试 GPIO 拉高
- 示波器测量 `CH1` 上升沿到 `CH2` 上升沿的时间差
- 该时间差即单向端到端时延

这是最直接、最准确、最容易解释的方案。

### 3.2 为什么不用串口打印计时

不推荐用串口日志作为主测量手段，原因如下：

- 串口打印本身会引入不可忽略抖动
- 打印路径会占用 CPU，污染测试结果
- 当目标是 `1 ms` 以内时，日志扰动已经不能忽略

串口日志只适合辅助确认功能是否通。

## 4. 角色与配置建议

建议将两个工程调整为如下角色：

- `projects/sle_low_latency_tx`：发送端，建议配成 `SERVER`
- `projects/sle_low_latency_rx`：接收端，建议配成 `CLIENT`

原因：

- SDK 中 `SERVER` 路径里有更明确的发送入口 `sle_uart_server_read_int_handler()`
- `CLIENT` 路径里有现成接收通知回调 `sle_uart_notification_cb()`
- 这两个点作为 GPIO 打点位置，语义清楚，便于定义时延起点和终点

### 4.1 配置项建议

在对应 `.config` 中至少保证：

- 发送端：
  - `CONFIG_SAMPLE_SUPPORT_SLE_UART=y`
  - `CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER=y`
  - `# CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT is not set`
  - `CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE=y`
  - `# CONFIG_SAMPLE_SUPPORT_NORMAL_TYPE is not set`

- 接收端：
  - `CONFIG_SAMPLE_SUPPORT_SLE_UART=y`
  - `CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT=y`
  - `# CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER is not set`
  - `CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE=y`
  - `# CONFIG_SAMPLE_SUPPORT_NORMAL_TYPE is not set`

另外保留：

- `CONFIG_FEATURE_GLE_LOW_LATENCY=y`

## 5. GPIO 打点原则

### 5.1 打点位置选择原则

起点和终点必须满足：

- 位置尽量靠近真实业务边界
- 尽量避免放在串口打印前后
- 尽量避免放在定时线程、消息队列等较远路径上
- 尽量使用“置高/置低”而不是“toggle”，避免示波器判读歧义

### 5.2 推荐打点定义

推荐定义两个 GPIO：

- `TX_MARK_GPIO`：发送板测试输出脚
- `RX_MARK_GPIO`：接收板测试输出脚

推荐选择空闲 GPIO，不要占用当前 UART 引脚 `17/18/19/20`。如果板级没有约束，建议优先选两个空闲 `S_MGPIO`。SDK 示例代码里曾使用过 `S_MGPIO10` 作为测试脚，可作为参考，但最终仍以你的硬件原理图为准。

## 6. 推荐测量点

### 6.1 发送侧起点

参考 SDK 文件：

- `sdk_root_dir/application/samples/products/sle_uart/sle_uart.c`

推荐起点放在：

- `sle_uart_server_read_int_handler()` 中
- 且在 `g_buff` 拷贝成功后、数据正式交给低时延发送路径之前

原因：

- 这个点已经拿到了本次要发的数据
- 这个点比较接近低时延发包入口
- 避开了更前面的串口中断和上层触发不确定性

建议动作：

1. 默认将 `TX_MARK_GPIO` 置低
2. 每次准备发一个测试包时，先 `TX_MARK_GPIO = 1`
3. 等待本次测试结束后，再软件复位为 `0`

### 6.2 接收侧终点

优先推荐终点放在：

- `sle_uart_notification_cb()`

如果低时延模式最终走的是专门的 `low_latency_rx_cb()`，则更推荐放在：

- `sle_uart_server_low_latency_recv_data_cbk()` 或等效低时延接收回调

原则是：

- 哪个回调最先拿到空口过来的有效负载，就在哪个回调里打终点

建议动作：

1. 默认将 `RX_MARK_GPIO` 置低
2. 收到测试包且校验通过后，立即 `RX_MARK_GPIO = 1`
3. 为了便于下一次采样，在延时一小段固定时间后拉回 `0`

## 7. 推荐测试包设计

建议测试包尽量短，优先测最小业务负载时延。

推荐测试负载：

- `1 byte`
- `4 byte`
- `8 byte`
- `20 byte`
- `50 byte`

其中建议把 `1 byte` 或 `4 byte` 作为主指标，因为你的目标是验证 `1 ms` 以内低时延能力，短包更能体现链路本征时延。

测试包内容建议至少包含：

- `seq`：包序号
- `tag`：固定测试标记

用途：

- 防止误触发非测试包
- 便于 RX 侧只对目标测试包拉高 GPIO

## 8. 示波器接线方案

### 8.1 接线

- 示波器 `CH1` 探头接 `TX_MARK_GPIO`
- 示波器 `CH2` 探头接 `RX_MARK_GPIO`
- 示波器地夹接系统公共地
- 两块板必须共地

### 8.2 示波器设置建议

建议设置：

- 触发源：`CH1` 上升沿
- 时基：先从 `200 us/div` 开始，再根据结果细调
- 采样率：尽量高，建议 `100 MSa/s` 及以上
- 带宽限制：关闭低带宽限制
- 测量项：`CH1 rising -> CH2 rising` 的时间差
- 采集方式：单次采样或正常采样均可，建议先单次抓图

### 8.3 波形判读

理想波形应满足：

- `CH1` 先上升
- 经过一段延时后 `CH2` 上升
- 两个上升沿之间的时间差就是单向时延

如果 `CH2` 没有稳定出现，通常说明：

- 两端未真正建立链路
- 收到的不是测试包
- RX 侧打点位置不在真正第一接收点
- GPIO 未正确初始化

## 9. 建议的软件实现方式

### 9.1 GPIO 初始化

初始化逻辑建议：

- `uapi_pin_set_mode(TEST_GPIO, HAL_PIO_FUNC_GPIO)`
- `uapi_gpio_set_dir(TEST_GPIO, GPIO_DIRECTION_OUTPUT)`
- `uapi_gpio_set_val(TEST_GPIO, GPIO_LEVEL_LOW)`

说明：

- 初始化完成后默认输出低电平
- 测试时只打窄脉冲或置高后再拉低

### 9.2 建议不要使用 toggle

SDK 样例里有 `uapi_gpio_toggle(SLE_UART_S_MGPIO)` 的写法，但正式测量不推荐直接照搬。

原因：

- `toggle` 无法保证每次都是同一方向边沿
- 连续包场景下，示波器不容易统一以“上升沿”作为时间基准

建议改成固定脉冲：

- 起点：`LOW -> HIGH`
- 一小段固定时间后再 `HIGH -> LOW`

## 10. 推荐测试流程

### 10.1 基础功能确认

1. 修改两个工程角色和模式，使其形成 `TX(server)` 与 `RX(client)` 配对。
2. 两端都打开 `LOW_LATENCY_TYPE`。
3. 加入 GPIO 初始化代码。
4. 先不接示波器，只确认两端可以稳定收发。
5. 用串口只打印少量关键日志，确认链路建立成功。

### 10.2 单发单测

1. `TX` 端手动触发发送一个短测试包。
2. 发送前 `TX_MARK_GPIO` 拉高。
3. `RX` 端收到测试包后立即拉高 `RX_MARK_GPIO`。
4. 示波器触发并记录两个上升沿时间差。
5. 重复 20 次，记录最小值、最大值、平均值。

### 10.3 连续统计

1. 固定包长，例如 `4 byte`。
2. 固定发送周期，例如 `20 ms` 或 `50 ms`，避免包间堆积。
3. 连续发 `100` 次。
4. 示波器打开 measurement statistics。
5. 观察：
   - 最小时延
   - 平均时延
   - 最大时延
   - 抖动范围

### 10.4 压力边界测试

再分别测试：

- 不同包长：`1/4/8/20/50 byte`
- 不同发送周期：`5 ms / 10 ms / 20 ms / 50 ms`
- 不同距离、不同天线摆位

目标是区分：

- 链路本征低时延
- 业务负载增大后的时延变化
- 无线环境导致的抖动

## 11. 结果判定标准

如果你的目标是“低于 `1 ms`”，建议按下面方式判定：

- 主判定指标：`4 byte` 负载下，单向时延平均值 `< 1 ms`
- 辅助判定指标：`4 byte` 负载下，单向时延最大值尽量也 `< 1 ms`
- 额外记录：最小值、平均值、最大值、标准差或抖动范围

如果只有最小值小于 `1 ms`，但平均值或最大值明显超过 `1 ms`，则不能认为该方案已经稳定满足目标。

## 12. 误差来源与规避

### 12.1 主要误差来源

- GPIO 打点位置选错
- GPIO 使用 `toggle` 导致边沿歧义
- RX 侧打点放在串口转发之后
- 测试时仍大量打印日志
- 两端发送过快，造成缓存堆积
- 角色或模式配置错误，实际跑的是普通模式

### 12.2 规避建议

- 只保留最少日志
- 先测短包，再测长包
- 先测低频单发，再测连续发送
- 起止点都放在“真正处理测试包”的第一时间点
- 示波器始终使用同一类边沿做测量

## 13. 最终推荐结论

针对你当前的需求，最可行的方案是：

1. 先把两个工程改成一端 `SERVER`、一端 `CLIENT`。
2. 两端都打开 `CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE`。
3. 在发送侧发送入口打 `TX_MARK_GPIO` 上升沿。
4. 在接收侧第一接收回调打 `RX_MARK_GPIO` 上升沿。
5. 用双通道示波器直接测两个上升沿之间的时间差。

这个方案的优点是：

- 测量定义清楚
- 仪器直读，不依赖软件时间戳同步
- 对 `1 ms` 以内目标足够准确
- 结果容易复现，也容易给别人复核

## 14. 建议的后续落地动作

如果下一步要真正执行测试，建议按下面顺序推进：

1. 先改工程配置，使 `tx/rx` 角色和 `LOW_LATENCY_TYPE` 正确生效。
2. 再在实际代码中加入两个 GPIO 测试点。
3. 最后做一次单包和连续包示波器验证。

如果需要，我下一步可以直接继续帮你把这份方案对应的代码改到两个工程里，包括：

- 配置项修改
- GPIO 初始化代码
- TX/RX 打点代码
- 一个最小可执行的测试触发流程
