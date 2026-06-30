# USB HID Gamepad落地方案

本文档说明 `light_gun_driver_test` 中把 OpenFIRE 风格 gamepad 能力落到 TR5310 的设计决策、改动位置和实现边界。

## 1. 设计目标

目标不是移植 TinyUSB，也不是复刻 OpenFIRE 全量 USB 栈，而是在当前 TR5310 USB gadget 框架内做到：

- 保持现有 `CDC ACM + HID` 可用
- HID 继续只有一个接口、一个 IN endpoint
- 在同一个 HID report descriptor 中追加 `gamepad report`
- 让真实 IR 和 GPIO 都能驱动 gamepad 数据

## 2. 为什么不用“再加一个 HID 接口”

TR5310 当前 USB 资源更适合：

- `CDC interrupt IN`
- `CDC bulk IN`
- `CDC bulk OUT`
- `HID IN`

也就是说，HID 最稳妥的做法是继续维持单接口单 IN endpoint，把键盘、鼠标、gamepad 都塞进同一个 descriptor，用 `Report ID` 区分。

这和 OpenFIRE 的方向是一致的，只是我们的 gadget 框架和 TinyUSB 不同。

## 3. 最终方案

### 3.1 HID report 组成

一个 HID descriptor，挂三类 report：

- `0x01`：Keyboard
- `0x04`：Mouse
- `0x05`：Gamepad

### 3.2 Gamepad report 结构

当前实现先做：

- `X`
- `Y`
- `Rx`
- `Ry`
- `Hat`
- `16 buttons`

原因：

1. 足够覆盖 light gun 联调
2. 比 `6轴 + 32键` 更容易先验证 host 兼容性
3. 不会把现有测试工程复杂度一下拉太高

### 3.3 输出模式

新增三种输出模式：

- `Mouse only`
- `Gamepad only`
- `Mouse + gamepad`

其中：

- `Mouse only` 更适合桌面和普通 HID 联调
- `Gamepad only` 更适合模拟器、多枪和避免桌面鼠标乱跑
- `Mouse + gamepad` 适合开发阶段同时看两条链路

## 4. IR 映射策略

### 4.1 Mouse

Mouse 保持相对坐标：

- `dx = 当前帧 screen_x - 上一帧 screen_x`
- `dy = 当前帧 screen_y - 上一帧 screen_y`

### 4.2 Gamepad

Gamepad 走绝对坐标：

- `screen_x -> -32767..32767`
- `screen_y -> -32767..32767`

当前采用：

- `onscreen`：更新 `X / Y`
- `offscreen`：保持最后绝对值

## 5. GPIO 映射策略

GPIO 现在同时可以驱动多条 HID 路径：

- 鼠标按钮
- 键盘按键
- gamepad buttons
- gamepad hat

方向键这次不是只发键盘箭头，还会同步更新 gamepad hat。

## 6. 代码改动位置

### 6.1 项目配置

- `config/standard_tr5310_s.config`

### 6.2 Kconfig

- `sdk_overlay/custom/Kconfig`

新增：

- `LIGHT_GUN_USB_HID_GAMEPAD`
- `LIGHT_GUN_USB_OUTPUT_MODE_*`
- `LIGHT_GUN_USB_SIM_USE_IR_LIVE_GAMEPAD`

### 6.3 USB HID 业务代码

- `sdk_overlay/custom/usb_sim_test.c`
- `sdk_overlay/custom/usb_sim_test.h`

主要改动点：

1. 组合 HID descriptor 新增 `gamepad report`
2. 新增 `usb_sim_gamepad_report_t`
3. 新增 gamepad state、button、hat、axis 映射
4. 假 IR / 真实 IR 新增绝对坐标输出路径
5. GPIO 新增 gamepad button / hat 输出
6. 链路探测新增 gamepad report 探测

### 6.4 文档

- `docs/USB模拟闭环测试说明.md`
- `docs/USB_HID_Gamepad落地方案.md`

## 7. 当前边界

这次实现明确不做：

1. TinyUSB 移植
2. 新的 HID interface / endpoint
3. OpenFIRE 全量 `6轴 + 32键`
4. 上位机协议层，例如 MameHooker 串口协议

## 8. 后续扩展点

如果这版验证稳定，下一步可按顺序扩：

1. `Rx / Ry` 接实体摇杆
2. `16 buttons` 扩成 `32 buttons`
3. `4轴` 扩成 `6轴`
4. 补 host 侧命令通道
