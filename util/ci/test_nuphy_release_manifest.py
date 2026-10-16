#!/usr/bin/env python3

import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from nuphy_release_manifest import ManifestError
from nuphy_release_manifest import binary_name
from nuphy_release_manifest import load_manifest
from nuphy_release_manifest import resolve_targets
from nuphy_release_manifest import validate_manifest


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = REPOSITORY_ROOT / "keyboards/nuphy/release-manifest.json"


class ReleaseManifestTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = load_manifest(MANIFEST_PATH)

    def test_manifest_has_expected_profiles_and_metadata(self):
        self.assertEqual(len(resolve_targets(self.manifest, "release")), 21)
        self.assertEqual(len(resolve_targets(self.manifest, "personal")), 5)
        self.assertEqual(len(self.manifest["release_metadata"]), 5)
        validate_manifest(self.manifest, REPOSITORY_ROOT)

    def test_metadata_lives_with_via_keymap(self):
        self.assertEqual(
            self.manifest["release_metadata"],
            [
                "keyboards/nuphy/air75v2/ansi/keymaps/via/NuPhy Air75 V2 via3.json",
                "keyboards/nuphy/gem80/ansi/keymaps/via/NuPhy Gem80 via3.json",
                "keyboards/nuphy/gem80/iso/keymaps/via/NuPhy Gem80 ISO via3.json",
                "keyboards/nuphy/halo75v2/ansi/keymaps/via/NuPhy Halo75v2 via3.json",
                "keyboards/nuphy/halo96v2/ansi/keymaps/via/NuPhy Halo96v2 via3.json",
            ],
        )

    def test_release_target_order_is_preserved(self):
        targets = resolve_targets(self.manifest, "release")

        self.assertEqual(
            (targets[0]["keyboard"], targets[0]["layout"], targets[0]["keymap"]),
            ("air75v2", "ansi", "default"),
        )
        self.assertEqual(
            (targets[-1]["keyboard"], targets[-1]["layout"], targets[-1]["keymap"]),
            ("halo96v2", "ansi", "srgb"),
        )

    def test_resolve_targets_rejects_unknown_profile(self):
        with self.assertRaisesRegex(ManifestError, "profile"):
            resolve_targets(self.manifest, "debug")

    def test_resolve_targets_rejects_non_object_record(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["release_targets"][0] = []

        with self.assertRaisesRegex(ManifestError, "JSON objects"):
            resolve_targets(manifest, "release")

    def test_binary_name_sanitizes_tag_and_keeps_prefix(self):
        target = {
            "output_prefix": "wired-",
            "keyboard": "gem80",
            "layout": "ansi",
            "keymap": "default",
        }

        self.assertEqual(
            binary_name(target, "r/1.2.3"),
            "wired-gem80-ansi-default-r-1.2.3.bin",
        )

    def test_duplicate_target_ids_are_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["personal_targets"][0]["id"] = manifest["release_targets"][0]["id"]

        with self.assertRaisesRegex(ManifestError, "duplicate target id"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_duplicate_output_identities_are_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        duplicate = copy.deepcopy(manifest["release_targets"][0])
        duplicate["id"] = "duplicate-output-identity"
        manifest["personal_targets"][0] = duplicate

        with self.assertRaisesRegex(ManifestError, "duplicate output"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_unknown_work_mode_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["release_targets"][0]["work_mode"] = "UNKNOWN_MODE"

        with self.assertRaisesRegex(ManifestError, "WORK_MODE"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_malformed_target_field_types_raise_manifest_error(self):
        fields = ("id", "keyboard", "layout", "keymap", "work_mode", "output_prefix")
        for field in fields:
            for invalid in ([], {}):
                with self.subTest(field=field, invalid=type(invalid).__name__):
                    manifest = copy.deepcopy(self.manifest)
                    manifest["release_targets"][0][field] = invalid

                    with self.assertRaisesRegex(ManifestError, f"{field} must be a string"):
                        validate_manifest(manifest, REPOSITORY_ROOT)

    def test_unsupported_board_layout_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["release_targets"][0]["keyboard"] = "unsupported"

        with self.assertRaisesRegex(ManifestError, "board/layout"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_missing_keymap_source_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["release_targets"][0]["keymap"] = "missing"

        with self.assertRaisesRegex(ManifestError, "keymap source"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_missing_metadata_file_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["release_metadata"][0] = "keyboards/nuphy/missing.json"

        with self.assertRaisesRegex(ManifestError, "metadata"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_unsafe_paths_are_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["release_metadata"][0] = "/tmp/metadata.json"

        with self.assertRaisesRegex(ManifestError, "safe relative path"):
            validate_manifest(manifest, REPOSITORY_ROOT)

        manifest = copy.deepcopy(self.manifest)
        manifest["personal_targets"][0]["keymap"] = "../default"
        with self.assertRaisesRegex(ManifestError, "safe path"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_personal_keymap_is_rejected_from_release_profile(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["release_targets"][0]["keymap"] = "ryodeushii"

        with self.assertRaisesRegex(ManifestError, "release keymap"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_profile_count_mismatch_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["release_targets"].pop()

        with self.assertRaisesRegex(ManifestError, "21 release"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_invalid_qmk_base_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["qmk_base"]["version"] = "0.32.8"

        with self.assertRaisesRegex(ManifestError, "QMK base version"):
            validate_manifest(manifest, REPOSITORY_ROOT)

        manifest = copy.deepcopy(self.manifest)
        manifest["qmk_base"]["commit"] = "0000000000"
        with self.assertRaisesRegex(ManifestError, "QMK base commit"):
            validate_manifest(manifest, REPOSITORY_ROOT)

    def test_malformed_json_raises_manifest_error(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "broken.json"
            path.write_text("{broken", encoding="utf-8")

            with self.assertRaises(ManifestError):
                load_manifest(path)

    def test_invalid_utf8_raises_manifest_error(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid-utf8.json"
            path.write_bytes(b"\x80")

            with self.assertRaisesRegex(ManifestError, "UTF-8"):
                load_manifest(path)

    def test_manifest_loader_rejects_non_object_json(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "list.json"
            path.write_text(json.dumps([]), encoding="utf-8")

            with self.assertRaisesRegex(ManifestError, "JSON object"):
                load_manifest(path)


if __name__ == "__main__":
    unittest.main()
