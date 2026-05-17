#!/usr/bin/env bash
set -euo pipefail

name=""
patch_set=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --name) name="$2"; shift 2 ;;
        --patch-set) patch_set="$2"; shift 2 ;;
        *) echo "Unknown arg: $1"; echo "Usage: $0 --name <message> [--patch-set <id>]"; exit 1 ;;
    esac
done

if [[ -z "${name}" ]]; then
    echo "--name is required"
    exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "${script_dir}/../.." && pwd)"
sdk_dir="${workspace_root}/sdk_root_dir"

if [[ -z "${patch_set}" ]]; then patch_set="default"; fi
patch_dir="${workspace_root}/patches/sdk/sdk_root_dir/${patch_set}"
mkdir -p "${patch_dir}"

tmp_old="$(mktemp -d)"
cleanup() { rm -rf "${tmp_old}"; }
trap cleanup EXIT

if [[ -d "${script_dir}/.cache/baseline_snapshot" ]]; then
    cp -a "${script_dir}/.cache/baseline_snapshot/." "${tmp_old}/"
else
    echo "[patch] baseline snapshot missing: tools/tr531x/.cache/baseline_snapshot"
    echo "[patch] this helper currently requires a baseline snapshot to diff against"
    exit 1
fi

slug="$(echo "${name}" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]+/-/g; s/^-//; s/-$//')"
patch_file="${patch_dir}/0001-${slug}.patch"
diff -ruN --label a --label b "${tmp_old}" "${sdk_dir}" > "${patch_file}" || true

if [[ ! -s "${patch_file}" ]]; then
    rm -f "${patch_file}"
    echo "[patch] no changes to save"
    exit 1
fi

echo "[patch] saved to ${patch_file}"
