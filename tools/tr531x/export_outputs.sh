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
sdk_root="${workspace_root}/sdk_root_dir"

chip="tr5310"
if [[ "${target}" == *"tr5310p"* ]]; then
    chip="tr5310p"
elif [[ "${target}" == *"tr5312l"* ]]; then
    chip="tr5312l"
elif [[ "${target}" == *"tr5316"* ]]; then
    chip="tr5316"
fi

src_acore="${sdk_root}/output/${chip}/acore/${target}"
src_fwpkg="${sdk_root}/output/${chip}/fwpkg/${target}"

dst_root="${workspace_root}/projects/${project_name}/output/${target}"
dst_acore="${dst_root}/acore"
dst_fwpkg="${dst_root}/fwpkg"

mkdir -p "${dst_acore}" "${dst_fwpkg}"

if command -v rsync >/dev/null 2>&1; then
    if [[ -d "${src_acore}" ]]; then
        rsync -a --delete --include='*/' --include='*.elf' --include='*.map' --include='*.bin' --include='*.hex' --include='*.nm' --exclude='*' "${src_acore}/" "${dst_acore}/"
    fi
    if [[ -d "${src_fwpkg}" ]]; then
        rsync -a --delete --include='*/' --include='*.fwpkg' --exclude='*' "${src_fwpkg}/" "${dst_fwpkg}/"
    fi
else
    if [[ -d "${src_acore}" ]]; then
        find "${src_acore}" -maxdepth 1 -type f \( -name '*.elf' -o -name '*.map' -o -name '*.bin' -o -name '*.hex' -o -name '*.nm' \) -exec cp -af {} "${dst_acore}/" \;
    fi
    if [[ -d "${src_fwpkg}" ]]; then
        find "${src_fwpkg}" -maxdepth 1 -type f -name '*.fwpkg' -exec cp -af {} "${dst_fwpkg}/" \;
    fi
fi

echo "[export] acore -> ${dst_acore}"
echo "[export] fwpkg -> ${dst_fwpkg}"
