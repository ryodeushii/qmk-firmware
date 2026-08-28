#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any, Mapping, Sequence, Union


PathLike = Union[str, Path]
BASE_VERSION = "0.32.7"
BASE_COMMIT = "ed80e21858"
EXPECTED_RELEASE_COUNT = 21
EXPECTED_PERSONAL_COUNT = 5
EXPECTED_METADATA_COUNT = 5
ALLOWED_MODES = frozenset({"USB_MODE", "THREE_MODE"})
ALLOWED_OUTPUT_PREFIXES = frozenset({"", "wired-", "threemode-"})
RELEASE_KEYMAPS = ("default", "via", "srgb")
PERSONAL_KEYMAPS = ("ryodeushii",)
SUPPORTED_LAYOUTS = frozenset({
    ("air75v2", "ansi"),
    ("gem80", "ansi"),
    ("gem80", "iso"),
    ("halo75v2", "ansi"),
    ("halo96v2", "ansi"),
})
EXPECTED_METADATA = (
    "keyboards/nuphy/air75v2/ansi/keymaps/default/NuPhy Air75 V2 via3.json",
    "keyboards/nuphy/gem80/ansi/keymaps/default/NuPhy Gem80 via3.json",
    "keyboards/nuphy/gem80/iso/keymaps/default/NuPhy Gem80 ISO via3.json",
    "keyboards/nuphy/halo75v2/ansi/keymaps/default/NuPhy Halo75v2 via3.json",
    "keyboards/nuphy/halo96v2/ansi/keymaps/default/NuPhy Halo96v2 via3.json",
)


class ManifestError(ValueError):
    """Raised when a NuPhy release manifest is invalid."""



def load_manifest(path: PathLike) -> dict:
    manifest_path = Path(path)
    try:
        with manifest_path.open(encoding="utf-8") as manifest_file:
            manifest = json.load(manifest_file)
    except OSError as error:
        raise ManifestError(f"cannot read manifest {manifest_path}: {error}") from error
    except json.JSONDecodeError as error:
        raise ManifestError(f"invalid JSON in manifest {manifest_path}: {error}") from error

    if not isinstance(manifest, dict):
        raise ManifestError("manifest must contain a JSON object")
    return manifest



def _is_sequence(value: Any) -> bool:
    return isinstance(value, Sequence) and not isinstance(value, (str, bytes, bytearray))



def _safe_relative_path(value: Any) -> bool:
    if not isinstance(value, str) or not value:
        return False
    path = Path(value)
    return not path.is_absolute() and ".." not in path.parts and "." not in path.parts



def _safe_component(value: Any) -> bool:
    if not _safe_relative_path(value):
        return False
    return len(Path(value).parts) == 1



def _target_source(repo_root: Path, target: Mapping[str, Any]) -> Path:
    return (repo_root / "keyboards" / "nuphy" / target["keyboard"] / target["layout"] /
            "keymaps" / target["keymap"] / "keymap.c")



def _target_output_identity(target: Mapping[str, Any]) -> str:
    return "{output_prefix}{keyboard}-{layout}-{keymap}".format(**target)



def _expected_targets(profile: str) -> set:
    if profile == "release":
        groups = (
            ("air75v2", "ansi", "THREE_MODE", ""),
            ("gem80", "ansi", "USB_MODE", "wired-"),
            ("gem80", "ansi", "THREE_MODE", "threemode-"),
            ("gem80", "iso", "USB_MODE", "wired-"),
            ("gem80", "iso", "THREE_MODE", "threemode-"),
            ("halo75v2", "ansi", "THREE_MODE", ""),
            ("halo96v2", "ansi", "THREE_MODE", ""),
        )
        keymaps = RELEASE_KEYMAPS
    else:
        groups = (
            ("air75v2", "ansi", "THREE_MODE", ""),
            ("gem80", "ansi", "USB_MODE", "wired-"),
            ("gem80", "ansi", "THREE_MODE", "threemode-"),
            ("halo75v2", "ansi", "THREE_MODE", ""),
            ("halo96v2", "ansi", "THREE_MODE", ""),
        )
        keymaps = PERSONAL_KEYMAPS

    return {
        (keyboard, layout, keymap, mode, output_prefix)
        for keyboard, layout, mode, output_prefix in groups
        for keymap in keymaps
    }



def _validate_targets(
    profile: str,
    records: Any,
    expected_count: int,
    allowed_keymaps: Sequence[str],
    repo_root: Path,
    errors: list,
    target_ids: set,
    output_identities: set,
) -> None:
    if not _is_sequence(records):
        errors.append(f"{profile}_targets must be a JSON array")
        return

    if len(records) != expected_count:
        errors.append(f"expected {expected_count} {profile} targets, found {len(records)}")

    actual_targets = set()
    for index, record in enumerate(records):
        location = f"{profile}_targets[{index}]"
        if not isinstance(record, Mapping):
            errors.append(f"{location} must be a JSON object")
            continue

        required_fields = ("id", "keyboard", "layout", "keymap", "work_mode", "output_prefix")
        missing = [field for field in required_fields if field not in record]
        if missing:
            errors.append(f"{location} missing fields: {', '.join(missing)}")
            continue

        values = {field: record[field] for field in required_fields}
        if not isinstance(values["id"], str) or not _safe_component(values["id"]):
            errors.append(f"{location}.id must be a non-empty safe path component")
        elif values["id"] in target_ids:
            errors.append(f"duplicate target id: {values['id']}")
        else:
            target_ids.add(values["id"])

        for field in ("keyboard", "layout", "keymap"):
            if not _safe_component(values[field]):
                errors.append(f"{location}.{field} must be a safe path component")

        if (values["keyboard"], values["layout"]) not in SUPPORTED_LAYOUTS:
            errors.append(
                f"{location} has unsupported board/layout: "
                f"{values['keyboard']}/{values['layout']}"
            )

        if values["keymap"] not in allowed_keymaps:
            if profile == "release" and values["keymap"] == "ryodeushii":
                errors.append(f"{location} uses personal release keymap: ryodeushii")
            else:
                errors.append(
                    f"{location} has invalid {profile} keymap: {values['keymap']}"
                )

        if values["work_mode"] not in ALLOWED_MODES:
            errors.append(f"{location} has unknown WORK_MODE: {values['work_mode']}")

        if values["output_prefix"] not in ALLOWED_OUTPUT_PREFIXES:
            errors.append(
                f"{location} has invalid output prefix: {values['output_prefix']}"
            )

        if all(isinstance(values[field], str) for field in required_fields):
            output_identity = _target_output_identity(values)
            if output_identity in output_identities:
                errors.append(f"duplicate output identity: {output_identity}")
            else:
                output_identities.add(output_identity)

            actual_targets.add((
                values["keyboard"],
                values["layout"],
                values["keymap"],
                values["work_mode"],
                values["output_prefix"],
            ))

            if not all(_safe_component(values[field]) for field in ("keyboard", "layout", "keymap")):
                continue
            source = _target_source(repo_root, values)
            if not source.is_file():
                errors.append(f"{location} missing keymap source: {source}")

    expected_targets = _expected_targets(profile)
    if actual_targets != expected_targets:
        missing = sorted(expected_targets - actual_targets)
        extra = sorted(actual_targets - expected_targets)
        if missing:
            errors.append(f"missing expected {profile} targets: {missing}")
        if extra:
            errors.append(f"unexpected {profile} targets: {extra}")



def validate_manifest(manifest: Mapping[str, object], repo_root: PathLike = ".") -> None:
    if not isinstance(manifest, Mapping):
        raise ManifestError("manifest must contain a JSON object")

    root = Path(repo_root)
    errors = []

    if manifest.get("schema_version") != 1:
        errors.append("schema_version must be 1")

    qmk_base = manifest.get("qmk_base")
    if not isinstance(qmk_base, Mapping):
        errors.append("qmk_base must be a JSON object")
    else:
        if qmk_base.get("version") != BASE_VERSION:
            errors.append(f"QMK base version must be {BASE_VERSION}")
        if qmk_base.get("commit") != BASE_COMMIT:
            errors.append(f"QMK base commit must be {BASE_COMMIT}")

    target_ids = set()
    output_identities = set()
    _validate_targets(
        "release",
        manifest.get("release_targets"),
        EXPECTED_RELEASE_COUNT,
        RELEASE_KEYMAPS,
        root,
        errors,
        target_ids,
        output_identities,
    )
    _validate_targets(
        "personal",
        manifest.get("personal_targets"),
        EXPECTED_PERSONAL_COUNT,
        PERSONAL_KEYMAPS,
        root,
        errors,
        target_ids,
        output_identities,
    )

    metadata = manifest.get("release_metadata")
    if not _is_sequence(metadata):
        errors.append("release_metadata must be a JSON array")
    else:
        if len(metadata) != EXPECTED_METADATA_COUNT:
            errors.append(
                f"expected {EXPECTED_METADATA_COUNT} release metadata files, found {len(metadata)}"
            )
        metadata_values = []
        for index, metadata_path in enumerate(metadata):
            location = f"release_metadata[{index}]"
            if not _safe_relative_path(metadata_path):
                errors.append(f"{location} must be a safe relative path")
                continue
            metadata_values.append(metadata_path)
            source = root / metadata_path
            if not source.is_file():
                errors.append(f"missing metadata file: {source}")
        if tuple(metadata_values) != EXPECTED_METADATA:
            errors.append("release_metadata paths do not match the supported VIA3 files")

    if errors:
        raise ManifestError("\n".join(errors))



def resolve_targets(manifest: Mapping[str, object], profile: str) -> list[dict]:
    if profile not in ("release", "personal"):
        raise ManifestError(f"unknown profile: {profile}")
    field = f"{profile}_targets"
    records = manifest.get(field)
    if not _is_sequence(records):
        raise ManifestError(f"{field} must be a JSON array")
    return [dict(record) for record in records]



def binary_name(target: Mapping[str, str], tag: str) -> str:
    required = ("output_prefix", "keyboard", "layout", "keymap")
    missing = [field for field in required if field not in target]
    if missing:
        raise ManifestError(f"target is missing fields: {', '.join(missing)}")
    if not isinstance(tag, str) or not tag:
        raise ManifestError("tag must be a non-empty string")
    return (
        f"{target['output_prefix']}{target['keyboard']}-{target['layout']}-"
        f"{target['keymap']}-{tag.replace('/', '-')}.bin"
    )



def _base_info(manifest: Mapping[str, object], repo_root: Path) -> dict:
    validate_manifest(manifest, repo_root)
    qmk_base = manifest["qmk_base"]
    declared_commit = qmk_base["commit"]
    try:
        resolved_commit = subprocess.run(
            ["git", "rev-parse", "--verify", f"{declared_commit}^{{commit}}"],
            cwd=repo_root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ManifestError(
            f"cannot resolve QMK base commit {declared_commit}: {error}"
        ) from error

    ancestry = subprocess.run(
        ["git", "merge-base", "--is-ancestor", resolved_commit, "HEAD"],
        cwd=repo_root,
        capture_output=True,
        text=True,
    )
    if ancestry.returncode != 0:
        raise ManifestError(
            f"QMK base commit {declared_commit} is not an ancestor of HEAD"
        )

    return {
        "version": qmk_base["version"],
        "commit": declared_commit,
        "resolved_commit": resolved_commit,
        "base_is_ancestor": True,
    }



def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Validate and query NuPhy release manifests")
    subparsers = parser.add_subparsers(dest="command", required=True)

    def add_manifest_options(command_parser: argparse.ArgumentParser) -> None:
        command_parser.add_argument("--manifest", required=True, type=Path)
        command_parser.add_argument(
            "--repo-root",
            "--root",
            dest="repo_root",
            type=Path,
            default=Path("."),
        )

    validate_parser = subparsers.add_parser("validate")
    add_manifest_options(validate_parser)

    targets_parser = subparsers.add_parser("targets")
    add_manifest_options(targets_parser)
    targets_parser.add_argument("--profile", required=True, choices=("release", "personal"))
    targets_parser.add_argument("--tag")

    metadata_parser = subparsers.add_parser("metadata")
    add_manifest_options(metadata_parser)

    base_parser = subparsers.add_parser("base")
    add_manifest_options(base_parser)

    return parser



def main(argv: Union[Sequence[str], None] = None) -> int:
    args = _parser().parse_args(argv)
    try:
        manifest = load_manifest(args.manifest)
        if args.command == "validate":
            validate_manifest(manifest, args.repo_root)
            print(json.dumps({
                "valid": True,
                "release_targets": EXPECTED_RELEASE_COUNT,
                "personal_targets": EXPECTED_PERSONAL_COUNT,
                "release_metadata": EXPECTED_METADATA_COUNT,
            }, separators=(",", ":")))
        elif args.command == "targets":
            validate_manifest(manifest, args.repo_root)
            targets = resolve_targets(manifest, args.profile)
            if args.tag is not None:
                for target in targets:
                    target["binary"] = binary_name(target, args.tag)
            print(json.dumps(targets, separators=(",", ":")))
        elif args.command == "metadata":
            validate_manifest(manifest, args.repo_root)
            print(json.dumps(manifest["release_metadata"], separators=(",", ":")))
        elif args.command == "base":
            print(json.dumps(_base_info(manifest, args.repo_root), separators=(",", ":")))
    except ManifestError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
