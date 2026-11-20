"""Builds small temporary repositories for the tests."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

CHECKS_FOLDER = Path(__file__).resolve().parent.parent
if str(CHECKS_FOLDER) not in sys.path:
    sys.path.insert(0, str(CHECKS_FOLDER))


class TempRepositoryTest(unittest.TestCase):
    """A test that works inside an empty temporary repository."""

    def setUp(self) -> None:
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.root = Path(folder.name).resolve()
        self.write("AGENTS.md", "# Rules\n")

    def write(self, path: str, content: str) -> Path:
        file = self.root / path
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text(content, encoding="utf-8", newline="\n")
        return file

    def messages(self, findings: list) -> str:
        return "\n".join(str(finding) for finding in findings)

    def assert_refused(self, findings: list, expected: str) -> None:
        self.assertTrue(findings, "the violation was not refused")
        self.assertIn(expected, self.messages(findings))
