# Light Gun 260517 项目架构与业务流程入门说明

> 面向第一次接触本项目的开发同学。目标是“先看懂，再敢改”。

## 1. 这个工程是做什么的

`projects/light_gun_260517` 是“光枪端”业务工程。

它主要做三件事：
1. 采集/接收输入与控制数据。
2. 通过统一传输层在 `USB CDC` 或 `SLE` 无线链路上传输数据。
3. 按协议处理上位机或对端命令（Docked/MH），并维护运行状态。

## 2. 目录结构怎么理解

核心代码在 `sdk_overlay/custom`：

- `include/`：头文件，放接口定义。
- `src/app/`：入口与主运行流程。
- `src/sm/`：状态机相关代码。
- `src/drivers/`：驱动抽象层（USB/SLE）。
- `src/transport/`：传输层封装与 SDK hook。
- `src/protocols/`：协议解析与回复。
- `src/services/`：业务服务层（路由、配置、校准等）。
- `src/platform/`：平台相关工具（如时间函数）。

可以把它看成 4 层：
1. 协议层：`protocols`
2. 业务编排层：`app + sm + services`
3. 传输层：`transport`
4. 驱动层：`drivers`

## 2.1 业务代码直接用到的驱动与 GPIO（精确口径）

> 口径说明：这里只统计 `sdk_overlay/custom` 业务代码里直接调用到的驱动，不统计“平台配置里启用但业务未调用”的外设。

### A. 直接用到的驱动

1. `drv_usb_cdc`
- 业务入口：`src/drivers/drv_usb_cdc.c`
- 直接调用：`usb_serial_read()`、`usb_serial_write()`
- 作用：USB CDC 数据收发

2. `drv_sle_link`
- 业务入口：`src/drivers/drv_sle_link.c`
- 直接调用路径：
  - 发送：`of_sle_sdk_send()`（在 `src/transport/transport_sle_sdk_hook.c` 实现）
  - 接收：`of_sle_on_rx_data()` 回调入队（`src/transport/transport_rx_hooks.c`）
- 作用：SLE 链路数据收发

3. 计时接口（诊断用途）
- 业务入口：`src/platform/of_time.c`、`src/app/of_runtime.c`
- 直接调用：`uapi_tcxo_get_us()`
- 作用：RTT/时延统计

### B. 这些“直接使用驱动”对应的 GPIO 结论

1. `drv_usb_cdc` 对应 GPIO
- **业务代码中未直接操作任何 GPIO 引脚号**。
- USB 物理引脚由底层 USB 驱动/板级初始化管理，不在本业务代码里显式配置。

2. `drv_sle_link` 对应 GPIO
- **业务代码中未直接操作任何 GPIO 引脚号**。
- SLE 的底层射频与串口映射由 sample/底层配置管理，不在本业务代码里写死 GPIO。

3. `uapi_tcxo_get_us` 对应 GPIO
- 这是时钟/计时接口，不对应业务层 GPIO 占用。

### C. 一句话结论

- 当前 `light_gun_260517` 业务代码直接用到的核心驱动是：`USB CDC`、`SLE`、`TCXO计时`。
- **业务代码层面直接占用的 GPIO：没有显式写死。**

## 3. 代码启动后走什么流程

入口函数是 `demo_sle_uart_overlay_entry()`：
文件：`src/app/of_entry.c`

启动过程（简化版）：
1. `of_sm_init()` 初始化状态机。
2. 连续 `of_sm_step()` 让启动状态推进到可运行阶段。
3. `of_diag_init()` 初始化统计模块。
4. `svc_transport_route_auto()` 自动选择链路（优先 USB，其次 SLE）。
5. 启动链路路由线程（2ms tick），持续做 USB/SLE 切换防抖。
6. 进入 `of_runtime_once()` 周期运行。

## 4. 运行时主循环做什么

主循环在 `src/app/of_runtime.c`，每次执行：
1. 先调用 `svc_transport_route_tick()`，维护链路状态。
2. 从当前链路读数据 `of_transport_read()`。
3. 收到数据后：
   - 统计 RX 字节数。
   - 若是压测 `PING` 帧，直接回 `PONG`。
   - 否则交给协议层：`of_proto_docked_process()`、`of_proto_mh_process()`。
4. 统计诊断信息（RTT 分位、TX/RX计数等）。

## 5. 传输层与驱动层关系

### 5.1 统一传输接口

统一接口在 `include/of_transport.h`：
- `of_transport_init`
- `of_transport_read`
- `of_transport_write`
- `of_transport_deinit`

业务层不关心底层是 USB 还是 SLE，只用这些接口。

### 5.2 f_ops 风格驱动

每个驱动都按 `of_fops_t` 提供：`open/close/read/write/ioctl`。

- USB 驱动：`src/drivers/drv_usb_cdc.c`
- SLE 驱动：`src/drivers/drv_sle_link.c`

`transport_core.c` 负责把统一接口转发给当前选中的驱动设备。

## 6. USB/SLE 切换机制（入门要点）

切换逻辑在 `src/services/svc_transport_router.c`：
1. 采样 `drv_usb_cdc_is_ready()` 和 `drv_sle_link_is_ready()`。
2. 用稳定计数防抖（不是一次 ready 就切）。
3. 加切换冷却时间，避免来回抖动。
4. 切换时先 `deinit` 旧链路，再 `init` 新链路。

## 7. 协议与数据互通（当前实现）

## 7.1 Docked（二进制）

文件：`src/protocols/proto_docked.c`

当前已实现最小命令集：
1. 握手：`01 02` -> 回复 `81 02 00`
2. 设置模式：`10 <mode>` -> 回复 `90 <mode> 00`
3. 读取模式：`11` -> 回复 `91 <mode>`
4. 写配置：`20 <idx> <val>` -> 回复 `A0 <idx> <val> 00`
5. 读配置：`21 <idx>` -> 回复 `A1 <idx> <val>`
6. 状态查询：`30` -> 回复 `B0 <mode> 01`

## 7.2 MH（ASCII）

文件：`src/protocols/proto_mh.c`

当前支持：
1. `PING` -> `PONG\n`
2. `MODE?` -> `MODE=<num>\n`
3. `MODE=<num>` -> `OK\n`
4. 首字母 `S` -> `START\n`
5. 首字母 `E` -> `END\n`

## 7.3 压测帧（内部测试协议）

`of_runtime.c` 里有测试回环协议头：
- 前两字节 `OF`
- 类型 `PING/PONG`

这套协议主要给 PC 压测脚本用，不属于正式业务协议。

## 8. 诊断统计怎么看

文件：`src/app/of_diag.c`

输出信息示例：
- `tx/rx`：累计发送/接收字节
- `rtt_us p50/p95/p99`：RTT 分位统计
- `n`：统计样本数

用于快速判断“链路是否稳定、延时是否劣化”。

## 9. 新人最容易踩的坑

1. 直接在业务代码里 include 驱动头并调用底层函数。
- 正确做法：业务层只用 `of_transport_*` 或 service 接口。

2. 把测试心跳和正式协议混发。
- 现在默认已关闭心跳（避免污染协议流）。

3. 修改协议时只改 parser，不改 reply。
- 协议是双向的，收发必须配套改。

4. 忽略链路切换防抖参数。
- 防抖/冷却窗口过小会导致 USB/SLE 来回切。

## 10. 推荐阅读顺序（30分钟入门）

1. `src/app/of_entry.c`
2. `src/app/of_runtime.c`
3. `src/services/svc_transport_router.c`
4. `src/transport/transport_core.c`
5. `src/drivers/drv_usb_cdc.c` + `src/drivers/drv_sle_link.c`
6. `src/protocols/proto_docked.c` + `src/protocols/proto_mh.c`

看完这 6 组文件，基本能理解整个系统的数据流和控制流。
