#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "${script_dir}/../.." && pwd)"
sdk_dir="${workspace_root}/sdk_root_dir"
patch_root="${workspace_root}/patches/sdk/sdk_root_dir"

patch_set="${1:-}"
if [[ -z "${patch_set}" ]]; then
    patch_set="default"
fi

patch_dir="${patch_root}/${patch_set}"
if [[ ! -d "${patch_dir}" ]]; then
    echo "[patch] no patch directory: ${patch_dir}, skip"
    exit 0
fi

shopt -s nullglob
patches=("${patch_dir}"/*.patch)
if [[ ${#patches[@]} -eq 0 ]]; then
    echo "[patch] no patch files in ${patch_dir}, skip"
    exit 0
fi

use_git="false"
if git -C "${sdk_dir}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    use_git="true"
fi

for p in "${patches[@]}"; do
    if [[ "${use_git}" == "true" ]]; then
        if git -C "${sdk_dir}" apply --reverse --check "${p}" >/dev/null 2>&1; then
            echo "[patch] already applied: $(basename "${p}")"
            continue
        fi
        git -C "${sdk_dir}" apply --check "${p}"
        git -C "${sdk_dir}" apply "${p}"
    else
        if patch -d "${sdk_dir}" -p1 -R --dry-run < "${p}" >/dev/null 2>&1; then
            echo "[patch] already applied: $(basename "${p}")"
            continue
        fi
        patch -d "${sdk_dir}" -p1 --dry-run < "${p}" >/dev/null
        patch -d "${sdk_dir}" -p1 < "${p}" >/dev/null
    fi
    echo "[patch] applied: $(basename "${p}")"
done
