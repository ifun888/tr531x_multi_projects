# USB模拟闭环测试说明

本文档对应工程：

- `projects/light_gun_driver_test`

对应源码文件：

- `sdk_overlay/custom/usb_sim_test.c`
- `sdk_overlay/custom/usb_sim_test.h`
- `sdk_overlay/custom/Kconfig`

## 1. 当前实现目标

这套测试现在不再只是 “键盘 + 鼠标” 的 USB 治具，而是扩成一条更接近 OpenFIRE 的单 HID 复合路径：

- 一个 HID 接口
- 一个 HID IN endpoint
- 三个 Report ID
  - `Keyboard`
  - `Mouse`
  - `Gamepad`
- 可选再叠加一个 `CDC ACM`

这样做的目的有两个：

1. 保持 TR5310 端点占用稳定，不再新增 HID interface / endpoint
2. 给后续 `MAME / TeknoParrot / 双枪` 这类需要 gamepad 的场景留出联调路径

## 2. 当前 USB 枚举形态

当前工程支持两种设备模式：

1. `HID only`
2. `CDC ACM + HID`

无论哪种模式，HID 侧都走同一个组合 report descriptor，包含：

- `Report ID 0x01`：Keyboard
- `Report ID 0x04`：Mouse
- `Report ID 0x05`：Gamepad

如果 `CONFIG_LIGHT_GUN_USB_HID_GAMEPAD=y`，会使用新的 PID：

- `0x0022`

这样做是为了减少 Windows 复用旧 HID 描述符缓存带来的干扰。

## 3. 当前 Gamepad 设计

这次落地的 gamepad 不是 OpenFIRE 的全量 `6 轴 + 32 键` 版本，而是先上一个更适合 TR5310 测试工程的最小可用版本：

- `X`：16-bit absolute
- `Y`：16-bit absolute
- `Rx`：16-bit absolute
- `Ry`：16-bit absolute
- `Hat`：8-way D-pad
- `Buttons`：16-bit 位图

当前实际使用里：

- 瞄准坐标主要驱动 `X / Y`
- `Rx / Ry` 先保留为 0
- `Hat` 由方向键 GPIO 驱动
- `Buttons` 由 Trigger / A / B / Start / Select / Home / Gamepad GPIO 驱动

## 4. GPIO 到 HID 的默认映射

当前默认 GPIO 输入映射：

- `Trigger` -> `GPIO21`
- `A` -> `GPIO23`
- `B` -> `GPIO22`
- `START` -> `GPIO18`
- `SELECT` -> `GPIO17`
- `HOME` -> `GPIO16`
- `KEY_UP` -> `GPIO27`
- `KEY_DOWN` -> `GPIO28`
- `KEY_LEFT` -> `GPIO29`
- `KEY_RIGHT` -> `GPIO30`
- `KEY_MIDDLE` -> `GPIO0`
- `GAMEPAD` -> `GPIO255`，默认禁用

GPIO 初始化策略不变：

1. 复用为 GPIO
2. 使能输入
3. 上拉
4. 低电平视为按下

### 4.1 鼠标映射

- `Trigger`
  - 屏内：鼠标左键
  - 屏外：按 Kconfig 配置，默认鼠标右键
- `A`：鼠标右键
- `B`：鼠标中键

### 4.2 键盘映射

- `START`：`Enter`
- `SELECT`：`Esc`
- `HOME`：`Home`
- `KEY_UP`：`Up`
- `KEY_DOWN`：`Down`
- `KEY_LEFT`：`Left`
- `KEY_RIGHT`：`Right`
- `KEY_MIDDLE`：`B`

### 4.3 Gamepad 映射

- `Trigger`：Button 1
- `A`：Button 2
- `B`：Button 3
- `GAMEPAD`：Button 4
- `SELECT`：Button 11
- `START`：Button 12
- `HOME`：Button 13
- `KEY_UP / DOWN / LEFT / RIGHT`：D-pad Hat

说明：

- 这里的 gamepad button 编号是测试工程内部约定，host 侧通常会把它解释成标准手柄按钮序号
- `GAMEPAD` 引脚默认还是未启用，只有你填成真实 GPIO 后才会参与按键扫描

## 5. IR 如何驱动 Mouse / Gamepad

### 5.1 Mouse

Mouse 仍然是相对坐标 HID。

- 假 IR 脚本：使用相邻两点的增量，转成鼠标 `dx / dy`
- 真实 IR：使用 `screen_x / screen_y` 的帧间差值，转成鼠标 `dx / dy`

### 5.2 Gamepad

Gamepad 走绝对坐标。

- 假 IR 脚本：把脚本点 `(x, y)` 映射到 `-32767..32767`
- 真实 IR：把 `screen_x / screen_y` 映射到 `-32767..32767`

当前约定：

- `onscreen`：正常更新 gamepad `X / Y`
- `offscreen`：保持上一帧绝对坐标，不强制回中

这更适合模拟器场景，避免屏外时准星突然跳回中心。

## 6. 输出模式

新增了一个输出模式选择：

1. `Mouse only`
2. `Gamepad only`
3. `Mouse + gamepad`

含义：

- `Mouse only`
  - 只发鼠标
  - 适合桌面联调

- `Gamepad only`
  - 只发 gamepad
  - 适合模拟器、双枪和避免桌面鼠标乱跑的场景

- `Mouse + gamepad`
  - 两条链路同时发
  - 适合开发阶段观察 HID 行为

## 7. Live IR 运行逻辑

现在分成两条独立开关：

- `Use live IR solve to drive USB mouse`
- `Use live IR solve to drive HID gamepad`

如果任意一条 live IR 打开：

- `fake IR`
- `scripted mouse move`
- `scripted mouse click`
- `trigger reference`

会在运行时自动停用，避免脚本和真实 IR 抢同一套指向状态。

## 8. CDC 最小回显测试

如果设备模式是 `CDC ACM + HID`，并且：

- `CONFIG_LIGHT_GUN_USB_CDC_ECHO_TEST=y`

会额外创建一条最小 CDC 回显线程：

1. 启动后尝试打印一条 banner
2. 主机发送任意字节后，设备原样回显

这条测试的目的不是协议通信，而是确认：

- CDC 不只是枚举成功
- `usb_serial_read() / usb_serial_write()` 真能收发

## 9. 关键 Kconfig

这次 USB gamepad 落地新增或依赖的关键配置：

- `CONFIG_LIGHT_GUN_USB_HID_GAMEPAD`
- `CONFIG_LIGHT_GUN_USB_OUTPUT_MODE_MOUSE_ONLY`
- `CONFIG_LIGHT_GUN_USB_OUTPUT_MODE_GAMEPAD_ONLY`
- `CONFIG_LIGHT_GUN_USB_OUTPUT_MODE_MOUSE_GAMEPAD`
- `CONFIG_LIGHT_GUN_USB_SIM_USE_IR_LIVE_GAMEPAD`
- `CONFIG_LIGHT_GUN_USB_CDC_ECHO_TEST`
- `CONFIG_LIGHT_GUN_USB_DEBUG`

当前默认测试配置 `standard_tr5310_s.config` 中：

- `CDC ACM + HID`：开启
- `HID Gamepad`：开启
- `Mouse + gamepad`：开启
- `Live IR -> Mouse`：开启
- `Live IR -> Gamepad`：开启
- `CDC echo`：开启
- `USB_DEBUG`：关闭

## 10. 推荐验证顺序

### 10.1 先看枚举

看调试串口是否出现：

- `USB CDC ACM + HID ready`

如果开了 CDC echo，还会看到：

- `CDC echo task started`

### 10.2 看 CDC 是否真收发

在主机侧打开 CDC 串口后：

1. 是否能先收到 banner
2. 随便输入几个字节
3. 是否原样回显

### 10.3 看 mouse / keyboard 是否还正常

验证：

- `Trigger / A / B` 的鼠标按键
- `START / SELECT / HOME / 方向键` 的键盘事件

### 10.4 看 gamepad 是否起来

主机侧打开 `gamepad tester` 或模拟器输入配置界面，验证：

1. `Trigger / A / B / Start / Select / Home` 是否映射到 gamepad buttons
2. `KEY_UP / DOWN / LEFT / RIGHT` 是否驱动 hat
3. 真实 IR 是否驱动 `X / Y`

## 11. 当前边界

当前实现还不是完整 OpenFIRE USB 复刻，边界如下：

1. `Gamepad` 先实现的是 `4轴 + hat + 16键`，不是 OpenFIRE 的 `6轴 + 32键`
2. `Rx / Ry` 目前保留，没有接真实摇杆
3. 真实 `MameHooker / Docked command` 之类上位机协议还没做
4. 屏外状态下 gamepad 只保持最后坐标，没有做单独“离屏按钮策略”

## 12. 对应文档

如果你要看这次 gamepad 落地方案和改动位置，继续看：

- `docs/USB_HID_Gamepad落地方案.md`
