"""Focused staging-safety tests for compile_all.sh."""

from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
COMPILE_SCRIPT = REPO_ROOT / "compile_all.sh"
MANIFEST = REPO_ROOT / "keyboards/nuphy/release-manifest.json"


class CompileAllStagingSafetyTest(unittest.TestCase):
    """Verify compile_all.sh cannot delete an unexpected staging tree."""

    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.temp_root = Path(self.tempdir.name)
        self.fake_bin = self.temp_root / "bin"
        self.fake_bin.mkdir()
        self.marker = self.temp_root / "qmk-invoked"
        fake_qmk = self.fake_bin / "qmk"
        fake_qmk.write_text(
            "#!/bin/sh\n"
            "set -eu\n"
            "printf '%s\\n' invoked >> \"$FAKE_QMK_MARKER\"\n"
            "target=\n"
            "for argument in \"$@\"; do\n"
            "  case \"$argument\" in\n"
            "    TARGET=*) target=${argument#TARGET=} ;;\n"
            "  esac\n"
            "done\n"
            "test -n \"$target\"\n"
            "printf '%s\\n' fake > \"$FAKE_QMK_REPO_ROOT/$target.bin\"\n",
            encoding="utf-8",
        )
        fake_qmk.chmod(0o755)

    def tearDown(self) -> None:
        for artifact in REPO_ROOT.glob("air75v2-ansi-default-harness.bin"):
            artifact.unlink()
        self.tempdir.cleanup()

    def run_compile(self, staging_dir: Path) -> subprocess.CompletedProcess[str]:
        environment = os.environ.copy()
        environment.update(
            {
                "PATH": f"{self.fake_bin}{os.pathsep}{environment['PATH']}",
                "FAKE_QMK_MARKER": str(self.marker),
                "FAKE_QMK_REPO_ROOT": str(REPO_ROOT),
                "KEYBOARDS": "air75v2",
                "LAYOUTS": "ansi",
                "KEYMAPS": "default",
            }
        )
        return subprocess.run(
            [
                str(COMPILE_SCRIPT),
                "--profile",
                "release",
                "--manifest",
                str(MANIFEST),
                "--tag",
                "harness",
                "--staging-dir",
                str(staging_dir),
            ],
            cwd=REPO_ROOT,
            env=environment,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

    def assert_rejected_without_qmk(self, staging_dir: Path) -> None:
        result = self.run_compile(staging_dir)
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("Refusing", result.stdout)
        self.assertFalse(self.marker.exists(), result.stdout)

    def test_rejects_external_tree_with_unexpected_file(self) -> None:
        staging_dir = self.temp_root / "external-staging"
        staging_dir.mkdir()
        sentinel = staging_dir / "keep.txt"
        sentinel.write_text("do not delete", encoding="utf-8")

        self.assert_rejected_without_qmk(staging_dir)
        self.assertEqual(sentinel.read_text(encoding="utf-8"), "do not delete")

    def test_rejects_home_and_system_temp_directories(self) -> None:
        for staging_dir in (Path.home(), Path(tempfile.gettempdir())):
            with self.subTest(staging_dir=staging_dir):
                self.assert_rejected_without_qmk(staging_dir)

    def test_rejects_symlink_staging_path(self) -> None:
        staging_target = self.temp_root / "staging-target"
        staging_target.mkdir()
        sentinel = staging_target / "keep.txt"
        sentinel.write_text("do not delete", encoding="utf-8")
        staging_link = self.temp_root / "staging-link"
        staging_link.symlink_to(staging_target, target_is_directory=True)

        self.assert_rejected_without_qmk(staging_link)
        self.assertTrue(sentinel.exists())
        self.assertTrue(staging_link.is_symlink())

    def test_allows_nonexistent_external_staging_path(self) -> None:
        staging_dir = self.temp_root / "new" / "release-output"

        result = self.run_compile(staging_dir)

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.marker.read_text(encoding="utf-8").splitlines(), ["invoked"])
        self.assertTrue(
            (staging_dir / "firmware/air75v2-ansi-default-harness.bin").is_file()
        )
        self.assertEqual(len(list((staging_dir / "metadata").glob("*.json"))), 5)

    def test_cleans_only_known_files_in_existing_release_staging(self) -> None:
        staging_dir = self.temp_root / "dedicated-release"
        firmware_dir = staging_dir / "firmware"
        metadata_dir = staging_dir / "metadata"
        firmware_dir.mkdir(parents=True)
        metadata_dir.mkdir()
        old_firmware = firmware_dir / "old.bin"
        old_metadata = metadata_dir / "old.json"
        old_firmware.write_text("old", encoding="utf-8")
        old_metadata.write_text("{}", encoding="utf-8")

        result = self.run_compile(staging_dir)

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertFalse(old_firmware.exists())
        self.assertFalse(old_metadata.exists())
        self.assertTrue(
            (staging_dir / "firmware/air75v2-ansi-default-harness.bin").is_file()
        )
        self.assertEqual(len(list(metadata_dir.glob("*.json"))), 5)

    def test_preserves_existing_repository_protected_path_rejections(self) -> None:
        for staging_dir in (REPO_ROOT / ".git", REPO_ROOT / "keyboards/nuphy"):
            with self.subTest(staging_dir=staging_dir):
                self.assert_rejected_without_qmk(staging_dir)


if __name__ == "__main__":
    unittest.main()
