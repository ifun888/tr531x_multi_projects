#!/usr/bin/env bash
set -euo pipefail

workspace_root="${1:?workspace_root required}"
project_name="${2:?project_name required}"
target="${3:?target required}"

out_dir="${workspace_root}/projects/${project_name}/output/${target}/fwpkg"
if [[ -d "${out_dir}" ]]; then
    find "${out_dir}" -maxdepth 1 -type f -name '*.fwpkg' -exec sha256sum {} \;
fi
