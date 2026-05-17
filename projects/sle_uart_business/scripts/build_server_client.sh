#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "${script_dir}/.." && pwd)"
workspace_root="$(cd "${project_dir}/../.." && pwd)"
project_name="sle_uart_business"
target="standard-tr5310-s"
patch_set="default"

cfg_main="${project_dir}/config/standard_tr5310_s.config"
cfg_client="${project_dir}/config/standard_tr5310_s_client.config"
cfg_server="${project_dir}/config/standard_tr5310_s_server.config"
role_out_dir="${project_dir}/output/${target}/role_fwpkg"

[[ -f "${cfg_client}" ]] || { echo "missing ${cfg_client}"; exit 1; }
[[ -f "${cfg_server}" ]] || { echo "missing ${cfg_server}"; exit 1; }

mkdir -p "${role_out_dir}"

backup_cfg="$(mktemp)"
cp -f "${cfg_main}" "${backup_cfg}"
cleanup() {
    cp -f "${backup_cfg}" "${cfg_main}" || true
    rm -f "${backup_cfg}"
}
trap cleanup EXIT

build_role() {
    local role="$1"
    local role_cfg="$2"
    local out_name="tr5310_all_in_one_sle_uart_${role}.fwpkg"
    local src_pkg="${project_dir}/output/${target}/fwpkg/tr5310_all_in_one.fwpkg"

    cp -f "${role_cfg}" "${cfg_main}"
    "${workspace_root}/tools/tr531x/build_project.sh" --project "${project_name}" --target "${target}" --patch-set "${patch_set}" --no-clean

    if [[ -f "${src_pkg}" ]]; then
        cp -f "${src_pkg}" "${role_out_dir}/${out_name}"
        echo "[role] exported ${role_out_dir}/${out_name}"
    else
        echo "[role] missing package: ${src_pkg}"
        exit 1
    fi
}

build_role "server" "${cfg_server}"
build_role "client" "${cfg_client}"

echo "[role] done"
