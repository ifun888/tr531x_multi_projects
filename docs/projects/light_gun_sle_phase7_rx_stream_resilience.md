# 光枪 SLE 替代无线阶段7：收包流健壮性补齐

## 1. 阶段目标

本阶段聚焦补齐 SLE 无线流解析在异常帧场景下的健壮性，避免单个坏包让同一批次后续好包延后到下一轮 runtime 才能继续处理。

阶段范围：

- 不改变 `of_wireless_pkt` 帧格式。
- 不改变现有业务包语义。
- 仅修正 gun / dongle 两端 runtime 的无线流解析循环。
- 对解析失败增加最小诊断。
- 不改动 `sdk_root_dir`。

## 2. 本阶段整改位置

### 2.1 Gun 工程

- `projects/light_gun_260517/sdk_overlay/custom/src/app/of_runtime.c`

### 2.2 Dongle 工程

- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/app/of_runtime.c`

### 2.3 文档

- `docs/projects/light_gun_sle_phase7_rx_stream_resilience.md`

## 3. 开发方案

### 3.1 当前问题

当前 gun / dongle 两端 runtime 对无线流的处理都采用：

- `while (of_wireless_stream_next(...) > 0) { ... }`

而 `of_wireless_stream_next()` 在以下场景会返回 `-1`：

- CRC 校验失败
- payload 超出接收缓存能力

虽然底层流缓冲已经丢弃了异常帧或推进了同步位置，但上层循环会因为 `-1` 立刻退出，导致同一批数据中后续已经到达的好包不能继续立刻处理。

### 3.2 收口策略

本阶段采用最小侵入策略：

- 将两端 runtime 的无线流解析改为显式 `for (;;)`/`switch rc` 模式。
- `rc == 0` 时退出，表示当前没有完整包。
- `rc < 0` 时记录一次解析失败日志并继续尝试拉取后续包。
- `rc > 0` 时按原业务逻辑处理。

## 4. 验收标准

- gun / dongle 两端遇到单个坏包后，当前轮次仍可继续处理后续好包。
- 两端对解析失败不再完全静默。
- 两个工程串行编译通过。
- 本阶段不修改 `sdk_root_dir`。

## 5. 完成记录

### 5.1 实现结果

- 已修正 gun / dongle 两端 runtime 的无线流解析循环，不再使用：
  - `while (of_wireless_stream_next(...) > 0)`
- 现改为显式判断返回值：
  - `0`：当前没有完整包，退出循环
  - `< 0`：当前帧解析失败，记录一次日志后继续尝试后续包
  - `> 0`：继续按原业务逻辑处理
- 这样在出现 CRC 错误或异常长度帧时，不会因为单个坏包让同批次后续好包被延后到下一轮 runtime 才处理。
- 两端新增一次性诊断日志：
  - gun：`[openfire][gun] malformed wireless frame dropped`
  - dongle：`[openfire][dongle] malformed wireless frame dropped`
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
  - SHA256：`80d1095ca0eaf0f20cf91a69b7112efa32f90ca99452852bd8d2096faab74c4e`
- 说明：
  - 两个工程仍需串行构建，不能并行；构建过程会共享 `sdk_root_dir/application/samples/custom` 链接和配置切换。
  - 本阶段验证重点是异常帧场景下的收包循环健壮性回归；实机坏包注入验证可在后续联调阶段继续做。
