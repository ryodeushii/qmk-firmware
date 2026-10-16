#!/bin/bash

set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: ./compile_all.sh --profile <release|personal> --manifest <path> [--tag <tag>] [--staging-dir <directory>]
USAGE
}

PROFILE=""
MANIFEST=""
TAG=""
TAG_PROVIDED=false
STAGING_DIR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --profile)
      [[ $# -ge 2 ]] || { printf '%s\n' 'Missing value for --profile' >&2; usage >&2; exit 2; }
      PROFILE=$2
      shift 2
      ;;
    --manifest)
      [[ $# -ge 2 ]] || { printf '%s\n' 'Missing value for --manifest' >&2; usage >&2; exit 2; }
      MANIFEST=$2
      shift 2
      ;;
    --tag)
      [[ $# -ge 2 ]] || { printf '%s\n' 'Missing value for --tag' >&2; usage >&2; exit 2; }
      TAG=$2
      TAG_PROVIDED=true
      shift 2
      ;;
    --staging-dir)
      [[ $# -ge 2 ]] || { printf '%s\n' 'Missing value for --staging-dir' >&2; usage >&2; exit 2; }
      STAGING_DIR=$2
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown argument: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ -z "$PROFILE" ]]; then
  printf '%s\n' 'Missing required argument: --profile' >&2
  usage >&2
  exit 2
fi

if [[ "$PROFILE" != "release" && "$PROFILE" != "personal" ]]; then
  printf 'Unknown profile: %s\n' "$PROFILE" >&2
  exit 2
fi

if [[ -z "$MANIFEST" ]]; then
  printf '%s\n' 'Missing required argument: --manifest' >&2
  usage >&2
  exit 2
fi

if [[ "$TAG_PROVIDED" == true && -z "$TAG" ]]; then
  printf '%s\n' 'Explicit --tag cannot be empty' >&2
  exit 2
fi

REPO_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
VALIDATOR="$REPO_ROOT/util/ci/nuphy_release_manifest.py"

if [[ ! -f "$VALIDATOR" ]]; then
  printf 'Manifest validator not found: %s\n' "$VALIDATOR" >&2
  exit 1
fi

resolve_path() {
  local path=$1

  if [[ "$path" != /* ]]; then
    path="$REPO_ROOT/$path"
  fi

  realpath -m -- "$path"
}

MANIFEST_PATH=$(resolve_path "$MANIFEST")
if [[ ! -f "$MANIFEST_PATH" ]]; then
  printf 'Manifest not found: %s\n' "$MANIFEST_PATH" >&2
  exit 1
fi

validate_staging_files() {
  local directory=$1
  local extension=$2
  local entry

  for entry in "$directory"/* "$directory"/.[!.]* "$directory"/..?*; do
    if [[ ! -e "$entry" && ! -L "$entry" ]]; then
      continue
    fi
    if [[ -L "$entry" || ! -f "$entry" ]]; then
      return 1
    fi
    case "$entry" in
      *."$extension") ;;
      *) return 1 ;;
    esac
  done
}

validate_staging_contents() {
  local entry
  local directory_name

  if [[ ! -e "$STAGING_PATH" && ! -L "$STAGING_PATH" ]]; then
    return 0
  fi
  if [[ -L "$STAGING_PATH" || ! -d "$STAGING_PATH" ]]; then
    return 1
  fi

  for entry in "$STAGING_PATH"/* "$STAGING_PATH"/.[!.]* "$STAGING_PATH"/..?*; do
    if [[ ! -e "$entry" && ! -L "$entry" ]]; then
      continue
    fi
    if [[ -L "$entry" || ! -d "$entry" ]]; then
      return 1
    fi

    directory_name=$(basename -- "$entry")
    case "$directory_name" in
      firmware)
        validate_staging_files "$entry" bin || return 1
        ;;
      metadata)
        [[ "$PROFILE" == "release" ]] || return 1
        validate_staging_files "$entry" json || return 1
        ;;
      *)
        return 1
        ;;
    esac
  done
}

if [[ -n "$STAGING_DIR" ]]; then
  staging_input_path="$STAGING_DIR"
  if [[ "$staging_input_path" != /* ]]; then
    staging_input_path="$REPO_ROOT/$staging_input_path"
  fi
  if [[ -L "$staging_input_path" ]]; then
    printf 'Refusing symlink staging directory: %s\n' "$staging_input_path" >&2
    exit 2
  fi

  STAGING_PATH=$(resolve_path "$STAGING_DIR")
  if [[ "$STAGING_PATH" == "$REPO_ROOT" || "$STAGING_PATH" == "/" ]]; then
    printf 'Refusing unsafe staging directory: %s\n' "$STAGING_PATH" >&2
    exit 2
  fi
  if [[ "$STAGING_PATH" == "$REPO_ROOT/"* && "$STAGING_PATH" != "$REPO_ROOT/${PROFILE}-artifacts" ]]; then
    printf 'Refusing in-repository staging directory outside %s-artifacts: %s\n' "$PROFILE" "$STAGING_PATH" >&2
    exit 2
  fi
  case "$REPO_ROOT/" in
    "$STAGING_PATH"/*)
      printf 'Refusing staging directory that contains repository root: %s\n' "$STAGING_PATH" >&2
      exit 2
      ;;
  esac
  if ! validate_staging_contents; then
    printf 'Refusing unsafe staging directory contents: %s\n' "$STAGING_PATH" >&2
    exit 2
  fi
else
  STAGING_PATH=""
fi

if [[ "$TAG_PROVIDED" == false ]]; then
  TAG=$(git -C "$REPO_ROOT" describe --tags --abbrev=0 2>/dev/null || git -C "$REPO_ROOT" rev-parse --short HEAD)
fi

SILENT=${SILENT:-false}
PARALLEL=${PARALLEL:-0}
FAILED_FILE="$REPO_ROOT/.failed"
read -r -a qmk_args <<<"${QMK_ARGS:-}"
read -r -a keyboard_filter <<<"${KEYBOARDS:-}"
read -r -a layout_filter <<<"${LAYOUTS:-}"
read -r -a keymap_filter <<<"${KEYMAPS:-}"

matches_filter() {
  local value=$1
  shift

  if [[ $# -eq 0 ]]; then
    return 0
  fi

  local candidate
  for candidate in "$@"; do
    if [[ "$candidate" == "$value" ]]; then
      return 0
    fi
  done

  return 1
}

log_info() {
  printf 'Compiled id:%s keyboard:%s layout:%s keymap:%s mode:%s target:%s\n' "$1" "$2" "$3" "$4" "$5" "$6"
}

log_error() {
  printf 'Failed id:%s keyboard:%s layout:%s keymap:%s mode:%s target:%s\n' "$1" "$2" "$3" "$4" "$5" "$6" >&2
  printf '%s\n' "$1" >>"$FAILED_FILE"
  failure_count=$((failure_count + 1))
}

SIGNALRGB_DIR="$REPO_ROOT/modules/signalrgb"

restore_signalrgb_version() {
  local exit_status=$?

  if ! git -C "$SIGNALRGB_DIR" checkout -- qmk_version.h >/dev/null 2>&1; then
    printf 'Failed to restore SignalRGB version file in %s\n' "$SIGNALRGB_DIR" >&2
    exit_status=1
  fi

  return "$exit_status"
}

cd "$REPO_ROOT"
rm -f -- "$FAILED_FILE"

# Validate before resolving targets or invoking qmk compile.
python3 "$VALIDATOR" validate --manifest "$MANIFEST_PATH" --repo-root "$REPO_ROOT"

target_json=$(python3 "$VALIDATOR" targets \
  --manifest "$MANIFEST_PATH" \
  --repo-root "$REPO_ROOT" \
  --profile "$PROFILE" \
  --tag "$TAG")
target_tsv=$(python3 -c '
import json
import sys

fields = ("id", "keyboard", "layout", "keymap", "work_mode", "binary")
for target in json.load(sys.stdin):
    print("\t".join(target[field] for field in fields))
' <<<"$target_json")

target_records=()
if [[ -n "$target_tsv" ]]; then
  mapfile -t target_records <<<"$target_tsv"
fi

selected_targets=()
for record in "${target_records[@]}"; do
  IFS=$'\t' read -r target_id keyboard layout keymap work_mode binary <<<"$record"

  if ! matches_filter "$keyboard" "${keyboard_filter[@]}" \
    || ! matches_filter "$layout" "${layout_filter[@]}" \
    || ! matches_filter "$keymap" "${keymap_filter[@]}"; then
    continue
  fi

  selected_targets+=("$record")
done

if [[ ${#selected_targets[@]} -eq 0 ]]; then
  printf 'No matching manifest targets for profile=%s KEYBOARDS=%s LAYOUTS=%s KEYMAPS=%s\n' \
    "$PROFILE" "${KEYBOARDS:-}" "${LAYOUTS:-}" "${KEYMAPS:-}" >&2
  exit 1
fi

if [[ -n "$STAGING_PATH" ]]; then
  mkdir -p -- "$STAGING_PATH"
  rm -rf -- "$STAGING_PATH/firmware"
  if [[ "$PROFILE" == "release" ]]; then
    rm -rf -- "$STAGING_PATH/metadata"
  fi
  mkdir -p -- "$STAGING_PATH/firmware"
  if [[ "$PROFILE" == "release" ]]; then
    mkdir -p -- "$STAGING_PATH/metadata"
  fi
fi

build_count=0
failure_count=0

compile_target() {
  local target_id=$1
  local keyboard=$2
  local layout=$3
  local keymap=$4
  local work_mode=$5
  local binary=$6
  local target=${binary%.bin}
  local artifact_path="$REPO_ROOT/$binary"
  local extraflags="-DCFW_VERSION_TOKEN=${TAG} -DWORK_MODE=${work_mode}"
  local cmd

  build_count=$((build_count + 1))
  rm -f -- "$artifact_path"

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
    if [[ ! -f "$artifact_path" ]]; then
      log_error "$target_id" "$keyboard" "$layout" "$keymap" "$work_mode" "$target"
      return
    fi

    if [[ -n "$STAGING_PATH" ]]; then
      if ! cp -- "$artifact_path" "$STAGING_PATH/firmware/$binary"; then
        log_error "$target_id" "$keyboard" "$layout" "$keymap" "$work_mode" "$target"
        return
      fi
    fi
    log_info "$target_id" "$keyboard" "$layout" "$keymap" "$work_mode" "$target"
  else
    log_error "$target_id" "$keyboard" "$layout" "$keymap" "$work_mode" "$target"
  fi
}

if [[ -f "$SIGNALRGB_DIR/gen-version.sh" ]]; then
  trap restore_signalrgb_version EXIT
  printf '%s\n' 'Updating QMK version for SignalRGB module...'
  (cd "$SIGNALRGB_DIR" && ./gen-version.sh) || {
    printf '%s\n' 'Failed to update QMK version for SignalRGB module' >&2
    exit 1
  }
fi

for record in "${selected_targets[@]}"; do
  IFS=$'\t' read -r target_id keyboard layout keymap work_mode binary <<<"$record"
  compile_target "$target_id" "$keyboard" "$layout" "$keymap" "$work_mode" "$binary"
done

printf 'Completed %d builds with %d failures.\n' "$build_count" "$failure_count"

if [[ "$failure_count" -ne 0 ]]; then
  printf 'See %s for failed target IDs.\n' "$FAILED_FILE" >&2
  exit 1
fi

if [[ -n "$STAGING_PATH" ]]; then
  expected_binary_count=${#selected_targets[@]}
  actual_binary_count=$(find "$STAGING_PATH/firmware" -maxdepth 1 -type f -name '*.bin' -print | wc -l)
  if [[ "$actual_binary_count" -ne "$expected_binary_count" ]]; then
    printf 'Expected %d staged binaries, found %d\n' "$expected_binary_count" "$actual_binary_count" >&2
    exit 1
  fi

  for record in "${selected_targets[@]}"; do
    IFS=$'\t' read -r _ _ _ _ _ binary <<<"$record"
    if [[ ! -f "$STAGING_PATH/firmware/$binary" ]]; then
      printf 'Missing staged binary: %s\n' "$STAGING_PATH/firmware/$binary" >&2
      exit 1
    fi
  done

  if [[ "$PROFILE" == "release" ]]; then
    metadata_json=$(python3 "$VALIDATOR" metadata \
      --manifest "$MANIFEST_PATH" \
      --repo-root "$REPO_ROOT")
    metadata_paths=$(python3 -c '
import json
import sys

for path in json.load(sys.stdin):
    print(path)
' <<<"$metadata_json")

    metadata_count=0
    metadata_copy_failed=false
    while IFS= read -r metadata_path; do
      [[ -n "$metadata_path" ]] || continue
      source_path="$REPO_ROOT/$metadata_path"
      destination_path="$STAGING_PATH/metadata/$(basename -- "$metadata_path")"
      if [[ ! -f "$source_path" ]]; then
        printf 'Missing release metadata: %s\n' "$source_path" >&2
        metadata_copy_failed=true
        continue
      fi
      if ! cp -- "$source_path" "$destination_path"; then
        printf 'Failed to stage release metadata: %s\n' "$metadata_path" >&2
        metadata_copy_failed=true
        continue
      fi
      metadata_count=$((metadata_count + 1))
    done <<<"$metadata_paths"

    actual_metadata_count=$(find "$STAGING_PATH/metadata" -maxdepth 1 -type f -name '*.json' -print | wc -l)
    if [[ "$actual_metadata_count" -ne "$metadata_count" ]]; then
      printf 'Expected %d staged metadata files, found %d\n' "$metadata_count" "$actual_metadata_count" >&2
      exit 1
    fi
    if [[ "$metadata_copy_failed" == true ]]; then
      printf '%s\n' 'One or more release metadata files failed to stage.' >&2
      exit 1
    fi
  fi
fi
