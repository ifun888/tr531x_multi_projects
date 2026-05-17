#!/bin/sh

set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TARGET="standard-tr5310-s"
CONFIG_DIR="$ROOT_DIR/build/config/target_config/tr5310/menuconfig/acore"
CFG_FILE="$CONFIG_DIR/standard_tr5310_s.config"
CFG_OLD_FILE="$CONFIG_DIR/standard_tr5310_s.config.old"
PKG_SRC="$ROOT_DIR/output/tr5310/fwpkg/$TARGET/tr5310_all_in_one.fwpkg"
DEST_DIR="${DEST_DIR:-/mnt/f/wsl-data/TR531X/hardware/烧录/BurnTool_5.0.45/fwpkg}"

set_role_in_file() {
    role="$1"
    file="$2"

    [ -f "$file" ] || return 0

    if [ "$role" = "server" ]; then
        sed -i \
            -e 's/^# CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER is not set$/CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER=y/' \
            -e 's/^CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT=y$/# CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT is not set/' \
            "$file"
    else
        sed -i \
            -e 's/^CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER=y$/# CONFIG_SAMPLE_SUPPORT_SLE_UART_SERVER is not set/' \
            -e 's/^# CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT is not set$/CONFIG_SAMPLE_SUPPORT_SLE_UART_CLIENT=y/' \
            "$file"
    fi
}

set_role() {
    role="$1"
    set_role_in_file "$role" "$CFG_FILE"
    set_role_in_file "$role" "$CFG_OLD_FILE"
}

build_and_copy() {
    role="$1"
    out_name="tr5310_all_in_one_sle_uart_${role}.fwpkg"

    echo "[INFO] Switch role -> $role"
    set_role "$role"

    echo "[INFO] Build $role firmware"
    if ! (cd "$ROOT_DIR" && ./build.py -c "$TARGET"); then
        echo "[ERROR] Build failed for role=$role" >&2
        exit 1
    fi

    if [ ! -f "$PKG_SRC" ]; then
        echo "[ERROR] Firmware not found: $PKG_SRC" >&2
        exit 1
    fi

    mkdir -p "$DEST_DIR"
    if ! cp -avf "$PKG_SRC" "$DEST_DIR/$out_name"; then
        echo "[ERROR] Copy failed, destination may be unavailable: $DEST_DIR" >&2
        exit 1
    fi
    echo "[INFO] Copied -> $DEST_DIR/$out_name"
}

build_and_copy server
build_and_copy client

echo "[INFO] Done. Server/Client firmware compiled and copied."
