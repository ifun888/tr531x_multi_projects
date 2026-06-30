# LightGun260517 IR升级为OpenFIRE风格整改方案

本文档目标：

- 回答 `projects/light_gun_260517` 当前 IR 能力和 `ref_projs/OpenFIRE-Firmware-ESP32` 的差异
- 给出一套适合当前工程结构的升级方案
- 重点覆盖：
  - 上下左右四边校准
  - 中心点校准
  - 坐标映射升级
  - 简化版 spring/offset 抗跳方案
  - 推荐落地顺序

---

## 1. 先说结论

当前 `projects/light_gun_260517` 的 IR 链路已经具备：

- I2C 初始化和基础读数
- IR 点位解算
- 坐标有效位 `valid`
- 简单平滑
- 中心偏移校准
- IR 坐标接 USB HID 鼠标移动

但是它还没有做到 OpenFIRE 那套更完整的能力：

- 没有 `Top / Bottom / Left / Right` 四边校准
- 没有独立的四边 offset 参数
- 没有按四边参数重映射屏幕坐标
- 没有完整的 perspective 边缘修正链路
- 没有真正的 spring/offset 弹性抗跳处理

通俗理解：

- 当前版像“先出点，再减中心偏移，再做一点平滑”
- OpenFIRE 像“先恢复屏幕几何关系，再做四边拉正，最后再做边缘和跳点处理”

---

## 2. 当前 260517 的 IR 业务现状

当前主链路分三层：

### 2.1 驱动层

- 文件：
  - `projects/light_gun_260517/sdk_overlay/custom/src/drivers/drv_ir_cam.c`

职责：

- 初始化 I2C 和 IR 传感器
- 读取原始点
- 做基础解算
- 输出 `x / y / valid`

### 2.2 校准层

- 文件：
  - `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_calibration.c`

职责：

- 连续收样本
- 求平均值
- 只得到一个中心偏移：
  - `cal_x`
  - `cal_y`

这个实现本质上不是“屏幕校准”，而更像“零点校准”。

### 2.3 位置服务层

- 文件：
  - `projects/light_gun_260517/sdk_overlay/custom/src/services/svc_position.c`

职责：

- 从 `drv_ir_cam` 取结果
- 读取 `cal_x / cal_y`
- 做简单偏移修正
- 做运行模式平滑
- 输出最终位置给 USB HID 或协议层

当前核心逻辑可以概括成：

```c
adj_x = raw_x - cal_x;
adj_y = raw_y - cal_y;
```

然后再做：

- 不平滑
- 2 点平均
- 4 点平均

所以它现在并不知道：

- 屏幕上边界应该在哪里
- 屏幕下边界应该在哪里
- 左边打偏多少
- 右边打偏多少

---

## 3. OpenFIRE 那边多了什么

OpenFIRE 的 profile 里保存的是一整套屏幕映射参数，不只是中心点。

参考：

- `ref_projs/OpenFIRE-Firmware-ESP32/lightgun/src/OpenFIREprefs.h`
- `ref_projs/OpenFIRE-Firmware-ESP32/lightgun/src/OpenFIREcommon.cpp`

关键参数有：

- `topOffset`
- `bottomOffset`
- `leftOffset`
- `rightOffset`
- `TLled`
- `TRled`
- `adjX`
- `adjY`
- `irSens`
- `runMode`
- `irLayout`
- `aspectRatio`

其中和你现在最相关的是前四个：

- `topOffset`
- `bottomOffset`
- `leftOffset`
- `rightOffset`

它们的意义很通俗：

- 枪瞄到屏幕上边时，如果鼠标还没到最顶，就需要记录一个 `topOffset`
- 枪瞄到下边时，如果鼠标还没到最底，就需要记录一个 `bottomOffset`
- 左右同理

后续映射时，不再是简单 `raw - center`，而是：

```c
mouseX = map(x, 0, screen_w, -leftOffset, screen_w + rightOffset);
mouseY = map(y, 0, screen_h, -topOffset,  screen_h + bottomOffset);
```

这就是“上下左右分开修正”的本质。

---

## 4. 你要补哪些能力

建议拆成 4 个整改点。

### 4.1 改 profile 数据结构

当前 `svc_profile` 只存：

- `run_mode`
- `cal_x`
- `cal_y`

要升级成至少能存：

- `center_x`
- `center_y`
- `top_offset`
- `bottom_offset`
- `left_offset`
- `right_offset`
- `ir_run_mode`
- `ir_sensitivity`
- `ir_layout`

如果你想先做最小闭环，第一阶段至少加：

- `center_x`
- `center_y`
- `top_offset`
- `bottom_offset`
- `left_offset`
- `right_offset`

### 4.2 改校准状态机

当前 `svc_calibration` 只有：

- `IDLE`
- `RUNNING`
- `DONE`

这不够用。

建议改成多阶段状态机：

- `OF_CAL_IDLE`
- `OF_CAL_TOP`
- `OF_CAL_BOTTOM`
- `OF_CAL_LEFT`
- `OF_CAL_RIGHT`
- `OF_CAL_CENTER`
- `OF_CAL_VERIFY`
- `OF_CAL_DONE`

这样程序就知道“现在该采哪一个边”。

### 4.3 改位置映射逻辑

当前 `svc_position.c` 只做“减中心偏移”。

需要改成两段式：

1. 先做中心修正
2. 再做四边映射

通俗公式可以先用：

```c
adj_x = raw_x - center_x;
adj_y = raw_y - center_y;
```

然后再把 `adj_x/adj_y` 映射成屏幕坐标：

```c
screen_x = linear_map(adj_x,
                      raw_min_x,
                      raw_max_x,
                      0 - left_offset,
                      screen_w + right_offset);

screen_y = linear_map(adj_y,
                      raw_min_y,
                      raw_max_y,
                      0 - top_offset,
                      screen_h + bottom_offset);
```

第一版不一定要完全照搬 OpenFIRE 的 perspective 库，但必须先把“4 边独立补偿”做进去。

### 4.4 增加 spring 简化版抗跳

这里先说明一下，“spring” 你可以先按程序员视角理解成：

- 不是一有新点就 100% 相信它
- 如果新点突然跳得很远，不立刻跟过去
- 而是像弹簧一样“拉一部分”

适合你当前工程的简化版可以这样做：

#### 方案 A：限幅追踪

每次位置更新时：

- 如果本次点和上次点差值很小，直接更新
- 如果差值很大，只允许本次最多移动固定步长

例如：

```c
dx = new_x - last_x;
dy = new_y - last_y;

if (abs(dx) > limit) {
    new_x = last_x + sign(dx) * limit;
}
if (abs(dy) > limit) {
    new_y = last_y + sign(dy) * limit;
}
```

优点：

- 最好实现
- 对突发跳点很有效

缺点：

- 快速甩枪时会感觉略拖

#### 方案 B：弹性跟随

如果跳点很大，不直接跳过去，而是：

```c
filtered_x = last_x + (new_x - last_x) * alpha;
filtered_y = last_y + (new_y - last_y) * alpha;
```

其中：

- `alpha` 小，手感更稳
- `alpha` 大，跟手性更强

优点：

- 手感更自然

缺点：

- 要调参

#### 当前建议

先做：

- 小范围：直接更新
- 大范围：限幅
- 连续稳定若干帧后：恢复全速

这对你当前硬件 bring-up 最稳。

---

## 5. 推荐的代码改造点

建议按当前 `260517` 分层结构改，不要做成一个大文件。

### 5.1 `svc_profile.h / svc_profile.c`

要改什么：

- 增加 IR profile 字段定义
- 增加获取/设置四边参数的接口

建议新增接口：

- `svc_profile_get_ir_center(...)`
- `svc_profile_set_ir_center(...)`
- `svc_profile_get_ir_offsets(...)`
- `svc_profile_set_ir_offsets(...)`

为什么：

- 让 `svc_position` 和 `svc_calibration` 都通过统一入口读写 profile
- 后面如果要从串口协议或上位机写入参数，也有统一接口

### 5.2 `svc_calibration.h / svc_calibration.c`

要改什么：

- 把单一平均校准改成多阶段校准状态机
- 支持分别提交 top / bottom / left / right / center
- 支持校准结果暂存和 commit

建议新增接口：

- `svc_calibration_start()`
- `svc_calibration_set_stage(stage)`
- `svc_calibration_push_sample(x, y)`
- `svc_calibration_get_offsets(...)`
- `svc_calibration_commit()`

为什么：

- 这样后面无论是按键触发校准，还是上位机引导校准，都能共用

### 5.3 `svc_position.c`

要改什么：

- 把当前“减中心偏移”改成：
  - 中心修正
  - 四边映射
  - spring/抗跳滤波

建议内部拆成几个小函数：

- `pos_apply_center()`
- `pos_apply_edge_offsets()`
- `pos_apply_spring_filter()`
- `pos_apply_run_mode_filter()`

为什么：

- 后面调试时，你可以单独开日志看每一层结果
- 不会把所有问题混在一起

### 5.4 `proto_docked.c`

要改什么：

- 如果后面要和上位机联动校准，需要扩展当前校准命令
- 现在它只适合非常简单的中心校准

建议：

- 保留旧命令兼容
- 新增分阶段校准命令

为什么：

- 这样 PC 端可以明确告诉枪：
  - 现在采 top
  - 现在采 bottom
  - 现在采 left
  - 现在采 right
  - 现在 commit

### 5.5 `docs` 和调试日志

要改什么：

- 补 IR 校准阶段日志
- 补四边参数打印
- 补滤波前后坐标打印开关

建议日志内容：

- 当前校准阶段
- 当前原始点
- 中心修正后点
- 四边映射后点
- spring 后点

为什么：

- 不然你后面调试会看见“鼠标就是不对”，但不知道错在：
  - 驱动读数
  - 中心偏移
  - 四边映射
  - spring 参数

---

## 6. 推荐落地顺序

不要一口气全抄 OpenFIRE，建议分 4 步落地。

### 第 1 步：先补 profile 四边参数

目标：

- 能保存和读取：
  - `center_x`
  - `center_y`
  - `top_offset`
  - `bottom_offset`
  - `left_offset`
  - `right_offset`

验收标准：

- 上电后能正确 load/save
- 串口能打印当前参数

### 第 2 步：再补四边校准状态机

目标：

- 不改 IR 驱动
- 只改 `svc_calibration`
- 能按阶段分别收 top/bottom/left/right/center

验收标准：

- 每个阶段都能采到值
- commit 后参数进入 profile

### 第 3 步：升级 `svc_position`

目标：

- 把坐标处理从“中心偏移”升级成“中心 + 四边映射”

验收标准：

- 屏幕边缘明显比现在更贴边
- 左右和上下不再只能整体平移

### 第 4 步：再加 spring 简化滤波

目标：

- 处理突发跳点
- 不影响基础可用性

验收标准：

- 静止瞄准更稳
- 偶发坏点不再让鼠标瞬移
- 快速移动还能接受

---

## 7. 给你的最实用建议

如果你现在的目标是“尽快把 260517 做到可玩”，不要一上来追 OpenFIRE 全量几何模型。

最合适的顺序是：

1. 先做四边参数存储
2. 再做四边校准
3. 再做四边映射
4. 最后补 spring

因为真正影响体验最大的，不是“有没有最复杂的透视模型”，而是：

- 边缘能不能打准
- 坐标会不会乱跳
- 校准流程能不能稳定复现

也就是说，第一阶段你只要做到：

- 中心准
- 四边准
- 跳点别太离谱

这版就已经比当前 `260517` 强很多了。

---

## 8. 一句话总结

当前 `projects/light_gun_260517` 的 IR 还是“中心偏移版”，还不是 OpenFIRE 那种“上下左右分边校准 + spring/offset 抗跳版”。

最合理的整改路线不是硬抄全部算法，而是按下面顺序逐步升级：

- `profile` 扩字段
- `calibration` 改多阶段
- `position` 改四边映射
- `spring` 做简化抗跳

这样改，风险最低，调试也最清楚。
