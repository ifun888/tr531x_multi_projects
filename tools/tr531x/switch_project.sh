#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <project_name> <target>"
    exit 1
fi

project_name="$1"
target="$2"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "${script_dir}/../.." && pwd)"
sdk_dir="${workspace_root}/sdk_root_dir"
project_dir="${workspace_root}/projects/${project_name}"
cache_dir="${script_dir}/.cache"

if [[ ! -d "${project_dir}" ]]; then
    echo "[switch] project not found: ${project_dir}"
    exit 1
fi

custom_src="${project_dir}/sdk_overlay/custom"
custom_dst="${sdk_dir}/application/samples/custom"
if [[ ! -d "${custom_src}" ]]; then
    echo "[switch] custom overlay not found: ${custom_src}"
    exit 1
fi

if [[ -L "${custom_dst}" ]]; then
    rm -f "${custom_dst}"
elif [[ -d "${custom_dst}" ]]; then
    rm -rf "${custom_dst}"
fi
ln -s "${custom_src}" "${custom_dst}"

chip="tr5310"
if [[ "${target}" == *"tr5310p"* ]]; then
    chip="tr5310p"
elif [[ "${target}" == *"tr5312l"* ]]; then
    chip="tr5312l"
elif [[ "${target}" == *"tr5316"* ]]; then
    chip="tr5316"
fi

config_name="${target//-/_}.config"
project_cfg="${project_dir}/config/${config_name}"
sdk_cfg="${sdk_dir}/build/config/target_config/${chip}/menuconfig/acore/${config_name}"
backup_cfg="${cache_dir}/${config_name}.bak"

if [[ ! -f "${project_cfg}" ]]; then
    echo "[switch] project config not found: ${project_cfg}"
    exit 1
fi
if [[ ! -f "${sdk_cfg}" ]]; then
    echo "[switch] sdk config not found: ${sdk_cfg}"
    exit 1
fi

mkdir -p "${cache_dir}"
cp -f "${sdk_cfg}" "${backup_cfg}"
cp -f "${project_cfg}" "${sdk_cfg}"

echo "[switch] project=${project_name} target=${target} chip=${chip}"
echo "[switch] linked ${custom_dst} -> ${custom_src}"
echo "[switch] config  ${project_cfg} -> ${sdk_cfg}"
