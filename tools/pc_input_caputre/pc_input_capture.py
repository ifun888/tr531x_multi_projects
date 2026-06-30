#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import selectors
import sys
import time
from collections import deque
from dataclasses import dataclass

from PyQt5 import QtCore, QtGui, QtWidgets

try:
    from evdev import InputDevice, categorize, ecodes, list_devices
except ImportError as exc:  # pragma: no cover
    raise SystemExit(f"missing dependency evdev: {exc}")


LOG_LIMIT = 100
GRID_COLUMNS = 4
DISCOVERY_INTERVAL_SEC = 2.0
TRANSIENT_HIGHLIGHT_MS = 250
AXIS_ACTIVE_DEADZONE = 0


def build_code_name_maps() -> tuple[dict[int, str], dict[int, str], dict[int, str], dict[int, str]]:
    key_names: dict[int, str] = {}
    abs_names: dict[int, str] = {}
    rel_names: dict[int, str] = {}
    for name, value in ecodes.ecodes.items():
        if not isinstance(value, int):
            continue
        if name.startswith("KEY_") or name.startswith("BTN_"):
            key_names.setdefault(value, name)
        elif name.startswith("ABS_"):
            abs_names.setdefault(value, name)
        elif name.startswith("REL_"):
            rel_names.setdefault(value, name)

    category_names = {
        ecodes.EV_KEY: "EV_KEY",
        ecodes.EV_REL: "EV_REL",
        ecodes.EV_ABS: "EV_ABS",
        ecodes.EV_SYN: "EV_SYN",
        ecodes.EV_MSC: "EV_MSC",
    }
    return key_names, abs_names, rel_names, category_names


KEY_NAMES, ABS_NAMES, REL_NAMES, EVENT_TYPE_NAMES = build_code_name_maps()


def normalize_name(raw_name: str) -> str:
    return raw_name.replace("_", " ")


def is_mouse_button(code: int) -> bool:
    return code in {
        ecodes.BTN_LEFT,
        ecodes.BTN_RIGHT,
        ecodes.BTN_MIDDLE,
        ecodes.BTN_SIDE,
        ecodes.BTN_EXTRA,
        ecodes.BTN_FORWARD,
        ecodes.BTN_BACK,
        ecodes.BTN_TASK,
    }


def is_gamepad_button(code: int) -> bool:
    return ecodes.BTN_JOYSTICK <= code <= ecodes.BTN_DIGI or code in {
        ecodes.BTN_A,
        ecodes.BTN_B,
        ecodes.BTN_C,
        ecodes.BTN_X,
        ecodes.BTN_Y,
        ecodes.BTN_Z,
        ecodes.BTN_TL,
        ecodes.BTN_TR,
        ecodes.BTN_TL2,
        ecodes.BTN_TR2,
        ecodes.BTN_SELECT,
        ecodes.BTN_START,
        ecodes.BTN_MODE,
        ecodes.BTN_THUMBL,
        ecodes.BTN_THUMBR,
        ecodes.BTN_DPAD_UP,
        ecodes.BTN_DPAD_DOWN,
        ecodes.BTN_DPAD_LEFT,
        ecodes.BTN_DPAD_RIGHT,
    }


def classify_device(capabilities: dict[int, object]) -> str:
    has_rel = ecodes.EV_REL in capabilities
    has_abs = ecodes.EV_ABS in capabilities
    key_codes = set(capabilities.get(ecodes.EV_KEY, []))
    has_mouse = has_rel and any(is_mouse_button(code) for code in key_codes)
    has_gamepad = has_abs or any(is_gamepad_button(code) for code in key_codes)

    if has_mouse and has_gamepad:
        return "hybrid"
    if has_mouse:
        return "mouse"
    if has_gamepad:
        return "gamepad"
    if key_codes:
        return "keyboard"
    return "other"


@dataclass
class DeviceContext:
    device: InputDevice
    category: str


class FlowGrid(QtWidgets.QWidget):
    def __init__(self, title: str, columns: int = GRID_COLUMNS, parent: QtWidgets.QWidget | None = None) -> None:
        super().__init__(parent)
        self.columns = columns
        self.labels: dict[str, QtWidgets.QLabel] = {}
        self.timers: dict[str, QtCore.QTimer] = {}
        self.layout_ = QtWidgets.QGridLayout()
        self.layout_.setContentsMargins(8, 8, 8, 8)
        self.layout_.setHorizontalSpacing(8)
        self.layout_.setVerticalSpacing(8)

        box = QtWidgets.QGroupBox(title)
        outer = QtWidgets.QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(box)
        box.setLayout(self.layout_)

    def ensure_label(self, key: str, text: str | None = None) -> QtWidgets.QLabel:
        if key in self.labels:
            label = self.labels[key]
            if text:
                label.setText(text)
            return label

        label = QtWidgets.QLabel(text or key)
        label.setAlignment(QtCore.Qt.AlignCenter)
        label.setMinimumWidth(140)
        label.setMinimumHeight(36)
        label.setFrameShape(QtWidgets.QFrame.Box)
        label.setStyleSheet(self.idle_style())
        index = len(self.labels)
        row = index // self.columns
        col = index % self.columns
        self.layout_.addWidget(label, row, col)
        self.labels[key] = label
        return label

    def set_state(
        self,
        key: str,
        active: bool,
        text: str | None = None,
        transient_ms: int | None = None,
    ) -> None:
        label = self.ensure_label(key, text)
        if text:
            label.setText(text)
        label.setStyleSheet(self.active_style() if active else self.idle_style())
        if transient_ms:
            timer = self.timers.get(key)
            if timer is None:
                timer = QtCore.QTimer(self)
                timer.setSingleShot(True)
                timer.timeout.connect(lambda item=key: self.set_state(item, False))
                self.timers[key] = timer
            timer.start(transient_ms)

    @staticmethod
    def idle_style() -> str:
        return (
            "QLabel {"
            "background: #ffffff;"
            "border: 1px solid #b9c0c8;"
            "border-radius: 6px;"
            "padding: 6px 10px;"
            "color: #1f2933;"
            "font-weight: 600;"
            "}"
        )

    @staticmethod
    def active_style() -> str:
        return (
            "QLabel {"
            "background: #8ee28e;"
            "border: 1px solid #38a169;"
            "border-radius: 6px;"
            "padding: 6px 10px;"
            "color: #103d10;"
            "font-weight: 700;"
            "}"
        )


class InputMonitor(QtCore.QThread):
    status_signal = QtCore.pyqtSignal(str)
    log_signal = QtCore.pyqtSignal(str)
    key_signal = QtCore.pyqtSignal(str, bool)
    mouse_button_signal = QtCore.pyqtSignal(str, bool)
    mouse_activity_signal = QtCore.pyqtSignal(str, str)
    other_signal = QtCore.pyqtSignal(str, bool, str)

    def __init__(self, only_devices: list[str] | None = None, parent: QtCore.QObject | None = None) -> None:
        super().__init__(parent)
        self.only_devices = set(only_devices or [])
        self._running = True
        self._selector = selectors.DefaultSelector()
        self._devices: dict[int, DeviceContext] = {}
        self._last_discovery = 0.0

    def stop(self) -> None:
        self._running = False

    def run(self) -> None:
        self.status_signal.emit("scanning /dev/input/event*")
        while self._running:
            now = time.monotonic()
            if now - self._last_discovery >= DISCOVERY_INTERVAL_SEC:
                self._discover_devices()
                self._last_discovery = now

            events = self._selector.select(timeout=0.2)
            for key, _ in events:
                if not self._running:
                    break
                self._read_device_events(key.fd)

        self._close_all_devices()
        self.status_signal.emit("stopped")

    def _discover_devices(self) -> None:
        active_paths = set(list_devices())
        if self.only_devices:
            active_paths = {path for path in active_paths if os.path.basename(path) in self.only_devices}

        known_paths = {ctx.device.path for ctx in self._devices.values()}
        for path in sorted(active_paths - known_paths):
            try:
                device = InputDevice(path)
                category = classify_device(device.capabilities(verbose=False))
                self._selector.register(device.fd, selectors.EVENT_READ)
                self._devices[device.fd] = DeviceContext(device=device, category=category)
                self.log_signal.emit(
                    f"device connected: {device.path} | {device.name or 'Unknown'} | category={category}"
                )
                self.other_signal.emit(
                    f"DEVICE {os.path.basename(device.path)}",
                    True,
                    f"{os.path.basename(device.path)}\n{category}",
                )
            except PermissionError as exc:
                self.log_signal.emit(f"device skipped: {path} | permission denied: {exc}")
            except OSError as exc:
                self.log_signal.emit(f"device skipped: {path} | {exc}")

        for fd, ctx in list(self._devices.items()):
            if ctx.device.path in active_paths:
                continue
            self._unregister_device(fd, disconnected=True)

        self.status_signal.emit(f"monitoring {len(self._devices)} device(s)")

    def _read_device_events(self, fd: int) -> None:
        ctx = self._devices.get(fd)
        if ctx is None:
            return

        try:
            for event in ctx.device.read():
                self._handle_event(ctx, event)
        except OSError as exc:
            self.log_signal.emit(f"device read error: {ctx.device.path} | {exc}")
            self._unregister_device(fd, disconnected=True)

    def _handle_event(self, ctx: DeviceContext, event) -> None:
        if event.type == ecodes.EV_SYN:
            return

        if event.type == ecodes.EV_KEY:
            self._handle_key_event(ctx, event)
            return

        if event.type == ecodes.EV_REL:
            self._handle_rel_event(ctx, event)
            return

        if event.type == ecodes.EV_ABS:
            self._handle_abs_event(ctx, event)
            return

        type_name = EVENT_TYPE_NAMES.get(event.type, f"EV_{event.type}")
        self.log_signal.emit(
            f"{ctx.category:<8} {ctx.device.name or ctx.device.path}: {type_name} code={event.code} value={event.value}"
        )

    def _handle_key_event(self, ctx: DeviceContext, event) -> None:
        key_event = categorize(event)
        name = KEY_NAMES.get(event.code, str(key_event.keycode))
        pressed = event.value != 0
        state = "DOWN" if event.value == 1 else "UP" if event.value == 0 else "HOLD"
        source = ctx.device.name or ctx.device.path

        if is_mouse_button(event.code):
            display = name.replace("BTN_", "")
            self.mouse_button_signal.emit(display, pressed)
            self.log_signal.emit(f"mouse    {source}: {display} {state}")
            return

        if ctx.category in {"gamepad", "hybrid"} or is_gamepad_button(event.code):
            display = name.replace("BTN_", "")
            self.other_signal.emit(f"PAD {display}", pressed, f"PAD {display}")
            self.log_signal.emit(f"gamepad  {source}: {display} {state}")
            return

        display = name.replace("KEY_", "")
        self.key_signal.emit(display, pressed)
        self.log_signal.emit(f"keyboard {source}: {display} {state}")

    def _handle_rel_event(self, ctx: DeviceContext, event) -> None:
        rel_name = REL_NAMES.get(event.code, f"REL_{event.code}")
        source = ctx.device.name or ctx.device.path

        if event.code == ecodes.REL_X:
            text = f"MOVE X {event.value:+d}"
            self.mouse_activity_signal.emit("MOVE_X", text)
            self.log_signal.emit(f"mouse    {source}: move x={event.value:+d}")
            return

        if event.code == ecodes.REL_Y:
            text = f"MOVE Y {event.value:+d}"
            self.mouse_activity_signal.emit("MOVE_Y", text)
            self.log_signal.emit(f"mouse    {source}: move y={event.value:+d}")
            return

        if event.code == ecodes.REL_WHEEL:
            key = "WHEEL_UP" if event.value > 0 else "WHEEL_DOWN"
            text = "WHEEL +" if event.value > 0 else "WHEEL -"
            self.mouse_activity_signal.emit(key, text)
            self.log_signal.emit(f"mouse    {source}: wheel={event.value:+d}")
            return

        if event.code == ecodes.REL_HWHEEL:
            key = "HWHEEL_RIGHT" if event.value > 0 else "HWHEEL_LEFT"
            text = "HWHEEL +" if event.value > 0 else "HWHEEL -"
            self.mouse_activity_signal.emit(key, text)
            self.log_signal.emit(f"mouse    {source}: hwheel={event.value:+d}")
            return

        self.log_signal.emit(f"mouse    {source}: {rel_name}={event.value:+d}")

    def _handle_abs_event(self, ctx: DeviceContext, event) -> None:
        name = ABS_NAMES.get(event.code, f"ABS_{event.code}")
        source = ctx.device.name or ctx.device.path

        if event.code in {ecodes.ABS_HAT0X, ecodes.ABS_HAT0Y}:
            if event.code == ecodes.ABS_HAT0X:
                self.other_signal.emit("HAT LEFT", event.value < 0, "HAT LEFT")
                self.other_signal.emit("HAT RIGHT", event.value > 0, "HAT RIGHT")
            else:
                self.other_signal.emit("HAT UP", event.value < 0, "HAT UP")
                self.other_signal.emit("HAT DOWN", event.value > 0, "HAT DOWN")
            self.log_signal.emit(f"gamepad  {source}: {name}={event.value:+d}")
            return

        active = abs(event.value) > AXIS_ACTIVE_DEADZONE
        text = f"{name}\n{event.value:+d}"
        self.other_signal.emit(name, active, text)
        self.log_signal.emit(f"gamepad  {source}: {name}={event.value:+d}")

    def _unregister_device(self, fd: int, disconnected: bool = False) -> None:
        ctx = self._devices.pop(fd, None)
        if ctx is None:
            return
        try:
            self._selector.unregister(fd)
        except Exception:
            pass
        try:
            path = ctx.device.path
            ctx.device.close()
        except Exception:
            path = "<unknown>"
        if disconnected:
            self.log_signal.emit(f"device disconnected: {path}")
            self.other_signal.emit(f"DEVICE {os.path.basename(path)}", False, f"{os.path.basename(path)}\noffline")

    def _close_all_devices(self) -> None:
        for fd in list(self._devices):
            self._unregister_device(fd)


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self, only_devices: list[str] | None = None) -> None:
        super().__init__()
        self.setWindowTitle("PC Input Capture")
        self.resize(1380, 900)

        self._log_lines: deque[str] = deque(maxlen=LOG_LIMIT)

        root = QtWidgets.QWidget()
        root_layout = QtWidgets.QVBoxLayout(root)
        root_layout.setContentsMargins(12, 12, 12, 12)
        root_layout.setSpacing(10)

        header = QtWidgets.QHBoxLayout()
        title = QtWidgets.QLabel("OpenFIRE / TR531x PC Input Capture")
        title_font = QtGui.QFont()
        title_font.setPointSize(15)
        title_font.setBold(True)
        title.setFont(title_font)
        self.status_label = QtWidgets.QLabel("starting...")
        self.status_label.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
        header.addWidget(title)
        header.addStretch(1)
        header.addWidget(self.status_label)
        root_layout.addLayout(header)

        panels = QtWidgets.QGridLayout()
        panels.setHorizontalSpacing(12)
        panels.setVerticalSpacing(12)
        root_layout.addLayout(panels, stretch=1)

        self.keyboard_panel = FlowGrid("Keyboard Reports")
        self.mouse_panel = FlowGrid("Mouse Buttons")
        self.mouse_status_panel = FlowGrid("Mouse Activity")
        self.other_panel = FlowGrid("Other Reports")

        panels.addWidget(self.keyboard_panel, 0, 0)
        panels.addWidget(self.mouse_panel, 0, 1)
        panels.addWidget(self.mouse_status_panel, 1, 0)
        panels.addWidget(self.other_panel, 1, 1)

        self._seed_static_labels()

        log_group = QtWidgets.QGroupBox("Debug Window (last 100 lines)")
        log_layout = QtWidgets.QVBoxLayout(log_group)
        self.log_view = QtWidgets.QPlainTextEdit()
        self.log_view.setReadOnly(True)
        self.log_view.setMaximumBlockCount(LOG_LIMIT)
        log_layout.addWidget(self.log_view)
        root_layout.addWidget(log_group, stretch=1)

        self.setCentralWidget(root)

        self.monitor = InputMonitor(only_devices=only_devices, parent=self)
        self.monitor.status_signal.connect(self._set_status)
        self.monitor.log_signal.connect(self._append_log)
        self.monitor.key_signal.connect(self._handle_key)
        self.monitor.mouse_button_signal.connect(self._handle_mouse_button)
        self.monitor.mouse_activity_signal.connect(self._handle_mouse_activity)
        self.monitor.other_signal.connect(self._handle_other)
        self.monitor.start()

    def _seed_static_labels(self) -> None:
        for key in ["LEFT", "RIGHT", "MIDDLE", "SIDE", "EXTRA", "FORWARD", "BACK"]:
            self.mouse_panel.ensure_label(key, key)

        for key, text in {
            "MOVE_X": "MOVE X",
            "MOVE_Y": "MOVE Y",
            "WHEEL_UP": "WHEEL +",
            "WHEEL_DOWN": "WHEEL -",
            "HWHEEL_LEFT": "HWHEEL -",
            "HWHEEL_RIGHT": "HWHEEL +",
        }.items():
            self.mouse_status_panel.ensure_label(key, text)

        for key in ["HAT UP", "HAT DOWN", "HAT LEFT", "HAT RIGHT"]:
            self.other_panel.ensure_label(key, key)

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        self.monitor.stop()
        self.monitor.wait(1500)
        super().closeEvent(event)

    @QtCore.pyqtSlot(str)
    def _set_status(self, text: str) -> None:
        self.status_label.setText(text)

    @QtCore.pyqtSlot(str)
    def _append_log(self, text: str) -> None:
        timestamp = time.strftime("%H:%M:%S")
        self._log_lines.append(f"[{timestamp}] {text}")
        self.log_view.setPlainText("\n".join(self._log_lines))
        cursor = self.log_view.textCursor()
        cursor.movePosition(QtGui.QTextCursor.End)
        self.log_view.setTextCursor(cursor)

    @QtCore.pyqtSlot(str, bool)
    def _handle_key(self, key_name: str, pressed: bool) -> None:
        self.keyboard_panel.set_state(key_name, pressed, key_name)

    @QtCore.pyqtSlot(str, bool)
    def _handle_mouse_button(self, key_name: str, pressed: bool) -> None:
        self.mouse_panel.set_state(key_name, pressed, key_name)

    @QtCore.pyqtSlot(str, str)
    def _handle_mouse_activity(self, key_name: str, text: str) -> None:
        self.mouse_status_panel.set_state(key_name, True, text, transient_ms=TRANSIENT_HIGHLIGHT_MS)

    @QtCore.pyqtSlot(str, bool, str)
    def _handle_other(self, key_name: str, active: bool, text: str) -> None:
        self.other_panel.set_state(key_name, active, text)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Capture keyboard/mouse/gamepad reports from /dev/input.")
    parser.add_argument(
        "--device",
        action="append",
        default=[],
        help="Only monitor specified event device basename, e.g. event5. Can be passed multiple times.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv or sys.argv[1:])
    app = QtWidgets.QApplication(sys.argv)
    app.setStyle("Fusion")
    window = MainWindow(only_devices=args.device)
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
