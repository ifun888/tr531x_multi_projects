#!/usr/bin/env bash
set -euo pipefail

name="${1:-}"
if [[ -z "${name}" ]]; then
    echo "Usage: $0 <patch_name>"
    exit 1
fi

echo "[patch] sdk_root_dir is not a git repo in this workspace."
echo "[patch] create changes manually, then use patch_save.sh to export unified patch."
