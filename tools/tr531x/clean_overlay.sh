#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <target>"
    exit 1
fi

target="$1"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "${script_dir}/../.." && pwd)"
sdk_dir="${workspace_root}/sdk_root_dir"
cache_dir="${script_dir}/.cache"

chip="tr5310"
if [[ "${target}" == *"tr5310p"* ]]; then
    chip="tr5310p"
elif [[ "${target}" == *"tr5312l"* ]]; then
    chip="tr5312l"
elif [[ "${target}" == *"tr5316"* ]]; then
    chip="tr5316"
fi

config_name="${target//-/_}.config"
sdk_cfg="${sdk_dir}/build/config/target_config/${chip}/menuconfig/acore/${config_name}"
backup_cfg="${cache_dir}/${config_name}.bak"
custom_dst="${sdk_dir}/application/samples/custom"

if [[ -f "${backup_cfg}" ]]; then
    cp -f "${backup_cfg}" "${sdk_cfg}"
    rm -f "${backup_cfg}"
    echo "[clean] restored ${sdk_cfg}"
fi

if [[ -L "${custom_dst}" ]]; then
    rm -f "${custom_dst}"
    echo "[clean] removed ${custom_dst} symlink"
fi
