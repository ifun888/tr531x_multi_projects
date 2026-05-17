# sle_uart 工程时延分析与进一步优化方案（支持双向、<=50B）

## 1. 现状重述（结合你的补充）

你当前的问题不是“只在普通模式慢”，而是：

- 即使开启了 `LOW_LATENCY_TYPE`，时延仍然偏大。
- 时延测试不是单向固定路径，**Client->Server->Client** 和 **Server->Client->Server** 都要评估。
- 业务负载不是固定 15B，而是 **0~50B** 都要稳定低时延。

所以优化目标要从“单场景 15B”升级为：

- 双向 RTT 都要低
- 负载 <=50B 时延稳定
- P50/P90/P99 都可控（不仅看最小值）

## 2. 为什么开了低时延模式，仍可能出现大延迟

## 2.1 低时延模式并不自动覆盖双向业务链路

样例里低时延能力是存在的：

- Server 侧：`sle_low_latency_tx_enable`（`application/samples/products/sle_uart/sle_uart_server/sle_uart_server.c:396-401`）
- Client 侧：`sle_low_latency_rx_enable + sle_low_latency_set(..., 2000)`（`application/samples/products/sle_uart/sle_uart_client/sle_uart_client.c:205-207`）

但如果某一方向仍走普通 `ssapc_write_req` / notify 路径，就会混入 GATT 写请求确认路径时延，导致整体 RTT 仍偏大。

- 普通写路径触发点：`application/samples/products/sle_uart/sle_uart.c:403-411`

结论：**只开宏不等于双向都走低时延数据通道**。

## 2.2 UART 当前配置会导致拆包，50B场景更明显

两端 UART 回调是：

- `UART_RX_CONDITION_FULL_OR_SUFFICIENT_DATA_OR_IDLE`
- 阈值 `1`

代码：
- Client：`application/samples/products/sle_uart/sle_uart.c:420-423`
- Server：`application/samples/products/sle_uart/sle_uart.c:265-267`

这会导致 15B 甚至 50B 都可能被拆成多个上行事务；包越大，拆包概率和事务数越高，抖动越明显。

## 2.3 连接参数仍偏“省电容忍”，不是“强实时”

当前 server 参数：

- `conn_interval = 0x40`（8ms）
- `conn_max_latency = 0x1F3`（499）

代码：`application/samples/products/sle_uart/sle_uart_server/sle_uart_server_adv.c:25-28,36,143-146`

接口定义里 `conn_max_latency` 就是最大休眠连接间隔（`include/middleware/services/bts/sle/sle_device_discovery.h:212-214`）。

这类配置会提升省电容忍，但会放大时延尾部抖动（P99）。

## 3. 针对双向 + <=50B 的可行优化方案

## 3.1 目标与原则

1. 双向都统一走低时延通道（不混用普通写路径）。
2. UART 先聚合成帧再发空口，减少事务数。
3. 链路参数偏低时延（短间隔、低休眠容忍）。
4. 用“序号+时间戳”做双向 RTT 基准统计（P50/P90/P99）。

## 3.2 具体改造步骤

1. 确认双向数据面都走 low-latency API
- Client->Server 与 Server->Client 均采用 low-latency TX/RX 回调。
- 避免一侧 low-latency、另一侧仍 `ssapc_write_req` 的混合路径。

2. 增加 UART 聚合发送层（适配 <=50B）
- 维护一个小聚合窗口（建议 `0.5~2ms` 可配）。
- 条件触发发送：
  - 收到“完整帧”标记（协议头/长度/结束符）立即发。
  - 达到长度门限（如 50B）立即发。
  - 超时触发（0.5~2ms）发当前缓存。
- 目标：把“多次字节回调”压缩成“单次空口事务”。

3. 连接参数改成低时延优先
- `conn_interval` 建议从 `0x1E~0x20` 试配（比 8ms 更激进）。
- `conn_max_latency` 在测试阶段设 `0`，先压低 P99。
- 稳定后再按功耗需求回调。

4. 维持高 PHY/MCS（已有配置）
- 低时延模式中已有 4M PHY / QPSK / MCS10 设置流程：
  - `application/samples/products/sle_uart/sle_uart_client/sle_uart_client.c:175-183,210`
- 保持该配置，避免链路速率成为瓶颈。

5. 对 <=50B 进行分档策略
- 1~15B：零等待或极短等待（<=0.5ms）
- 16~32B：短聚合窗口（~1ms）
- 33~50B：窗口可放宽到 1~2ms，优先一次发完
- 通过配置项控制窗口，而不是写死。

## 4. 双向时延测试建议（必须双向测）

1. 定义两类 RTT：
- `RTT_C2S`：Client 发 -> Server 回响 -> Client 收
- `RTT_S2C`：Server 发 -> Client 回响 -> Server 收

2. 每个方向、每种负载都测：
- 负载：`8B / 15B / 32B / 50B`
- 每档至少 `1000` 次
- 统计：`min / avg / P50 / P90 / P99 / max`

3. 判定标准建议
- 以 `P90` 作为常态体验指标
- 以 `P99` 作为稳定性约束
- 不只看最小值

## 5. 预期结果（合理范围）

在“**双向 low-latency + UART 聚合成帧 + 低时延连接参数**”落实后，<=50B 场景通常会出现：

- 8B/15B：P50 接近 10~15ms
- 32B/50B：P50 可能略高，但应显著低于当前 50~160ms
- P99 明显收敛（不再频繁出现百毫秒级）

说明：精确值依赖现场干扰、固件调度、外设回响逻辑和测试治具，但这条路线是工程上可落地且有效的。

## 6. 结论

你要解决的是“**双向、可变长度（<=50B）低时延稳定性**”，不是单点 15B 优化。

核心抓手是三件事：

1. 双向统一低时延数据通道
2. UART 聚合成帧，减少空口事务数
3. 连接参数从省电导向切换到低时延导向（尤其 `conn_max_latency`）

按这三步推进，才有机会把两方向 RTT 都稳定压到你期望的量级。
