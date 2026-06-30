# sle_low_latency_tx/rx 移植与测试方案

## 1. 目标

基于现有两个工程：

- `projects/sle_low_latency_tx`
- `projects/sle_low_latency_rx`

在 **不改动 `sdk_root_dir` 源码** 的前提下，完成 SLE 低时延工程化移植，并给出可执行的测试方案。

这里的“移植”不是重写 SDK 的 `sle_uart` sample，而是：

- 复用 SDK 已有的 `sle_uart` low latency 能力
- 通过 `project config` 打开正确角色与模式
- 通过 `sdk_overlay/custom` 挂接业务发送、接收和 GPIO 打点
- 将工程变成可维护的项目侧实现，而不是直接改 SDK 源文件

## 2. 对参考文档 `ref_projs/low_latency_md` 的结论

参考文档给了两条建议：

1. 用 `sle_uart_client.c` 覆盖 `application/samples/products/sle_uart/sle_uart_client/sle_uart_client.c`
2. 用 `app.json` 覆盖 `middleware/chips/tr5310/nv/nv_config/tr5310_nv_default/cfg/acore/app.json`

这两条对当前仓库并不都是必须，原因如下。

### 2.1 `sle_uart_client.c` 覆盖不是首选

当前仓库里的 SDK `sle_uart_client.c` 已经包含：

- `CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE` 下的 `sle_low_latency_rx_enable()`
- `sle_low_latency_set(conn_id, true, 1000)`
- `sle_set_mcs()`
- `CONFIG_SAMPLE_SUPPORT_PERFORMANCE_TYPE` 下的 PHY 参数设置

也就是说，如果你的目标是：

- 跑通低时延链路
- 做工程化封装
- 做示波器时延测试

那么第一优先级不是覆盖 SDK client 源码，而是把 `.config` 配正确，并在 `sdk_overlay/custom` 做业务挂接。

### 2.2 `app.json` 覆盖不是当前主路径必需项

当前 SDK 的 `sle_uart` server 广播名直接写死在：

- `sdk_root_dir/application/samples/products/sle_uart/sle_uart_server/sle_uart_server_adv.c`

其本地名字是：

- `sle_uart_s_test`

同时 client 侧扫描逻辑也是按这个名字匹配。因此：

- 对“是否能连上”这件事，当前 sample 不依赖你参考文档里的 `app.json`
- `app.json` 只有在你要改 NV 缺省设备信息、地址、产品信息时才有意义

结论：

- 本次移植不以 `app.json` 覆盖为主方案
- 先按 SDK 默认名字和默认广播逻辑完成移植与测试

## 3. 当前两个工程的实际状态

当前 `sle_low_latency_tx` 和 `sle_low_latency_rx` 的配置状态并不满足“低时延一发一收”的目标：

- 两边当前都是 `SLE_UART_CLIENT`
- 两边当前都是 `NORMAL_TYPE`
- 两边虽然启用了 `CONFIG_FEATURE_GLE_LOW_LATENCY=y`，但 sample 模式并没有切到 low latency

这意味着当前这两个工程还只是“名字像低时延”，配置上并没有真正形成：

- 一端发
- 一端收
- 低时延模式

所以移植第一步必须先把角色和 sample mode 改正确。

## 4. 不改 SDK 的移植原则

不改 `sdk_root_dir` 时，建议采用下面这套边界：

### 4.1 SDK 负责的部分

由 SDK sample 继续负责：

- SLE 初始化
- 扫描 / 广播 / 建链
- SSAP 服务发现
- 低时延使能
- 低时延空口收发调度

### 4.2 项目 overlay 负责的部分

由 `projects/<name>/sdk_overlay/custom` 负责：

- 业务入口初始化
- 接收数据 hook
- 发送数据封装
- GPIO 打点
- 测试触发流程
- 缓冲与统计

这条边界很重要。你要移植的是“项目侧业务层”，不是“再造一个 SDK sample”。

## 5. 推荐角色划分

建议改成：

- `projects/sle_low_latency_tx`：`SERVER`
- `projects/sle_low_latency_rx`：`CLIENT`

原因：

- SDK server 侧广播名固定，client 默认就按这个名字扫描，最省事
- SDK server 侧有公开头文件 `sle_uart_server.h`，提供 `sle_uart_server_send_report_by_handle()`，业务侧可直接调用
- SDK client 侧有现成接收回调和 weak hook `of_sle_on_rx_data()`，业务侧很好挂接

如果你反过来做也不是绝对不行，但会让“项目侧发包接口”更绕。

## 6. 配置移植方案

### 6.1 `sle_low_latency_tx` 建议配置

目标：发送端，server 角色，low latency 模式。

建议关键配置：

- `CONFIG_SAMPLE_SUPPORT_SLE_UART=y`
- `CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER=y`
- `# CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT is not set`
- `CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE=y`
- `# CONFIG_SAMPLE_SUPPORT_NORMAL_TYPE is not set`
- `CONFIG_FEATURE_GLE_LOW_LATENCY=y`

可选增强：

- `CONFIG_SAMPLE_SUPPORT_PERFORMANCE_TYPE=y`

说明：

- 如果你要尽量逼近 `1 ms` 甚至更低，建议把 performance type 也打开
- 这样 SDK 内部会走更激进的 PHY/MCS 配置路径

### 6.2 `sle_low_latency_rx` 建议配置

目标：接收端，client 角色，low latency 模式。

建议关键配置：

- `CONFIG_SAMPLE_SUPPORT_SLE_UART=y`
- `CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT=y`
- `# CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER is not set`
- `CONFIG_SAMPLE_SUPPORT_LOW_LATENCY_TYPE=y`
- `# CONFIG_SAMPLE_SUPPORT_NORMAL_TYPE is not set`
- `CONFIG_FEATURE_GLE_LOW_LATENCY=y`

可选增强：

- `CONFIG_SAMPLE_SUPPORT_PERFORMANCE_TYPE=y`

### 6.3 配置变更方式

推荐用项目上下文配置，不直接手改 SDK：

```bash
cd /data/tr531x_proj
./tools/tr531x/project_menuconfig.sh --project sle_low_latency_tx --target standard-tr5310-s --patch-set default
./tools/tr531x/project_menuconfig.sh --project sle_low_latency_rx --target standard-tr5310-s --patch-set default
```

完成后由项目配置文件承接，不需要你去改 `sdk_root_dir`。

## 7. overlay 移植方案

你的主战场应该是：

- `projects/sle_low_latency_tx/sdk_overlay/custom/`
- `projects/sle_low_latency_rx/sdk_overlay/custom/`

当前这两个工程的 overlay 还是空壳，只有一个 `demo_sle_uart.c`。建议扩成最小业务骨架。

### 7.1 TX 工程建议目录

建议扩成：

```text
projects/sle_low_latency_tx/sdk_overlay/custom/
├─ CMakeLists.txt
├─ Kconfig
├─ include/
│  ├─ ll_test.h
│  └─ ll_gpio_mark.h
└─ src/
   ├─ app/ll_tx_entry.c
   ├─ app/ll_tx_runtime.c
   ├─ transport/ll_tx_sdk_hook.c
   ├─ transport/ll_tx_trigger.c
   └─ drivers/ll_gpio_mark.c
```

### 7.2 RX 工程建议目录

建议扩成：

```text
projects/sle_low_latency_rx/sdk_overlay/custom/
├─ CMakeLists.txt
├─ Kconfig
├─ include/
│  ├─ ll_test.h
│  └─ ll_gpio_mark.h
└─ src/
   ├─ app/ll_rx_entry.c
   ├─ transport/ll_rx_hooks.c
   ├─ transport/ll_rx_stats.c
   └─ drivers/ll_gpio_mark.c
```

## 8. 关键挂接点

### 8.1 接收侧挂接点

SDK `sle_uart.c` 已经提供 weak hook：

- `of_sle_on_rx_data(const uint8_t *data, uint16_t len)`
- `of_sle_on_link_state(int connected)`

因此在 `sle_low_latency_rx` 的 overlay 里实现这两个函数即可。

推荐做法：

- `of_sle_on_link_state()`：记录链路状态
- `of_sle_on_rx_data()`：
  - 判断是否是测试包
  - 若是测试包，立即拉高 `RX_MARK_GPIO`
  - 记录序号、长度、到达次数
  - 需要时再入队给上层处理

这是最干净的接收移植点，不需要改 SDK。

### 8.2 发送侧挂接点

发送侧不要再绕 UART 注入，推荐直接用 SDK 已公开的 server 发送接口：

- 头文件：`sle_uart_server.h`
- 接口：`sle_uart_server_send_report_by_handle(const uint8_t *data, uint8_t len)`
- 链路状态：`sle_uart_client_is_connected()`

因此在 `sle_low_latency_tx` overlay 里封装一个项目侧接口，例如：

- `int ll_tx_send_test_packet(const uint8_t *buf, uint16_t len)`

内部逻辑：

1. 判断 `sle_uart_client_is_connected()`
2. 拉高 `TX_MARK_GPIO`
3. 调 `sle_uart_server_send_report_by_handle()` 发包
4. 发送后根据测试策略延时拉低 GPIO

这样业务代码不需要去碰 SDK 内部的 static 函数。

### 8.3 入口函数

在 overlay 的 `demo_sle_uart_overlay_entry()` 里做项目级初始化：

- GPIO 初始化
- 测试状态初始化
- 可选：启动一个周期性发包线程
- 可选：注册 shell/test_suite 命令

注意：

- overlay 入口负责“项目业务初始化”
- SLE 建链流程还是由 SDK sample 继续负责

## 9. 为什么不建议再覆盖 `sle_uart_client.c`

除非出现下面两类刚性需求，否则不建议覆盖：

1. 你必须修改 client 的扫描判定逻辑
2. 你必须修改 connect callback 内部的低时延参数设置逻辑，而这些逻辑没有公共 API 可在 overlay 外挂完成

当前仓库下，这两个条件都不是必须：

- server 名字已经与 client 扫描常量匹配
- low latency 使能和 performance 配置路径已在 SDK sample 里存在

因此优先方案应是：

- 配 `.config`
- 用 `sdk_overlay/custom` 扩展业务层

而不是把 sample 源码复制一份继续分叉。

## 10. 最小可行移植步骤

### 10.1 第一步：修正角色和模式

先把两个工程配置改成：

- `sle_low_latency_tx = server + low_latency`
- `sle_low_latency_rx = client + low_latency`

这是先决条件。

### 10.2 第二步：建立 overlay 业务骨架

分别在两个工程的 `sdk_overlay/custom` 下新增：

- `src/app`
- `src/transport`
- `src/drivers`
- `include`

并修改 `CMakeLists.txt` 把这些新文件编进去。

### 10.3 第三步：打通 RX hook

先在 RX 工程里实现：

- `of_sle_on_rx_data()`
- `of_sle_on_link_state()`

先确认：

- 能收到包
- 能正确记录长度和次数
- 能在示波器上看到 `RX_MARK_GPIO` 跳变

### 10.4 第四步：打通 TX 业务发送接口

在 TX 工程里封装：

- `ll_tx_send_test_packet()`

内部直接调：

- `sle_uart_server_send_report_by_handle()`

先用固定 `1 byte` 或 `4 byte` 负载单发测试。

### 10.5 第五步：做 GPIO 时延打点

发送前拉高 `TX_MARK_GPIO`，接收回调中拉高 `RX_MARK_GPIO`。

建议使用：

- `LOW -> HIGH -> LOW` 固定脉冲

不要用：

- `toggle`

因为示波器连续统计时，`toggle` 很容易引入边沿歧义。

## 11. 测试方案

### 11.1 测试目标

验证单向应用层端到端时延是否小于 `1 ms`。

定义：

- 起点：TX 侧业务层调用发包接口前的 GPIO 上升沿
- 终点：RX 侧 `of_sle_on_rx_data()` 中的 GPIO 上升沿

### 11.2 接线

- 示波器 `CH1` 接 TX 板 `TX_MARK_GPIO`
- 示波器 `CH2` 接 RX 板 `RX_MARK_GPIO`
- 两块板与示波器共地

### 11.3 波形测量

示波器设置建议：

- 触发：`CH1` 上升沿
- 时基：先 `200 us/div`
- 采样率：`100 MSa/s` 及以上
- 统计项：`CH1 rise -> CH2 rise`

### 11.4 测试负载

优先测试：

- `1 byte`
- `4 byte`
- `8 byte`
- `20 byte`
- `50 byte`

其中建议主指标看：

- `4 byte` 平均单向时延

### 11.5 测试步骤

1. 两块板分别烧录 TX/RX 固件。
2. 上电后先看串口，确认 client 已扫描并连接 server。
3. 手动触发 TX 发送一个测试包。
4. 示波器抓取两个 GPIO 的时间差。
5. 连续做 20 次单发，记录最小/平均/最大值。
6. 再做固定周期连续发包，例如 `20 ms` 间隔，统计 100 次。

### 11.6 判定标准

建议以 `4 byte` 作为主判断口径：

- 平均时延 `< 1 ms`
- 最大时延尽量 `< 1 ms`

如果只有最小值小于 `1 ms`，但平均值和最大值明显超标，则不能认定满足目标。

## 12. 推荐的项目侧接口设计

为了后续维护，建议不要把测试逻辑直接散落在多个文件里，而是收口成几个接口。

### 12.1 TX 侧

建议接口：

- `int ll_tx_init(void)`
- `int ll_tx_send_test_packet(const uint8_t *buf, uint16_t len)`
- `void ll_tx_fire_mark(void)`

### 12.2 RX 侧

建议接口：

- `int ll_rx_init(void)`
- `void ll_rx_on_sle_data(const uint8_t *data, uint16_t len)`
- `void ll_rx_fire_mark(void)`

### 12.3 GPIO 驱动层

建议抽成公共接口：

- `int ll_mark_gpio_init(void)`
- `void ll_mark_tx_pulse(void)`
- `void ll_mark_rx_pulse(void)`

这样后面要换 pin 或换脉宽时，不会影响业务层。

## 13. 风险与约束

### 13.1 最大风险

如果你坚持“不改 SDK”，那就必须接受一个边界：

- 低时延链路的核心建链与调度参数仍然由 SDK sample 控制

所以如果后续你要微调：

- seek window / interval
- 广播参数
- 特定 PHY 细节
- 更深层 low latency rate

而公共 API 又不够用时，最终仍可能需要走“项目 patch set”而不是“直接改源码”的方式。

但在当前目标下，这一步暂时不需要。

### 13.2 不建议的做法

不建议：

- 继续保留两边都是 client
- 继续保留 normal mode
- 通过大量串口打印做时延测量
- 在 `sdk_root_dir` 里直接长期修改 sample 源码

## 14. 最终推荐方案

最稳妥的落地路径是：

1. 先把 `sle_low_latency_tx` 改成 `server + low_latency`。
2. 把 `sle_low_latency_rx` 改成 `client + low_latency`。
3. 不改 `sdk_root_dir`，直接在两个工程的 `sdk_overlay/custom` 扩展业务层。
4. RX 侧通过 `of_sle_on_rx_data()` 接管接收。
5. TX 侧通过 `sle_uart_server_send_report_by_handle()` 封装项目发包接口。
6. 用 GPIO 双通道示波器测端到端单向时延。

这条路径的优点是：

- 不分叉 SDK sample
- 工程边界清晰
- 后续可维护
- 能直接支撑你要的 `1 ms` 内时延验证

## 15. 下一步建议

如果你要继续实做，建议按下面顺序推进：

1. 先修改两个工程配置。
2. 再补充两个工程的 `sdk_overlay/custom` 目录骨架。
3. 然后实现 RX hook、TX send API、GPIO 打点。
4. 最后上示波器做时延验证。

如果需要，我下一步可以直接继续帮你把这套方案对应的代码骨架建出来，并把两个工程改到可编译状态。
