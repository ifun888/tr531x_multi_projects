# Light Gun Dongle 260517 项目架构与业务流程入门说明

> 面向入门开发人员。重点讲“这个 dongle 工程和光枪端有什么一样、有什么不一样”。

## 1. 这个工程的定位

`projects/light_gun_dongle_260517` 是“接收/中继端（dongle端）”工程。

它负责：
1. 通过 SLE 接收光枪端数据。
2. 通过 USB CDC（或同类串口语义通道）与 PC 互通。
3. 在协议层处理 Docked/MH 指令并返回结果。

## 2. 和光枪端的关系

两端代码结构几乎一致，主要差异点：
1. 编译宏 `OF_ROLE_DONGLE=1`（dongle 角色）。
2. SLE 发送 hook 不同：dongle 通过 `sle_uart_server_send_report_by_handle()` 发给对端。
3. 配置里 SLE 角色不同：dongle 侧启用 `SLE_UART_SERVER`。

## 3. 目录结构（怎么找代码）

核心仍在 `sdk_overlay/custom`：
- `src/app`：入口、运行主循环
- `src/sm`：状态机
- `src/services`：链路路由/服务逻辑
- `src/transport`：统一传输层与回调桥接
- `src/drivers`：USB/SLE驱动封装
- `src/protocols`：Docked/MH协议
- `src/platform`：平台工具

这套结构和光枪端统一，便于双端协同开发。

## 4. 启动流程（dongle端）

入口：`src/app/of_entry.c` 的 `demo_sle_uart_overlay_entry()`。

流程：
1. 初始化状态机并推进到 `READY`。
2. 初始化诊断模块。
3. 自动选择传输链路（USB优先，SLE兜底）。
4. 启动路由线程（周期调用 `svc_transport_route_tick`）。
5. 进入运行循环 `of_runtime_once()`。

## 5. 运行时业务流程

在 `src/app/of_runtime.c` 中，每次循环：
1. 更新链路状态（防抖切换）。
2. 从当前通道收包。
3. 数据进入协议层：
   - Docked 二进制命令
   - MH ASCII 命令
4. 更新诊断统计（字节计数、RTT分位）。

## 6. 传输架构（核心）

## 6.1 统一接口

对业务只暴露：
- `of_transport_init/read/write/deinit`

业务层无需知道“当前是 USB 还是 SLE”。

## 6.2 底层设备

- USB：`drv_usb_cdc.c`
- SLE：`drv_sle_link.c`

两者都实现 `f_ops` 风格接口（open/close/read/write/ioctl）。

## 6.3 链路切换

`svc_transport_router.c` 负责：
1. 采样 USB/SLE ready。
2. 稳定计数防抖。
3. 切换冷却时间。
4. 统一切换流程（deinit 旧链路 + init 新链路）。

## 7. 协议与数据互通

## 7.1 Docked 协议（最小闭环）

文件：`src/protocols/proto_docked.c`

支持：
1. 握手
2. 模式设置/读取
3. 配置写入/读取
4. 状态查询

这套命令用于快速验证“二进制协议链路是否打通”。

## 7.2 MH 协议（ASCII）

文件：`src/protocols/proto_mh.c`

支持：
1. `PING/PONG`
2. `MODE?`
3. `MODE=<num>`
4. `S`/`E` 简单控制

## 7.3 dongle 专用 SLE 发送路径

文件：`src/transport/transport_sle_sdk_hook.c`

逻辑：
1. 检查连接态 `sle_uart_client_is_connected()`。
2. 调用 `sle_uart_server_send_report_by_handle()` 发送。

这是 dongle 和光枪端最关键的实现差异之一。

## 8. 回调桥接（非常关键）

文件：`src/transport/transport_rx_hooks.c`

作用：
1. SDK sample 收到 SLE 数据后，通过 weak hook 回调 `of_sle_on_rx_data`。
2. 这里把数据推入 `drv_sle_link` 接收队列。
3. 主循环再通过 `of_transport_read()` 统一读取。

这样做的好处：
- 业务层完全不依赖 sample 内部细节。
- 以后替换 `sle_uart`/`sle_ota_dongle` 成本低。

## 9. 诊断与压测

板端：
- `of_diag.c` 输出 `tx/rx` 和 `rtt_us p50/p95/p99`。

PC端：
- 脚本 `tools/openfire/pc_latency_stress.py`。
- 可用于串口压力测试和延时分位统计。

## 10. 入门开发建议

1. 先读 `of_entry.c` 和 `of_runtime.c`，搞懂主流程。
2. 再读 `svc_transport_router.c`，理解切换策略。
3. 再看 `proto_docked.c` / `proto_mh.c`，理解协议收发。
4. 最后看 `transport_sle_sdk_hook.c`，理解 dongle 特有链路。

做到这 4 步后，再改业务逻辑会安全很多。
