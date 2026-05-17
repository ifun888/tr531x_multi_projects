#!/usr/bin/env bash
set -euo pipefail

patch_set="${1:-default}"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"${script_dir}/apply_sdk_patches.sh" "${patch_set}"
