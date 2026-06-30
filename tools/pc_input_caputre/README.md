# PC Input Capture

基于 `Python 3.13.9 + PyQt5 + evdev` 的 PC 端输入检测工具，用来观察 OpenFIRE / TR531x 在 PC 上实际注入的键盘、鼠标和手柄事件。

## 功能

- 键盘按下变绿，松开恢复白色
- 鼠标左右中键/扩展键按下变绿，松开恢复白色
- 鼠标移动、滚轮、点击在调试窗口打印
- 调试窗口只保留最近 100 行
- 手柄按钮、轴、Hat 等“其他上报”也用 `QLabel` 状态块显示
- 自动扫描 `/dev/input/event*`，支持设备热插拔

## 运行

```bash
cd /data/tr531x_proj
python3 tools/pc_input_caputre/pc_input_capture.py
```

如果只想抓特定设备：

```bash
python3 tools/pc_input_caputre/pc_input_capture.py --device event5 --device event6
```

## 依赖

- `PyQt5`
- `evdev`

当前实现直接读取 Linux `evdev` 事件，所以需要当前用户对 `/dev/input/event*` 有读取权限。
