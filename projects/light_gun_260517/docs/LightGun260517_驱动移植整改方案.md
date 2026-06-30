# LightGun260517 驱动移植整改方案

本文档目标：

- 对比 `projects/light_gun_driver_test` 与 `projects/light_gun_260517`
- 聚焦你已经验证通过的几块能力：
  - 电磁铁
  - 振动马达
  - IR 传感器
  - 按键
  - LED
  - USB
- 给出适合迁移到 `light_gun_260517` 正式工程结构里的整改方案

结论先说：

- `light_gun_driver_test` 更像“单板闭环验证工程”，代码是围绕“跑通硬件”写的
- `light_gun_260517` 更像“正式产品骨架工程”，代码已经分成了 `drivers / services / protocols / sm / transport`
- 所以不能直接把测试文件整个拷进去替换，而是要把“已经验证正确的硬件驱动逻辑”拆出来，接到 `260517` 现有驱动层和运行时链路里

---

## 1. 对比总览

### 1.1 `light_gun_driver_test` 里已经验证过的来源文件

- 电磁铁：
  - `projects/light_gun_driver_test/sdk_overlay/custom/solenoid_test.c`
  - `projects/light_gun_driver_test/sdk_overlay/custom/solenoid_test.h`
- 振动马达：
  - `projects/light_gun_driver_test/sdk_overlay/custom/rumble_test.c`
  - `projects/light_gun_driver_test/sdk_overlay/custom/rumble_test.h`
- IR：
  - `projects/light_gun_driver_test/sdk_overlay/custom/ir_test.c`
  - `projects/light_gun_driver_test/sdk_overlay/custom/ir_test.h`
- USB HID、按键 GPIO、IR 接 USB 鼠标联动参考：
  - `projects/light_gun_driver_test/sdk_overlay/custom/usb_sim_test.c`
  - `projects/light_gun_driver_test/sdk_overlay/custom/usb_sim_test.h`

### 1.2 `light_gun_260517` 当前对应文件

- 反馈输出总入口：
  - `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_feedback.c`
- IR：
  - `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_ir_cam.c`
  - `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_position.c`
- 按键：
  - `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_input_keys.c`
- LED：
  - `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_led.c`
- USB：
  - `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_usb_cdc.c`
  - `projects/light_gun_260517/sdk_overlay/custom/src/transport/transport_core.c`
- 启动与接线点：
  - `projects/light_gun_260517/sdk_overlay/custom/src/app/of_entry.c`
  - `projects/light_gun_260517/sdk_overlay/custom/src/app/of_runtime.c`
  - `projects/light_gun_260517/sdk_overlay/custom/src/protocols/proto_mh.c`
  - `projects/light_gun_260517/sdk_overlay/custom/src/protocols/proto_docked.c`

### 1.3 当前最大的结构性差异

- `driver_test` 是“测试状态机风格”
  - 软件定时器、脚本流、日志、硬件保护都写在一个文件里
- `260517` 是“设备驱动 + 协议 + 运行时”分层
  - 驱动负责真实硬件
  - `protocols` 负责接上位机协议
  - `services/runtime` 负责采样和状态流转

所以迁移时的原则应该是：

- 把“正确的底层寄存器/IO/PWM/I2C逻辑”迁进 `drivers`
- 把“测试专用脚本和假数据逻辑”留在 `driver_test`
- 把“IR -> 鼠标、按键 -> HID、协议 -> FFB”这种组合行为放到 `services` 或 `runtime`

---

## 2. 电磁铁整改方案

## 2.1 当前差异

### `driver_test` 已验证内容

来源文件：

- `projects/light_gun_driver_test/sdk_overlay/custom/solenoid_test.c`

已验证的关键点：

- GPIO 默认配置为输出低电平
- 支持上拉
- 有明确的软件定时器状态机
- 有安全钳位，避免线圈长时间通电烧毁
- 有单发、周期单发、有限连发、三连发等脉冲模型

### `260517` 当前状态

当前没有独立的电磁铁驱动文件。

最接近的是：

- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_feedback.c`

但这个文件本质上只是：

- 一路 PWM 占空比输出接口
- 没有电磁铁脉冲状态机
- 没有“拉高多久再拉低”的时序控制
- 没有“最大通电时长”保护
- 没有“电磁铁”和“振动马达”分离

更关键的是：

- `projects/light_gun_260517/sdk_overlay/custom/src/protocols/proto_mh.c`
  里 `F0` 和 `F1` 最终都走到同一个 `mh_apply_feedback_level()`
- 这意味着当前工程里“电磁铁命令”和“振动命令”其实没有真正分开

## 2.2 建议改法

### 方案建议

不要继续把电磁铁塞在 `drv_feedback.c` 里混着做，建议单独建驱动。

建议新增：

- `projects/light_gun_260517/sdk_overlay/custom/include/drivers/drv_solenoid.h`
- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_solenoid.c`

### 建议迁移的内容

从 `solenoid_test.c` 提炼以下能力进正式驱动：

- GPIO 初始化
- 默认低电平关断
- 软件定时器控制单次脉冲
- 最大通电时长钳位
- 强制关断接口
- 连发/三连发所需的基础“on/off 节拍控制”

### 建议暴露的正式接口

建议不要保留“测试 case”概念，而是改成业务接口，例如：

- `drv_solenoid_fire_once(on_ms)`
- `drv_solenoid_fire_burst(shots, on_ms, off_ms)`
- `drv_solenoid_stop()`
- `drv_solenoid_is_ready()`

### 需要改哪些文件

- 新增：
  - `include/drivers/drv_solenoid.h`
  - `src/drivers/drv_solenoid.c`
- 修改：
  - `src/app/of_entry.c`
    - 启动时打开电磁铁驱动
  - `src/protocols/proto_mh.c`
    - `F0` 不再调用统一 `mh_apply_feedback_level()`
    - 改成调用电磁铁驱动
  - `src/protocols/proto_docked.c`
    - 如果 `0x32` 需要映射到电磁铁，也要分流
  - `src/sm/sm_selftest.c`
    - 自检项从“feedback ready”改成更明确的“solenoid ready / rumble ready”

### 为什么这样改

- 电磁铁本质是“脉冲器件”，不是“持续占空比器件”
- 用一个简单 PWM 占空比接口去抽象电磁铁，后续很容易误触发长通电
- 单独建驱动后，安全保护和协议分流都更清晰

---

## 3. 振动马达整改方案

## 3.1 当前差异

### `driver_test` 已验证内容

来源文件：

- `projects/light_gun_driver_test/sdk_overlay/custom/rumble_test.c`

已验证的关键点：

- PWM 初始化和输出路径可用
- 空闲时切回 GPIO 低电平
- 做了 PWM 残留 pinmux 清理
- 处理过“换 IO 后两个脚同时出 PWM”的问题
- 有占空比和持续时间的安全边界

### `260517` 当前状态

当前振动能力也塞在：

- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_feedback.c`

它的问题是：

- 只有最薄的一层 `uapi_pwm_update_cfg()`
- 没有“强制切回 GPIO 低电平”
- 没有“残留 PWM pinmux 清理”
- 没有“持续时间保护”
- 没有把你已经验证通过的 PWM group/channel/pin/pinmux 经验沉淀进去

## 3.2 建议改法

### 方案建议

振动马达可以保留在 `drv_feedback.c`，但更推荐也单独拆驱动。

建议新增：

- `projects/light_gun_260517/sdk_overlay/custom/include/drivers/drv_rumble.h`
- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_rumble.c`

如果你希望尽量少改协议层，也可以把 `drv_feedback.c` 重构成“内部只代理 rumble”的兼容壳，但不建议继续把 solenoid 和 rumble 混成一个设备。

### 建议迁移的内容

从 `rumble_test.c` 提炼进正式驱动：

- GPIO 低电平空闲态
- stale PWM pinmux 清理
- PWM 初始化、open、start
- 占空比设置
- 停止时切回 GPIO 低电平
- 有界的振动持续时间

### 需要改哪些文件

- 新增：
  - `include/drivers/drv_rumble.h`
  - `src/drivers/drv_rumble.c`
- 修改：
  - `src/app/of_entry.c`
    - 启动时打开振动驱动
  - `src/protocols/proto_mh.c`
    - `F1` 改成调 rumble 驱动
  - `src/protocols/proto_docked.c`
    - 如果保留 `0x32` 为统一反馈口，需要重新定义它到底控制 rumble 还是带类型参数
  - `src/sm/sm_selftest.c`
    - 自检项同步更新

### 为什么这样改

- 振动马达适合 PWM 强度控制
- 电磁铁不适合用同一套接口抽象
- 两者分开后，协议、保护和后续调参都会更简单

---

## 4. IR 传感器整改方案

## 4.1 当前差异

### `driver_test` 已验证内容

来源文件：

- `projects/light_gun_driver_test/sdk_overlay/custom/ir_test.c`
- `projects/light_gun_driver_test/sdk_overlay/custom/ir_test.h`

已验证的关键点：

- I2C pinmux 和初始化可用
- 传感器寄存器初始化可用
- 默认 I2C 速率已经降到 `200k`
- 有 `basicAtomic` 双帧稳定读取
- 能解包 4 个 IR 点
- 有简化坐标解算
- 有单点降级
- 有平滑和日志节流

### `260517` 当前状态

当前 IR 驱动文件是：

- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_ir_cam.c`

当前问题非常明显：

- 只做了 `uapi_i2c_master_init(..., 400000U, ...)`
- 没有 Wii IR Camera 兼容初始化寄存器序列
- 读的是寄存器 `0x00` 的 5 字节，默认认为已经是 `x/y/valid`
- 没有 `basicAtomic`
- 没有 4 点解包
- 没有坐标解算

也就是说，`260517` 当前这个 `drv_ir_cam.c` 不是“把真实 IR 模块跑通的驱动”，更像是一个占位 stub。

同时：

- `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_position.c`
  当前假设 `drv_ir_cam.read()` 直接返回 `x/y/valid`

## 4.2 建议改法

### 方案建议

优先保持 `svc_position.c` 的接口不大动，把真正的 IR 初始化和解算塞进 `drv_ir_cam.c` 内部。

也就是：

- `drv_ir_cam.c` 继续对上层输出 `x/y/valid`
- 但内部实现换成 `driver_test` 里已经验证过的真实流程

这样改动范围最小。

### 建议迁移的内容

从 `ir_test.c` 提炼进 `drv_ir_cam.c`：

- I2C pinmux
- I2C 200k 初始化
- 传感器寄存器初始化
- `0x36` basic frame 双帧读
- 4 点解包
- seen mask 统计
- 左右点中心解算
- 单点降级
- 平滑

### `svc_position.c` 的处理建议

当前 `svc_position.c` 已经做了一次运行模式平均。

这里要注意：

- 如果 `drv_ir_cam.c` 已经做了平滑
- `svc_position.c` 再做一层 average
- 鼠标或准星就会更“肉”

建议：

- 先保留 `svc_position.c`
- 但默认运行模式优先用 `OF_POS_RUN_NORMAL`
- 如果发现延迟明显，再决定是关掉 IR 驱动里的平滑，还是降低 `svc_position` 的平均级别

### 需要改哪些文件

- 重点修改：
  - `src/drivers/drv_ir_cam.c`
- 可选新增：
  - `include/drivers/drv_ir_cam.h`
    - 如果你后续想暴露原始点、seen mask、degraded 状态，可以扩展接口
- 同步调整：
  - `src/services/svc_position.c`
    - 根据新输出决定是否保留双重平滑
  - `src/sm/sm_selftest.c`
    - `ready` 判断要基于真实初始化成功，而不是只看 I2C init

### 为什么这样改

- IR 是整条枪最关键的输入链路之一
- 现在 `260517` 里的 IR 驱动还没有把真实硬件流程接进去
- 你已经在 `driver_test` 里把这块踩坑踩完了，应该直接复用那套正确路径

---

## 5. 按键整改方案

## 5.1 当前差异

### `driver_test` 已验证内容

来源参考：

- `projects/light_gun_driver_test/sdk_overlay/custom/usb_sim_test.h`
- `projects/light_gun_driver_test/sdk_overlay/custom/usb_sim_test.c`

当前测试里已经沉淀了：

- 多个按键 GPIO 定义
- 输入上拉
- 低电平按下
- 去抖
- 角色映射

### `260517` 当前状态

当前按键驱动文件是：

- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_input_keys.c`

现在的问题是：

- 只支持 1 个 GPIO
- 读取结果只有 1 字节布尔值
- 没有多键 bitmask
- 没有去抖
- 没有 Trigger / A / B / START / SELECT / 方向键分离

更大的问题在运行时：

- `projects/light_gun_260517/sdk_overlay/custom/src/app/of_runtime.c`
  现在只把 `keys[0] != 0` 当成“有一个键按下”
- `svc_binding_get(0, &key_map0)` 也只映射了一个逻辑位

也就是说，即使你把 `drv_input_keys.c` 改成多键，运行时和绑定服务现在也接不住。

## 5.2 建议改法

### 方案建议

把 `drv_input_keys.c` 改成“多键扫描 + bitmask 输出”，并同步改运行时消费逻辑。

### 建议迁移的内容

从 `usb_sim_test.c` 抽取：

- 按键 GPIO 表
- 默认 pull-up
- 低电平按下规则
- 去抖字段和状态机

### 建议输出格式

建议 `drv_input_keys.read()` 改成至少 2 或 4 字节 bitmask，而不是 1 字节布尔值。

例如：

- bit0 = Trigger
- bit1 = A
- bit2 = B
- bit3 = START
- bit4 = SELECT
- bit5 = HOME
- bit6 = UP
- bit7 = DOWN
- bit8 = LEFT
- bit9 = RIGHT
- bit10 = MIDDLE

### 需要改哪些文件

- 重点修改：
  - `src/drivers/drv_input_keys.c`
- 同步修改：
  - `include/drivers/drv_input_keys.h`
    - 如果需要增加按键位定义
  - `src/app/of_runtime.c`
    - 从“单键布尔”改成“多键 bitmask”
  - `src/services/svc_binding.c`
    - 如果你要做真正的键位映射，这里也要跟上多键结构
  - `src/protocols/proto_docked.c`
    - 如果要上报真实按键映射配置，也要同步格式

### 为什么这样改

- `driver_test` 的多按键链路已经把硬件输入方式验证清楚了
- `260517` 当前的单键模型太弱，后面接 USB HID、游戏键位、扳机业务都会受限

---

## 6. LED 整改方案

## 6.1 当前差异

### `driver_test` 实际情况

`driver_test` 里没有真实 LED 硬件驱动，主要是：

- `usb_sim_test.c` 里打印 `LEDx ON/OFF` 日志来模拟业务事件

所以这里要特别说明：

- 你验证通过的是“LED 业务时序和上层事件流”
- 不是“真实 LED GPIO/PWM 驱动代码已经在 `driver_test` 里 ready 了”

### `260517` 当前状态

当前 LED 驱动文件：

- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_led.c`

现在它只是：

- 单 GPIO
- 单 bit 开关

但 `proto_mh.c` 里实际上已经存在：

- `rgb_r`
- `rgb_g`
- `rgb_b`

可当前实现却把三路颜色压成：

- 只要任意一路非 0，就点亮一个总 LED

这说明协议侧和驱动侧不匹配。

## 6.2 建议改法

### 如果你的硬件只有 1 个状态灯

那就保留 `drv_led.c` 单路模型，但要做两件事：

- 把正确的 LED GPIO、默认电平、有效极性写进 Kconfig
- 在文档里明确：`MH` 协议里 `R/G/B` 目前只是聚合成“灯亮/灭”

### 如果你的硬件有多路 LED 或 RGB

建议升级：

- `drv_led.c` 支持多路 GPIO 或 RGB 三通道
- `proto_mh.c` 里的 `rgb_r/g/b` 不再压成一个开关

### 需要改哪些文件

- 修改：
  - `src/drivers/drv_led.c`
  - `include/drivers/drv_led.h`
  - `src/protocols/proto_mh.c`
- 如果要多路 LED：
  - `src/protocols/proto_docked.c`
    - 相关 LED 控制命令也要对齐能力

### 为什么这样改

- 当前 `260517` 的 LED 是“最小占位版”
- 如果你后面要对齐 OpenFIRE 风格的灯效或协议字段，必须先把驱动能力补齐

---

## 7. USB 整改方案

## 7.1 当前差异

### `driver_test` 已验证内容

来源文件：

- `projects/light_gun_driver_test/sdk_overlay/custom/usb_sim_test.c`
- `projects/light_gun_driver_test/sdk_overlay/custom/usb_sim_test.h`

已经验证过的能力：

- 枚举成 USB HID 键盘 + 鼠标
- 主机枚举成功后再开始业务
- 枚举断开后暂停业务
- 物理按键可以上报 HID
- IR 解算结果可以接到鼠标移动

### `260517` 当前状态

当前 USB 文件：

- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_usb_cdc.c`
- `projects/light_gun_260517/sdk_overlay/custom/src/transport/transport_core.c`

这套代码本质是：

- CDC/串口 transport
- 给协议层收发数据

它不是：

- HID 鼠标/键盘驱动
- 也没有 HID report descriptor
- 也没有鼠标/键盘报文发送接口

换句话说：

- `260517` 现在的 USB“能通信”
- 但不是你在 `driver_test` 里验证过的那种“光枪 HID 输入设备”

## 7.2 建议改法

### 方案建议

不要把 `drv_usb_cdc.c` 直接替换掉。

推荐做法是：

- 保留 `CDC` 作为配置/调试/协议通道
- 另外新增一套 `USB HID` 驱动

这样后续最稳。

### 建议新增文件

- `projects/light_gun_260517/sdk_overlay/custom/include/drivers/drv_usb_hid.h`
- `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_usb_hid.c`

### 从 `usb_sim_test.c` 提炼什么

只迁移“正式可复用”的部分：

- HID report descriptor
- USB 枚举 ready 检测
- 鼠标报文发送
- 键盘报文发送
- 主机断开暂停

不要迁移测试专用部分：

- fake IR script
- fake LED script
- scripted mouse move
- scripted mouse click
- scripted keyboard

这些都属于测试工程资产，不该进正式产品路径。

### 运行时怎么接

建议新增一个服务层，例如：

- `src/services/svc_usb_hid.c`
- `include/services/svc_usb_hid.h`

职责：

- 从 `svc_position_get()` 取 IR 坐标
- 从 `drv_input_keys` 取真实按键 bitmask
- 生成鼠标移动、鼠标按键、键盘按键 HID 报告
- 处理屏内/屏外扳机逻辑

### 需要改哪些文件

- 新增：
  - `include/drivers/drv_usb_hid.h`
  - `src/drivers/drv_usb_hid.c`
  - `include/services/svc_usb_hid.h`
  - `src/services/svc_usb_hid.c`
- 修改：
  - `src/app/of_entry.c`
    - 初始化 HID 驱动
  - `src/app/of_runtime.c`
    - 周期调用 `svc_usb_hid_tick()`
  - `src/drivers/drv_input_keys.c`
    - 输出多键 bitmask
  - `src/services/svc_position.c`
    - 提供给 HID 鼠标位置
  - `src/sm/sm_selftest.c`
    - 如果 HID 是关键链路，可加状态检查

### 为什么这样改

- `driver_test` 里验证的是 HID 设备模型
- `260517` 当前只有 CDC 传输模型
- 两者不是替换关系，而是并行关系

---

## 8. 配置与工程文件整改方案

## 8.1 Kconfig

当前：

- `projects/light_gun_driver_test/sdk_overlay/custom/Kconfig`
  里已经有一大批真实可用的硬件配置项
- `projects/light_gun_260517/sdk_overlay/custom/Kconfig`
  现在几乎没有硬件配置项，只有一个 overlay 开关

建议把下面这些配置迁到 `260517` 的 Kconfig：

- 电磁铁 GPIO、默认极性、最大通电时间
- 振动 PWM channel/group/pin/pin_mode、最大持续时间、默认强度
- IR I2C bus、SCL/SDA pin、pin mode、addr、baudrate、poll ms
- 按键 GPIO 列表、去抖时间
- LED GPIO 列表和有效极性
- USB HID 开关

为什么要加 Kconfig：

- `260517` 是正式工程，不能再依赖测试工程里的硬编码宏
- 后续板级变种、不同批次 pin 改动时，Kconfig 会比改源码安全很多

## 8.2 默认配置

建议同步修改：

- `projects/light_gun_260517/config/standard_tr5310_s.config`

原则：

- 以 `projects/light_gun_driver_test/config/standard_tr5310_s.config`
  中当前已经跑通的参数为基础
- 再按 `260517` 的正式命名方式落地

## 8.3 CMakeLists

建议同步修改：

- `projects/light_gun_260517/sdk_overlay/custom/CMakeLists.txt`

需要增加的新源文件通常包括：

- `src/drivers/drv_solenoid.c`
- `src/drivers/drv_rumble.c`
- `src/drivers/drv_usb_hid.c`
- `src/services/svc_usb_hid.c`

如果你决定不拆分成这么细，也至少要把你实际新增的驱动文件补进来。

---

## 9. 推荐迁移顺序

建议按下面顺序推进，风险最低。

### 第一步：先补底层真实驱动

- `drv_ir_cam.c`
- `drv_input_keys.c`
- `drv_rumble.c`
- `drv_solenoid.c`
- `drv_led.c`

先保证：

- 每个驱动都能独立初始化
- 每个驱动都能用串口日志和示波器验证

### 第二步：再补运行时整合

- `svc_position.c`
- `of_runtime.c`
- `svc_usb_hid.c`

先保证：

- IR 坐标能被正式运行时取到
- 按键 bitmask 能被正式运行时取到
- HID 能发送真实鼠标/键盘报文

### 第三步：最后补协议分流

- `proto_mh.c`
- `proto_docked.c`

重点把：

- `F0 -> solenoid`
- `F1 -> rumble`
- `RGB -> led`

这些路由从“占位实现”改成“真实设备实现”。

---

## 10. 最终建议

最重要的不是“把测试文件搬过去”，而是按下面这个映射迁移：

- `solenoid_test.c` 的硬件控制思想
  -> `drv_solenoid.c`
- `rumble_test.c` 的 PWM 和安全收口
  -> `drv_rumble.c`
- `ir_test.c` 的真实初始化和解算
  -> `drv_ir_cam.c`
- `usb_sim_test.c` 的按键 GPIO 表和 HID 发送能力
  -> `drv_input_keys.c` + `drv_usb_hid.c` + `svc_usb_hid.c`
- `usb_sim_test.c` 的 fake 脚本
  -> 不迁移，继续只留在测试工程

一句话概括：

- `driver_test` 负责证明“这块硬件和时序是对的”
- `260517` 负责把“对的硬件能力”接成正式产品链路

如果后续开始实际动代码，建议优先从这 4 个文件开始改：

- `src/drivers/drv_ir_cam.c`
- `src/drivers/drv_input_keys.c`
- `src/protocols/proto_mh.c`
- `src/app/of_runtime.c`

原因是：

- 这 4 个点决定了输入、输出、协议和运行时主链路能不能真正闭环
