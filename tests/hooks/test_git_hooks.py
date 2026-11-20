"""Tests of the Git hooks in .githooks. Every test works in a temporary repository."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ZEROS = "0" * 40
SHA = "1" * 40


def find_shell() -> str:
    found = shutil.which("sh")
    if found and "system32" not in found.lower():
        return found
    git = shutil.which("git")
    if git:
        for candidate in (Path(git).parent.parent / "bin" / "sh.exe", Path(git).parent.parent / "usr" / "bin" / "sh.exe"):
            if candidate.exists():
                return str(candidate)
    raise RuntimeError("sh was not found")


def clean_environment() -> dict[str, str]:
    """Without the variables of a running Git hook, which would point Git to the real repository."""
    return {name: value for name, value in os.environ.items() if not name.upper().startswith("GIT_")}


class GitHookTest(unittest.TestCase):
    def setUp(self) -> None:
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.root = Path(folder.name).resolve()
        self.git("init", "-q")
        shutil.copytree(ROOT / ".githooks", self.root / ".githooks")

    def git(self, *arguments: str) -> None:
        subprocess.run(["git", *arguments], cwd=self.root, env=clean_environment(), check=True, capture_output=True)

    def hook(self, name: str, *arguments: str, stdin: str = "") -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [find_shell(), f".githooks/{name}", *arguments], cwd=self.root, env=clean_environment(), input=stdin,
            capture_output=True, text=True, encoding="utf-8", errors="replace", check=False,
        )


class CommitMessageTests(GitHookTest):
    def check(self, message: str) -> int:
        (self.root / "message.txt").write_text(message, encoding="utf-8")
        return self.hook("commit-msg", "message.txt").returncode

    def test_valid_messages_are_accepted(self) -> None:
        for message in ("feat(0000): T03 add the tick clock\n", "docs(prep): add documents\n", "ci(ci): run tests\n"):
            with self.subTest(message=message):
                self.assertEqual(self.check(message), 0)

    def test_invalid_messages_are_refused(self) -> None:
        for message in ("add tile picking\n", "feat: no scope\n", "Feat(0000): upper case\n", "wip(0000): unknown type\n",
                        "feat(0000):missing space\n"):
            with self.subTest(message=message):
                self.assertEqual(self.check(message), 1)


class PrePushTests(GitHookTest):
    def push(self, remote_ref: str, remote_sha: str) -> int:
        return self.hook("pre-push", stdin=f"refs/heads/x {SHA} {remote_ref} {remote_sha}\n").returncode

    def test_pushing_a_slice_branch_is_accepted(self) -> None:
        self.assertEqual(self.push("refs/heads/fatia/0000-fundacao", SHA), 0)

    def test_updating_main_is_refused(self) -> None:
        self.assertEqual(self.push("refs/heads/main", SHA), 1)

    def test_updating_master_is_refused(self) -> None:
        self.assertEqual(self.push("refs/heads/master", SHA), 1)

    def test_creating_main_for_the_first_time_is_accepted(self) -> None:
        self.assertEqual(self.push("refs/heads/main", ZEROS), 0)


class SecretScanTests(GitHookTest):
    def stage(self, name: str, content: str) -> int:
        file = self.root / name
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text(content, encoding="utf-8")
        self.git("add", "-f", name)
        return self.hook("scan-secrets.sh").returncode

    def test_ordinary_files_are_accepted(self) -> None:
        self.assertEqual(self.stage("src/a.cpp", "int main() {}\n"), 0)

    def test_secret_files_are_refused(self) -> None:
        for name in (".env", "config/.env.local", "server.pem", "private.key", "id_ed25519", "secrets.json"):
            with self.subTest(name=name):
                self.assertEqual(self.stage(name, "value\n"), 1)
                self.git("rm", "-q", "--cached", name)

    def test_private_keys_in_a_file_are_refused(self) -> None:
        self.assertEqual(self.stage("a.txt", "-----BEGIN OPENSSH " + "PRIVATE KEY-----\n"), 1)

    def test_access_tokens_in_a_file_are_refused(self) -> None:
        self.assertEqual(self.stage("a.txt", "token = ghp_" + "a" * 36 + "\n"), 1)


if __name__ == "__main__":
    unittest.main()
