#!/usr/bin/env python3
"""Runs the repository checks. Usage: py scripts/checks/run.py all

This file is a protected "trava" (see docs/WORKFLOW.md).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from nexum_checks.cli import main  # noqa: E402

sys.exit(main(sys.argv[1:]))
