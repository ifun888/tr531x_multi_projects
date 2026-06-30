#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
build_tool="${repo_root}/tools/tr531x/build_project.sh"

target="standard-tr5310-s"
patch_set="default"
mode="all"
jobs=""
no_clean="false"

usage() {
    cat <<USAGE
Usage:
  $(basename "$0") [tx|rx|all] [options]

Modes:
  tx      Build only projects/sle_low_latency_tx
  rx      Build only projects/sle_low_latency_rx
  all     Build both tx and rx (default)

Options:
  --target <name>      Build target. Default: ${target}
  --patch-set <name>   Patch set. Default: ${patch_set}
  -j, --jobs <n>       Parallel jobs passed to build_project.sh
  --no-clean           Reuse existing build output when possible
  -h, --help           Show this help
USAGE
}

if [[ $# -gt 0 ]]; then
    case "$1" in
        tx|rx|all)
            mode="$1"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
    esac
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --target)
            target="$2"
            shift 2
            ;;
        --patch-set)
            patch_set="$2"
            shift 2
            ;;
        -j|--jobs)
            jobs="$2"
            shift 2
            ;;
        --no-clean)
            no_clean="true"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [[ ! -x "${build_tool}" ]]; then
    echo "Build tool not found: ${build_tool}" >&2
    exit 1
fi

build_one() {
    local project="$1"
    local cmd=("${build_tool}" --project "${project}" --target "${target}" --patch-set "${patch_set}")

    if [[ -n "${jobs}" ]]; then
        cmd+=(--jobs "${jobs}")
    fi
    if [[ "${no_clean}" == "true" ]]; then
        cmd+=(--no-clean)
    fi

    echo "[sle-low-latency] building ${project} target=${target} patch_set=${patch_set}"
    (
        cd "${repo_root}"
        "${cmd[@]}"
    )
}

case "${mode}" in
    tx)
        build_one "sle_low_latency_tx"
        ;;
    rx)
        build_one "sle_low_latency_rx"
        ;;
    all)
        build_one "sle_low_latency_tx"
        build_one "sle_low_latency_rx"
        ;;
    *)
        echo "Invalid mode: ${mode}" >&2
        exit 1
        ;;
esac

echo "[sle-low-latency] done: mode=${mode}"
