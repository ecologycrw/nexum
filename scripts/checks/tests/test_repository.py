import unittest
from pathlib import Path

import support  # noqa: F401

from nexum_checks.cli import CHECKS

ROOT = Path(__file__).resolve().parents[3]


class RepositoryTests(unittest.TestCase):
    def test_this_repository_passes_every_check(self) -> None:
        findings = [str(finding) for name, check in CHECKS.items() for finding in check(ROOT)]
        self.assertEqual(findings, [])


if __name__ == "__main__":
    unittest.main()
