# USB模拟闭环测试说明

本文档对应工程：

- `projects/light_gun_driver_test`

对应源码文件：

- `sdk_overlay/custom/usb_sim_test.c`
- `sdk_overlay/custom/usb_sim_test.h`

## 1. 这套测试是干什么的

这套测试的目标不是一次性把 `OpenFIRE-Firmware-ESP32` 全量 USB 复刻完，而是先在当前 TR531x 工程里做一套：

- 能枚举成 USB HID 键盘 + 鼠标
- 能跑固定脚本，模拟 IR 坐标和 LED 事件
- 能读取物理按键 GPIO 并上报到电脑
- 能验证 OpenFIRE 风格的 `onscreen / offscreen trigger` 行为

这样你后续做二次开发时，可以先把：

- `USB枚举`
- `HID上报`
- `GPIO输入`
- `Trigger业务逻辑`

这四个核心闭环先打通。

## 2. 当前实现范围

当前 USB 模拟闭环测试实现了：

1. USB HID 键盘
2. USB HID 鼠标
3. 串口日志形式的假 LED 控制输出
4. 固定脚本形式的假 IR 坐标输出
5. 物理 GPIO 按键透传
6. OpenFIRE 风格的扳机屏内 / 屏外处理

当前没有实现：

1. USB CDC 串口设备
2. 真正的绝对坐标鼠标 HID
3. USB Gamepad HID

原因很直接：

- 当前工程已有 USB HID gadget 配置，适合先跑键盘/鼠标双 report map
- 当前 `CONFIG_DRIVERS_USB_HID_REPORT_MAP_NUM=2`，正好对应键盘 + 鼠标
- 你给的 `Gamepad` 物理引脚值也还没填，所以先做预留，不强行接第三类 HID

## 3. 物理按键映射

当前默认 GPIO 映射如下：

- `A` -> `GPIO23`
- `B` -> `GPIO22`
- `Trigger` -> `GPIO21`
- `START` -> `GPIO18`
- `SELECT` -> `GPIO17`
- `HOME` -> `GPIO16`
- `KEY_UP` -> `GPIO27`
- `KEY_DOWN` -> `GPIO28`
- `KEY_LEFT` -> `GPIO29`
- `KEY_RIGHT` -> `GPIO30`
- `KEY_MIDDLE` -> `GPIO0`
- `Gamepad` -> 默认 `255`，表示未启用

GPIO 输入初始化策略：

1. 复用为 GPIO
2. 使能输入
3. 配置为 `INPUT_PULLUP`
4. 按键按下判定为低电平

所以程序员视角可以直接理解为：

- 默认高电平
- 按下变低
- 松开恢复高

## 4. OpenFIRE 风格 trigger 是怎么模拟的

这部分是本次测试里最重要的业务逻辑。

### 4.1 onscreen fire

含义：

- 枪当前“看得到屏幕”
- 这时扣扳机，按正常开火处理

当前测试里的上报行为：

- `Trigger DOWN` -> 鼠标左键按下
- `Trigger UP` -> 鼠标左键释放

### 4.2 offscreen fire

含义：

- 枪当前“没看到屏幕”
- 也就是模拟 OpenFIRE 里的脱靶 / 屏外状态

当前测试支持两种策略，通过 Kconfig 选：

1. `Offscreen trigger suppressed`
   含义：
   屏外扣扳机时不上报点击，只打日志。

2. `Offscreen trigger -> right click`
   含义：
   屏外扣扳机时改成鼠标右键。

这更接近 OpenFIRE 常见的“屏外开火 = 换弹 / 右键动作”的测试思路。

## 5. 假 IR 坐标是怎么做的

因为你现在没有 IR 传感器，所以这里不是采真实点，而是内置一组固定点序列。

示例行为：

1. 输出一个屏内点 `(200,120)`
2. 再输出 `(360,120)`
3. 再输出 `(360,260)`
4. 再输出 `(200,260)`
5. 再切一帧 `OFFSCREEN`
6. 然后继续下一组点

串口会打印类似：

```text
[usb_sim_test] fake_ir -> onscreen point=(200,120)
[usb_sim_test] fake_ir -> OFFSCREEN
```

注意：

- 当前 HID 用的是相对鼠标描述符，不是绝对坐标鼠标
- 所以代码内部会把“固定坐标点”转换成“相邻两点的增量移动”发给电脑

这样做的好处是：

- 不需要你先改一套绝对坐标 HID 描述符
- 电脑上立刻能看到鼠标移动和点击
- 方便先闭环联调 USB 链路

## 6. 假 LED 是怎么做的

因为你现在没有 LED 控制外设，所以这里不做真 GPIO / PWM 控灯，而是直接输出日志：

```text
[usb_sim_test] LED1 ON
[usb_sim_test] LED2 OFF
```

这样验证的是：

- 上层“LED业务事件有没有被调度”
- 时序有没有按预期走

不是验证真实发光硬件。

## 7. Kconfig 功能开关说明

新增了一个测试对象：

- `USB simulated closed-loop test`

现在不再用单选 `full-cycle test`，而是改成多个独立开关。

你可以分别开启：

1. `Enable fake IR script`
   验证：
   - 固定 IR 点脚本是否按节拍运行
   - 屏内 / 屏外状态是否切换

2. `Enable fake LED log script`
   验证：
   - LED 业务事件是否按节拍输出日志

3. `Enable scripted mouse move`
   验证：
   - 鼠标移动路径是否能独立观察

4. `Enable scripted mouse click`
   验证：
   - 鼠标左键点击是否能独立观察

5. `Enable scripted keyboard report`
   验证：
   - Enter / Esc 键盘报文是否能独立观察

6. `Enable physical button passthrough`
   验证：
   - 物理按键 GPIO 去抖
   - 鼠标/键盘 HID 实时透传

7. `Enable OpenFIRE-style trigger reference`
   验证：
   - 系统自动在 `onscreen / offscreen` 之间切换
   - 你手按 Trigger 时，上报是否跟随状态变化

8. `Use live IR solve to drive USB mouse`
   验证：
   - 真实 IR 解算结果是否能直接驱动 USB 鼠标移动
   - 屏内 / 屏外状态是否跟随真实 IR 结果变化

程序员视角可以这样理解：

- 这些开关是“功能位”
- 不是“模式枚举”
- 你勾选一个，就只跑那一块
- 你全勾上，效果就等价于以前的 `full cycle`

补充说明：

- `Enable OpenFIRE-style trigger reference`
  - 现在只负责切换 `ONSCREEN / OFFSCREEN` 状态
  - 不会再间接替你开启脚本化鼠标移动

- `Enable fake IR script`
  - 现在只负责生成假坐标脚本和日志
  - 只有 `Enable scripted mouse move` 打开时，才会真的把这些坐标转换成鼠标移动报文

- `Use live IR solve to drive USB mouse`
  - 打开后，真实 IR 解算结果会直接驱动 USB 鼠标移动
  - 为了避免两套鼠标业务互相打架，运行时会自动停用：
    `fake IR`、`scripted mouse move`、`scripted mouse click`、`trigger reference`
  - 也就是说，启用这项后就不会再跑“模拟鼠标移动和点击”

- USB 链路探测
  - 会优先探测当前真正启用的 HID 路径
  - 比如 `scripted mouse move=off` 时，会尽量优先走键盘探测，减少 `mouse report send failed` 这类串口干扰

## 8. 物理按键默认映射到什么 HID

当前映射如下：

- `Trigger`
  - 屏内：鼠标左键
  - 屏外：按 Kconfig 选择，默认鼠标右键

- `A`
  - 鼠标右键

- `B`
  - 鼠标中键

- `START`
  - 键盘 `Enter`

- `SELECT`
  - 键盘 `Esc`

- `HOME`
  - 键盘 `Home`

- `KEY_UP`
  - 键盘 `Up`

- `KEY_DOWN`
  - 键盘 `Down`

- `KEY_LEFT`
  - 键盘 `Left`

- `KEY_RIGHT`
  - 键盘 `Right`

- `KEY_MIDDLE`
  - 键盘 `B`

说明：

- `KEY_MIDDLE` 这里暂时映射成键盘 `B`
- 只是为了给你一个可见的闭环测试键值
- 如果你后面要改成别的键或业务动作，直接改 `usb_sim_test.c` 里的表就行

## 9. USB驱动配置说明

这次实现依赖的 USB 配置，当前工程里已经具备关键项：

- `CONFIG_DRIVERS_USB=y`
- `CONFIG_DRIVERS_USB_GADGET=y`
- `CONFIG_DRIVERS_USB_HID_GADGET=y`
- `CONFIG_DRIVERS_USB_HID_FUNC_INTERFACE=y`
- `CONFIG_DRIVERS_USB_HID_REPORT_MAP_NUM=2`
- `CONFIG_DRIVERS_USB_COMPOSITE_GADGET=y`

所以这次代码直接走：

- `usb_init(DEVICE, DEV_HID)`

不是：

- `DEV_SER_HID`

也不是：

- `DEV_SERIAL`

这意味着当前电脑端看到的重点会是：

- 一个 HID 键盘
- 一个 HID 鼠标

而不是额外再多一个 CDC 串口。

## 10. 你怎么做闭环验证

建议按下面顺序来。

### 10.1 先测 USB 是否枚举成功

把测试对象切到：

- `USB simulated closed-loop test`

然后优先选：

- `Script mouse + keyboard HID`

验证点：

1. 板子插到电脑后，系统是否识别 HID 设备
2. 串口日志里是否打印：
   - `USB HID ready`
3. 电脑上是否能看到鼠标点击 / Enter / Esc 事件

### 10.2 再测假 IR + 假 LED

打开：

- `Enable fake IR script`
- `Enable fake LED log script`

验证点：

1. 串口是否周期打印 `LEDx ON/OFF`
2. 串口是否周期打印 `fake_ir`
3. 如果同时打开 `Enable scripted mouse move`，电脑上鼠标是否随脚本路径移动

如果 `Enable scripted mouse move` 关闭：

- 串口仍然可以看到 `fake_ir -> ...`
- 但电脑端不应该再出现脚本化鼠标移动

### 10.3 再测物理按键

打开：

- `Enable physical button passthrough`

验证点：

1. 每个 GPIO 按键按下时，串口是否打印 `button XXX DOWN`
2. 松开时，是否打印 `UP`
3. 电脑上是否看到对应鼠标/键盘事件

### 10.4 最后测 trigger 参考模式

打开：

- `Enable OpenFIRE-style trigger reference`

验证点：

1. 串口是否每隔一段时间切换：
   - `ONSCREEN`
   - `OFFSCREEN`
2. 你按下 Trigger 时：
   - 屏内是否变成左键
   - 屏外是否变成右键或被抑制

## 11. 当前实现的边界

这套代码现在更偏“工程联调治具”，不是量产级完整 lightgun 固件。

当前边界主要有三点：

1. 鼠标是相对坐标，不是绝对坐标

2. 没做 USB CDC 串口协议
3. Gamepad 只预留，没有真正上报

## 12. 关于“我关了鼠标移动，为什么串口还像在做鼠标业务”

现在代码已经做过两轮解耦：

1. `trigger_ref` 不再间接触发鼠标移动
2. `fake_ir` 不再默认触发鼠标移动

另外又加了一层链路探测收紧：

1. USB 链路探测会优先探测当前真正启用的 HID 路径
2. 当 `scripted mouse move=off` 且当前主要是键盘类测试时，会尽量优先走键盘探测

所以当你关闭：

- `Enable scripted mouse move`

之后，正常预期是：

1. 串口可以继续看到 `fake_ir`
2. 串口可以继续看到 `trigger reference state`
3. 但不会再因为这两者导致脚本化鼠标移动

如果你下一步要继续做，我建议优先级是：

1. 把鼠标改成绝对坐标 HID
2. 把 Gamepad 第三份 report map 加进去
3. 再补一个串口控制面，模拟 OpenFIRE Docked / MameHooker 风格命令链路
