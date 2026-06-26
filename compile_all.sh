#!/bin/bash

set -euo pipefail

SILENT=${SILENT:-false}
PARALLEL=${PARALLEL:-0}
FAILED_FILE=.failed
read -r -a qmk_args <<< "${QMK_ARGS:-}"

if [[ $# -gt 0 ]]; then
    TAG=$1
else
    TAG=$(git describe --tags --abbrev=0 2>/dev/null || git rev-parse --short HEAD)
fi

TARGET_TAG=${TAG//\//-}
VERSION_TOKEN=$TAG

read -r -a keyboard_list <<< "${KEYBOARDS:-air75v2 halo75v2 halo96v2 gem80}"
read -r -a layout_list <<< "${LAYOUTS:-ansi iso}"
read -r -a keymap_list <<< "${KEYMAPS:-default via ryodeushii}"

build_count=0
failure_count=0

log_info() {
    printf 'Compiled keyboard:%s layout:%s keymap:%s target:%s\n' "$1" "$2" "$3" "$4"
}

log_error() {
    printf 'Failed keyboard:%s layout:%s keymap:%s target:%s\n' "$1" "$2" "$3" "$4" >&2
    printf '%s\n' "$4" >> "$FAILED_FILE"
    failure_count=$((failure_count + 1))
}

matches_filter() {
    local value=$1
    shift

    local candidate
    for candidate in "$@"; do
        if [[ "$candidate" == "$value" ]]; then
            return 0
        fi
    done

    return 1
}

discover_targets() {
    shopt -s nullglob

    local keymap_path relative keyboard layout keymap
    for keymap_path in keyboards/nuphy/*/*/keymaps/*/keymap.c; do
        relative=${keymap_path#keyboards/nuphy/}
        IFS=/ read -r keyboard layout _ keymap _ <<< "$relative"

        matches_filter "$keyboard" "${keyboard_list[@]}" || continue
        matches_filter "$layout" "${layout_list[@]}" || continue
        matches_filter "$keymap" "${keymap_list[@]}" || continue

        printf '%s %s %s\n' "$keyboard" "$layout" "$keymap"
    done

    shopt -u nullglob
}

compile_target() {
    local keyboard=$1
    local layout=$2
    local keymap=$3
    local prefix=${4:-}
    local work_mode=${5:-}
    local target="${prefix}${keyboard}-${layout}-${keymap}-${TARGET_TAG}"
    local extraflags="-DCFW_VERSION_TOKEN=${VERSION_TOKEN}"
    local cmd

    build_count=$((build_count + 1))

    if [[ -n "$work_mode" ]]; then
        extraflags+=" -DWORK_MODE=$work_mode"
    fi

    # Inject release metadata at compile time instead of editing tracked config files.
    cmd=(
        qmk compile
        "${qmk_args[@]}"
        -kb "nuphy/$keyboard/$layout"
        -km "$keymap"
        -j "$PARALLEL"
        -c
        -e "TARGET=$target"
        -e "SILENT=$SILENT"
        -e "EXTRAFLAGS=$extraflags"
    )

    if "${cmd[@]}"; then
        log_info "$keyboard" "$layout" "$keymap" "$target"
    else
        log_error "$keyboard" "$layout" "$keymap" "$target"
    fi
}

rm -f "$FAILED_FILE"

mapfile -t targets < <(discover_targets)

if [[ ${#targets[@]} -eq 0 ]]; then
    printf 'No matching build targets found for KEYBOARDS=%s LAYOUTS=%s KEYMAPS=%s\n' "${KEYBOARDS:-air75v2 halo75v2 halo96v2 gem80}" "${LAYOUTS:-ansi iso}" "${KEYMAPS:-default via ryodeushii}" >&2
    exit 1
fi

for entry in "${targets[@]}"; do
    read -r keyboard layout keymap <<< "$entry"

    if [[ "$keyboard" == "gem80" ]]; then
        compile_target "$keyboard" "$layout" "$keymap" "wired-" "USB_MODE"
        compile_target "$keyboard" "$layout" "$keymap" "threemode-" "THREE_MODE"
    else
        compile_target "$keyboard" "$layout" "$keymap"
    fi
done

printf 'Completed %d builds with %d failures.\n' "$build_count" "$failure_count"

if [[ -f "$FAILED_FILE" ]]; then
    printf 'See %s for failed targets.\n' "$FAILED_FILE" >&2
    exit 1
fi
