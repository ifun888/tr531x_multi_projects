# 光枪 SLE 替代无线阶段3：协议去身份化收敛

## 1. 阶段目标

本阶段聚焦把项目侧业务协议里仍残留的“旧无线身份模型”入口清掉，确保当前方案真正收敛到：

- 光枪与 dongle 的无线链路只承担 SLE 数据承载。
- 不再存在 USB 身份交换入口。
- 项目侧 profile 不再暴露 `USB_ID` 这一类可写业务项。
- 保持现有 NV/存储布局兼容，避免因为直接缩表导致已有数据错位。

## 2. 本阶段整改位置

### 2.1 光枪工程

- `projects/light_gun_260517/sdk_overlay/custom/include/services/svc_profile.h`
- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_profile.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/protocols/proto_docked.c`

### 2.2 Dongle 工程

- `projects/light_gun_dongle_260517/sdk_overlay/custom/include/services/svc_profile.h`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/services/svc_profile.c`
- `projects/light_gun_dongle_260517/sdk_overlay/custom/src/protocols/proto_docked.c`

## 3. 开发方案

### 3.1 业务边界

当前链路已经完成：

- SLE 建链
- `HELLO / HELLO_ACK` 业务 ready
- `SERIAL_TUNNEL / HID / TELEMETRY` 承载

因此本阶段不继续改链路层，而是清理业务协议中不该再存在的身份项。

### 3.2 协议处理策略

对 `OF_CMD_COMMIT_ID (0xB0)` 的处理策略改为：

- 不再写入 profile。
- 明确返回失败状态，提示该命令在当前 SLE 方案下已废弃。

这样可以避免系统继续接受“USB 身份写入”这种旧模型行为。

### 3.3 存储兼容策略

`svc_profile` 的持久化结构体仍保留原有字节槽位，但改名为保留区：

- 不再通过 `OF_CFG_USB_ID` 暴露给业务层。
- 不再在协议层读写。
- 继续占位，避免已有存储镜像直接错位。

## 4. 验收标准

- 项目侧不再存在 `OF_CFG_USB_ID` 业务枚举。
- `proto_docked` 不再把 `0xB0` 写入 profile。
- 两个工程编译通过。
- 不改动 `sdk_root_dir`。

## 5. 完成记录

### 5.1 实现结果

- `svc_profile` 不再对业务层暴露 `OF_CFG_USB_ID`。
- profile 持久化结构仍保留原 18 字节占位，但改名为保留区，继续兼容已有存储布局。
- 光枪与 dongle 两端 `proto_docked` 的 `OF_CMD_COMMIT_ID (0xB0)` 已改为废弃命令：
  - 不再写入 profile
  - 返回 `status=0x01`，明确表示当前 SLE 方案下不支持该身份写入入口
- 其余配置项、校准项、绑定项和链路业务路径保持不变。

### 5.2 验证记录

- `./tools/tr531x/build_project.sh --project light_gun_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 产物：`projects/light_gun_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`9d1fd9d4a5ae953a9b813bd7a79723a04198eedc5a8c3ce5507bcfe6042dbded`
- `./tools/tr531x/build_project.sh --project light_gun_dongle_260517 --target standard-tr5310-s -j 4`
  - 结果：成功
  - 产物：`projects/light_gun_dongle_260517/output/standard-tr5310-s/fwpkg/tr5310_all_in_one.fwpkg`
  - SHA256：`673b9ae23e170560b990f3e16f549d4ba9aee11db6cb8556e561931555d90623`

补充说明：

- 两个工程仍需串行构建，原因是构建脚本会共享切换 `sdk_root_dir/application/samples/custom` 软链接。
- 本阶段仍未继续修改 `sdk_root_dir`。
