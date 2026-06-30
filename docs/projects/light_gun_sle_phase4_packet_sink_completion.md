# 光枪 SLE 替代无线阶段4：业务包落点补齐

## 1. 阶段目标

本阶段聚焦把光枪端已经发出的无线业务包，在 dongle 端补齐最终落点，避免“链路能发，但末端没人接”的半成品状态。

阶段范围：

- 保持现有 `of_wireless_pkt` 包格式不变。
- 重点补齐 `TELEMETRY` 在 dongle 端的处理。
- 对当前未支持的无线包类型增加最小诊断，避免静默丢包。
- 不改动 `sdk_root_dir`。

## 2. 本阶段整改位置

### 2.1 Dongle 工程

- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c`

### 2.2 文档

- `docs/projects/light_gun_sle_phase4_packet_sink_completion.md`

## 3. 开发方案

### 3.1 当前缺口

光枪端当前已经会通过 SLE 发送：

- `SERIAL_TUNNEL`
- `HID_MOUSE`
- `HID_KEYBOARD`
- `TELEMETRY`

但 dongle 端运行时只处理了：

- `SERIAL_TUNNEL -> USB CDC`
- `HID_MOUSE -> USB HID`
- `HID_KEYBOARD -> USB HID`

`TELEMETRY` 尚未落到 PC 侧，属于未闭环路径。

### 3.2 收口策略

本阶段采用最小改动策略：

- `TELEMETRY` 直接透传到 dongle 本地 USB CDC。
- 保持 payload 原样，不做重新封装。
- 若收到当前未支持的包类型，输出一次诊断日志，避免静默丢失。

## 4. 验收标准

- dongle 端能处理 `TELEMETRY` 包。
- `TELEMETRY` payload 可通过 USB CDC 向 PC 侧输出。
- 对未支持包类型不再静默吞掉。
- 两个工程编译通过。

## 5. 完成记录

### 5.1 实现结果

- 已在 `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c` 补齐 `OF_WPKT_TYPE_TELEMETRY` 落点：
  - dongle 从 SLE 收到该业务包后，直接将 payload 原样写入本地 USB CDC。
  - 不新增二次封装，不改变 payload 语义，满足“链路替换、业务内容不变”的阶段目标。
- 为避免后续联调时出现静默丢包，新增未支持无线包类型的一次性诊断日志：
  - `"[openfire][dongle] unsupported wireless pkt type=%u len=%u\r\n"`
- 本阶段未修改 `sdk_root_dir` 内任何文件；工程内现有 `sdk_root_dir` 脏改动均保持不动。

### 5.2 验证记录

- 2026-06-30 串行执行 dongle 构建：
  - 命令：`./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 固件：`projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`8bef5205f021a27f19793b7827a6416633b2afb86829870e83717a9928fc4f6e`
- 2026-06-30 串行执行 gun 构建：
  - 命令：`./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 固件：`projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`4e6dc0b33001626208e32861524a832efa57b73b0b36b7f4497861cb7197c6ca`
- 说明：
  - 两个工程必须串行构建，不能并行；构建脚本会共享 `sdk_root_dir/application/samples/custom` 链接和配置切换，并行时会互相踩踏。
  - 本阶段验证以编译通过和产物校验为准，运行时联调留到下一阶段继续补充。
