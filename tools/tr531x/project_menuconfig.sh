#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "${script_dir}/../.." && pwd)"
sdk_dir="${workspace_root}/sdk_root_dir"
profile_file="${script_dir}/build_profile.conf"

project_name=""
target=""
patch_set=""
save_to_sdk_patch="false"
patch_note=""

usage() {
    echo "Usage: $0 [--project <name>] [--target standard-tr5310-s] [--patch-set default] [--save-to-sdk-patch] [--patch-note \"remark\"]"
}

slugify() { echo "$1" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]+/-/g; s/^-//; s/-$//'; }

next_patch_index() {
    local patch_dir="$1" max=0 f base num
    shopt -s nullglob
    for f in "${patch_dir}"/*.patch; do
        base="$(basename "${f}")"
        num="${base%%-*}"
        if [[ "${num}" =~ ^[0-9]+$ ]] && (( 10#${num} > max )); then max=$((10#${num})); fi
    done
    printf "%04d" $((max + 1))
}

normalize_project_name() {
    local input="$1"
    local projects_root="${workspace_root}/projects"
    if [[ -z "${input}" ]]; then echo ""; return 0; fi
    if [[ "${input}" == projects/* ]]; then input="${input#projects/}"; input="${input%%/*}"; echo "${input}"; return 0; fi
    if [[ -d "${input}" ]]; then
        local abs rest
        abs="$(cd "${input}" && pwd -P)"
        if [[ "${abs}" == "${projects_root}"/* ]]; then rest="${abs#${projects_root}/}"; echo "${rest%%/*}"; return 0; fi
    fi
    echo "${input}"
}

infer_project_from_cwd() {
    local pwd_real projects_root rest
    pwd_real="$(pwd -P)"
    projects_root="${workspace_root}/projects"
    if [[ "${pwd_real}" == "${projects_root}"/* ]]; then rest="${pwd_real#${projects_root}/}"; echo "${rest%%/*}"; return 0; fi
    return 1
}

load_profile_if_exists() {
    if [[ -f "${profile_file}" ]]; then
        # shellcheck disable=SC1090
        source "${profile_file}"
        [[ -z "${project_name}" && -n "${PROJECT:-}" ]] && project_name="${PROJECT}"
        [[ -z "${target}" && -n "${TARGET:-}" ]] && target="${TARGET}"
        [[ -z "${patch_set}" && -n "${PATCH_SET:-}" ]] && patch_set="${PATCH_SET}"
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--project) project_name="$2"; shift 2 ;;
        -t|--target) target="$2"; shift 2 ;;
        --patch-set) patch_set="$2"; shift 2 ;;
        --save-to-sdk-patch) save_to_sdk_patch="true"; shift ;;
        --patch-note) patch_note="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown arg: $1"; usage; exit 1 ;;
    esac
done

load_profile_if_exists
if inferred_project="$(infer_project_from_cwd)"; then project_name="${inferred_project}"; fi
project_name="$(normalize_project_name "${project_name}")"

[[ -n "${project_name}" ]] || { echo "project is empty. Set --project or run from projects/<name> directory."; exit 1; }
[[ -n "${target}" ]] || target="standard-tr5310-s"
[[ -n "${patch_set}" ]] || patch_set="default"
if [[ "${save_to_sdk_patch}" == "true" && -z "${patch_note}" ]]; then read -rp "Patch note: " patch_note; fi

chip="tr5310"
if [[ "${target}" == *"tr5310p"* ]]; then chip="tr5310p"; elif [[ "${target}" == *"tr5312l"* ]]; then chip="tr5312l"; elif [[ "${target}" == *"tr5316"* ]]; then chip="tr5316"; fi

project_dir="${workspace_root}/projects/${project_name}"
[[ -d "${project_dir}" ]] || { echo "project not found: ${project_dir}"; exit 1; }

restore_overlay() { "${script_dir}/clean_overlay.sh" "${target}" || true; }
trap restore_overlay EXIT

"${script_dir}/apply_sdk_patches.sh" "${patch_set}"
"${script_dir}/switch_project.sh" "${project_name}" "${target}"

config_name="${target//-/_}.config"
sdk_cfg="${sdk_dir}/build/config/target_config/${chip}/menuconfig/acore/${config_name}"
project_cfg="${project_dir}/config/${config_name}"
project_patch_dir="${project_dir}/config/patches"
project_cfg_patch="${project_patch_dir}/${config_name}.patch"

(
    cd "${sdk_dir}"
    python3 build.py "${target}" menuconfig
)

mkdir -p "${project_dir}/config"
cp -f "${sdk_cfg}" "${project_cfg}"

mkdir -p "${project_patch_dir}"
baseline_tmp="$(mktemp)"
cleanup_tmp() { rm -f "${baseline_tmp}"; }
trap cleanup_tmp RETURN

cp -f "${script_dir}/.cache/${config_name}.bak" "${baseline_tmp}" 2>/dev/null || cp -f "${sdk_cfg}" "${baseline_tmp}"

diff -u --label "a/sdk_root_dir/build/config/target_config/${chip}/menuconfig/acore/${config_name}" --label "b/projects/${project_name}/config/${config_name}" "${baseline_tmp}" "${project_cfg}" > "${project_cfg_patch}" || true

echo "[menuconfig] saved project config: ${project_cfg}"
echo "[menuconfig] saved config patch: ${project_cfg_patch}"

if [[ "${save_to_sdk_patch}" == "true" ]]; then
    patch_dir="${workspace_root}/patches/sdk/sdk_root_dir/${patch_set}"
    notes_file="${patch_dir}/PATCH_NOTES.txt"
    mkdir -p "${patch_dir}"

    if cmp -s "${baseline_tmp}" "${sdk_cfg}"; then
        echo "[menuconfig] no sdk config change, skip sdk patch export"
    else
        idx="$(next_patch_index "${patch_dir}")"
        target_slug="$(slugify "${target}")"
        note_slug="$(slugify "${patch_note}")"
        [[ -z "${note_slug}" ]] && note_slug="menuconfig"
        sdk_patch="${patch_dir}/${idx}-menuconfig-${project_name}-${target_slug}-${note_slug}.patch"

        diff -u --label "a/build/config/target_config/${chip}/menuconfig/acore/${config_name}" --label "b/build/config/target_config/${chip}/menuconfig/acore/${config_name}" "${baseline_tmp}" "${sdk_cfg}" > "${sdk_patch}" || true

        if [[ -s "${sdk_patch}" ]]; then
            echo "$(basename "${sdk_patch}") | ${patch_note}" >> "${notes_file}"
            echo "[menuconfig] saved sdk patch: ${sdk_patch}"
            echo "[menuconfig] updated notes: ${notes_file}"
        else
            rm -f "${sdk_patch}"
            echo "[menuconfig] no sdk patch generated (empty diff)"
        fi
    fi
fi
