#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = [
    'tools/replay_validation.py',
    'tools/protocol_contract.py',
    'tools/generate_protocol_contract.py',
    'tools/protocol_golden_tests.py',
    'tools/check_wrapper_boundaries.py',
    'tools/check_role_duplicates.py',
    'tools/check_platformio_strict.py',
    'tools/check_python_syntax.py',
    'tools/check_static.py',
    'tools/run_quick_python_checks.py',
]


def main() -> int:
    for rel in FILES:
        path = ROOT / rel
        compile(path.read_text(encoding='utf-8'), rel, 'exec')
    print('Python source syntax check passed without writing __pycache__.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
