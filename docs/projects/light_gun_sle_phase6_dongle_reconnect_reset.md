# 光枪 SLE 替代无线阶段6：Dongle 回搜前主动断链

## 1. 阶段目标

本阶段聚焦补齐 dongle 侧异常恢复时的“真回搜”行为，避免链路异常后仅重新广播、但旧连接未被主动清理的半恢复状态。

阶段范围：

- 保持现有 `of_wireless_pkt` 业务格式不变。
- 不改动光枪业务协议和 USB 语义。
- 仅修正 dongle 在 `force_reconnect` 路径上的恢复动作。
- 不改动 `sdk_root_dir`。

## 2. 本阶段整改位置

### 2.1 Dongle 工程

- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/transport/transport_sle_sdk_hook.c`

### 2.2 文档

- `docs/projects/light_gun_sle_phase6_dongle_reconnect_reset.md`

## 3. 开发方案

### 3.1 当前问题

当前光枪侧 `of_sle_sdk_force_reconnect()` 会先调用：

- `sle_disconnect_all_remote_device()`

然后再重新进入扫描。

但 dongle 侧同名路径当前只做：

- `sle_start_announce(1U)`

这意味着当 dongle 因为：

- 业务握手超时
- 连续 TX fault

而触发 `drv_sle_link_request_search()` 时，应用层虽然把本地状态复位成“搜索中”，但底层旧连接未必真的被主动断开，存在进入伪回搜的风险。

### 3.2 收口策略

本阶段采用最小改动策略：

- dongle 侧 `of_sle_sdk_force_reconnect()` 先调用 `sle_disconnect_all_remote_device()`。
- 断链请求后再重新进入广播。
- 不改变初始开机广播路径，只修正异常恢复路径。

## 4. 验收标准

- dongle 侧 `force_reconnect` 不再仅仅重新广播。
- 异常恢复路径会先主动断开旧 SLE 连接，再重新广播。
- gun 与 dongle 工程串行编译通过。
- 本阶段不修改 `sdk_root_dir`。

## 5. 完成记录

### 5.1 实现结果

- 已在 `projects/light_gun_dongle_260517/sdk_overlay/custom/src/transport/transport_sle_sdk_hook.c`
  修正 dongle 侧异常恢复路径：
  - `of_sle_sdk_force_reconnect()` 不再只做 `sle_start_announce(1U)`
  - 改为先调用 `sle_disconnect_all_remote_device()` 主动断开旧 SLE 连接
  - 然后再重新进入广播
- 这样当 dongle 因为 `ready timeout` 或连续 `TX fault` 触发 `drv_sle_link_request_search()` 时，底层恢复动作与应用层“回搜”语义一致，不再停留在“状态上回搜、物理连接未清”的伪恢复状态。
- 本阶段未修改 `sdk_root_dir` 内任何文件。

### 5.2 验证记录

- 2026-06-30 串行执行 dongle 构建：
  - 命令：`./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 固件：`projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`
- 2026-06-30 串行执行 gun 构建：
  - 命令：`./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 固件：`projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`ea124b7fc14bb80a5823e18f7aafedffa4ac6c780198620084bdf875365d49df`
- 说明：
  - 两个工程仍需串行构建，不能并行；构建过程会共享 `sdk_root_dir/application/samples/custom` 链接和配置切换。
  - 本阶段验证重点是恢复路径修正后的双工程编译回归，运行时断链恢复联调可在下一阶段继续做实机确认。
