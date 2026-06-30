# 光枪 SLE 替代无线阶段5：光枪侧 SDK Client 去耦

## 1. 阶段目标

本阶段聚焦消除光枪工程对 `sdk_root_dir` 中 `sle_uart_client.c` 定制改动的依赖，保证后续继续推进 SLE 替代时，不需要再修改 SDK 目录内源码。

阶段范围：

- 将光枪侧使用的 `sle_uart_client` 逻辑收口到项目目录。
- 保持现有对外行为不变：
  - 扫描后连接 dongle
  - 连接态回调 `of_sle_on_link_state()`
  - 继续导出 `sle_uart_start_scan()`、`get_g_sle_uart_send_param()`、`get_g_sle_uart_conn_id()`
- 不改动 `sdk_root_dir`。

## 2. 本阶段整改位置

### 2.1 Gun 工程

- `projects/light_gun_260517/sdk_overlay/custom/CMakeLists.txt`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/of_sle_uart_client_local.c`

### 2.2 文档

- `docs/projects/light_gun_sle_phase5_gun_sdk_client_decouple.md`

## 3. 开发方案

### 3.1 当前问题

当前光枪工程通过 `custom/CMakeLists.txt` 直接编译 SDK 目录下的：

- `sdk_root_dir/application/samples/products/sle_uart/sle_uart_client/sle_uart_client.c`

而这份 SDK 源文件在工作区里已有本地改动，主要用于：

- 上报 `of_sle_on_link_state()`
- 调整低时延参数
- 补充写确认日志

这意味着光枪工程当前仍然隐含依赖 `sdk_root_dir` 内的定制源码，与“不要再动 SDK 目录”的约束冲突。

### 3.2 收口策略

本阶段采用项目内收口策略：

- 将当前需要的 `sle_uart_client.c` 行为复制到 gun 工程本地源文件。
- `custom/CMakeLists.txt` 改为编译项目内本地副本，不再直接引用 SDK 目录下的 `sle_uart_client.c`。
- 保持对 SDK 头文件和底层 API 的正常调用，只去掉对 SDK 源文件改动的依赖。

## 4. 验收标准

- gun 工程不再直接编译 `sdk_root_dir/.../sle_uart_client.c`。
- gun 侧仍能导出并使用：
  - `sle_uart_start_scan()`
  - `get_g_sle_uart_send_param()`
  - `get_g_sle_uart_conn_id()`
- gun 与 dongle 工程串行编译通过。
- 本阶段不修改 `sdk_root_dir`。

## 5. 完成记录

### 5.1 实现结果

- 已将光枪工程对 SDK 目录 `sle_uart_client.c` 的直接源码依赖收口到项目内：
  - `projects/light_gun_260517/sdk_overlay/custom/CMakeLists.txt`
    改为编译本地 `src/transport/of_sle_uart_client_local.c`
  - 新增项目内头文件 `projects/light_gun_260517/sdk_overlay/custom/include/sle_uart_client.h`
- 本地 `of_sle_uart_client_local.c` 保留了光枪 SLE 替代链路当前所需能力：
  - `sle_uart_start_scan()`
  - `get_g_sle_uart_send_param()`
  - `get_g_sle_uart_conn_id()`
  - 连接态上报 `of_sle_on_link_state()`
  - 扫描命中 server 名称后自动连接
  - 连接后继续交换 MTU / 发现 property / 准备写通道
- 结果上，光枪工程后续不需要再依赖 `sdk_root_dir/application/samples/products/sle_uart/sle_uart_client/sle_uart_client.c` 的本地定制版本。
- 本阶段未修改 `sdk_root_dir` 内任何文件。

### 5.2 验证记录

- 2026-06-30 串行执行 gun 构建：
  - 命令：`./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 固件：`projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`ea124b7fc14bb80a5823e18f7aafedffa4ac6c780198620084bdf875365d49df`
- 2026-06-30 串行执行 dongle 构建：
  - 命令：`./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 固件：`projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
- 说明：
  - 两个工程仍需串行构建，不能并行；构建过程会共享 `sdk_root_dir/application/samples/custom` 链接和配置切换。
  - 构建日志中仍有 SDK 自带 warning / `cp: missing destination file operand` 噪声，但不影响本阶段目标达成，最终均成功导出固件包。
