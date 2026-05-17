#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "${script_dir}/../.." && pwd)"
sdk_dir="${workspace_root}/sdk_root_dir"
profile_file_default="${script_dir}/build_profile.conf"
cache_dir="${script_dir}/.cache"

project_name=""
target=""
patch_set=""
post_script=""
jobs=""
no_clean=""
profile_file="${profile_file_default}"
project_set_by_cli="false"

usage() {
    echo "Usage: $0 [--project <name>] [--target standard-tr5310-s] [--patch-set default] [--post-script path] [-j N|--jobs N] [--no-clean] [--profile path]"
}

load_profile() {
    local file="$1"
    if [[ -f "${file}" ]]; then
        # shellcheck disable=SC1090
        source "${file}"

        if [[ -z "${project_name}" && -n "${PROJECT:-}" ]]; then project_name="${PROJECT}"; fi
        if [[ -z "${target}" && -n "${TARGET:-}" ]]; then target="${TARGET}"; fi
        if [[ -z "${patch_set}" && -n "${PATCH_SET:-}" ]]; then patch_set="${PATCH_SET}"; fi
        if [[ -z "${jobs}" && -n "${JOBS:-}" ]]; then jobs="${JOBS}"; fi
        if [[ -z "${post_script}" && -n "${POST_SCRIPT:-}" ]]; then post_script="${POST_SCRIPT}"; fi
        if [[ -z "${no_clean}" && -n "${NO_CLEAN:-}" ]]; then
            if [[ "${NO_CLEAN}" == "1" || "${NO_CLEAN}" == "true" || "${NO_CLEAN}" == "TRUE" ]]; then
                no_clean="true"
            else
                no_clean="false"
            fi
        fi
    fi
}

infer_project_from_cwd() {
    local pwd_real projects_root rest
    pwd_real="$(pwd -P)"
    projects_root="${workspace_root}/projects"
    if [[ "${pwd_real}" == "${projects_root}"/* ]]; then
        rest="${pwd_real#${projects_root}/}"
        echo "${rest%%/*}"
        return 0
    fi
    return 1
}

normalize_project_name() {
    local input="$1"
    local projects_root="${workspace_root}/projects"
    if [[ -z "${input}" ]]; then echo ""; return 0; fi
    if [[ "${input}" == projects/* ]]; then
        input="${input#projects/}"
        input="${input%%/*}"
        echo "${input}"
        return 0
    fi
    if [[ -d "${input}" ]]; then
        local abs rest
        abs="$(cd "${input}" && pwd -P)"
        if [[ "${abs}" == "${projects_root}"/* ]]; then
            rest="${abs#${projects_root}/}"
            echo "${rest%%/*}"
            return 0
        fi
    fi
    echo "${input}"
}

build_overlay_signature() {
    local project_dir_in="$1"
    local config_name_in="$2"
    local custom_dir="${project_dir_in}/sdk_overlay/custom"
    local project_cfg="${project_dir_in}/config/${config_name_in}"
    {
        if [[ -d "${custom_dir}" ]]; then
            find "${custom_dir}" -type f -print0 | sort -z | xargs -0 sha256sum 2>/dev/null || true
        fi
        if [[ -f "${project_cfg}" ]]; then
            sha256sum "${project_cfg}"
        fi
    } | sha256sum | awk '{print $1}'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--project) project_name="$2"; project_set_by_cli="true"; shift 2 ;;
        -t|--target) target="$2"; shift 2 ;;
        --patch-set) patch_set="$2"; shift 2 ;;
        --post-script) post_script="$2"; shift 2 ;;
        -j|--jobs) jobs="$2"; shift 2 ;;
        --no-clean) no_clean="true"; shift ;;
        --profile) profile_file="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown arg: $1"; usage; exit 1 ;;
    esac
done

load_profile "${profile_file}"

if inferred_project="$(infer_project_from_cwd)"; then
    if [[ "${project_set_by_cli}" != "true" ]]; then
        project_name="${inferred_project}"
    fi
fi

project_name="$(normalize_project_name "${project_name}")"

if [[ -z "${target}" ]]; then target="standard-tr5310-s"; fi
if [[ -z "${no_clean}" ]]; then no_clean="false"; fi
if [[ -z "${patch_set}" ]]; then patch_set="default"; fi

if [[ -z "${project_name}" ]]; then
    echo "project is empty. Set --project or configure ${profile_file}."
    exit 1
fi

project_dir="${workspace_root}/projects/${project_name}"
if [[ ! -d "${project_dir}" ]]; then
    echo "project not found: ${project_dir}"
    exit 1
fi

config_name="${target//-/_}.config"
current_signature="$(build_overlay_signature "${project_dir}" "${config_name}")"
build_meta_file="${cache_dir}/build_meta_${target}.conf"

if [[ "${no_clean}" == "true" ]]; then
    if [[ ! -f "${build_meta_file}" ]]; then
        echo "[build] no build meta found, force clean build once to initialize cache"
        no_clean="false"
    else
        last_project=""
        last_signature=""
        # shellcheck disable=SC1090
        source "${build_meta_file}"
        if [[ "${last_project}" != "${project_name}" || "${last_signature}" != "${current_signature}" ]]; then
            echo "[build] project/overlay changed, force clean build to avoid stale cache"
            no_clean="false"
        fi
    fi
fi

restore_overlay() {
    "${script_dir}/clean_overlay.sh" "${target}" || true
}
trap restore_overlay EXIT

"${script_dir}/apply_sdk_patches.sh" "${patch_set}"
"${script_dir}/switch_project.sh" "${project_name}" "${target}"

export TR531X_WORKSPACE_ROOT="${workspace_root}"
export TR531X_PROJECT_ROOT="${project_dir}"

(
    cd "${sdk_dir}"
    build_cmd=(python3 build.py)
    if [[ "${no_clean}" != "true" ]]; then
        build_cmd+=("-c")
    fi
    if [[ -n "${jobs}" ]]; then
        build_cmd+=("-j${jobs}")
    fi
    build_cmd+=("${target}")
    "${build_cmd[@]}"
)

"${script_dir}/export_outputs.sh" "${project_name}" "${target}"

if [[ -z "${post_script}" ]]; then
    if [[ -x "${project_dir}/scripts/post_build.sh" ]]; then
        post_script="${project_dir}/scripts/post_build.sh"
    fi
fi

if [[ -n "${post_script}" ]]; then
    "${post_script}" "${workspace_root}" "${project_name}" "${target}"
fi

mkdir -p "${cache_dir}"
cat > "${build_meta_file}" <<EOM
last_project=${project_name}
last_signature=${current_signature}
EOM

echo "[build] done: project=${project_name} target=${target}"
echo "[build] profile: ${profile_file}"
