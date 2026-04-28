#!/usr/bin/env python3
from __future__ import annotations

import runpy
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / 'tools'
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))


def run_script(rel: str, argv: list[str] | None = None) -> None:
    old_argv = sys.argv[:]
    sys.argv = [str(ROOT / rel)] + (argv or [])
    try:
        try:
            runpy.run_path(str(ROOT / rel), run_name='__main__')
        except SystemExit as exc:
            code = exc.code
            if code not in (None, 0):
                raise
    finally:
        sys.argv = old_argv


def assert_ascii_paths() -> None:
    encoded_marker = chr(35) + 'U'
    bad = [str(p) for p in ROOT.rglob('*') if encoded_marker in str(p)]
    if bad:
        raise SystemExit('encoded Unicode path names remain: ' + ', '.join(bad[:10]))
    print('ASCII path-name guard passed.')


def main() -> int:
    run_script('tools/generate_protocol_contract.py', ['--check'])
    run_script('tools/protocol_golden_tests.py')
    run_script('tools/check_wrapper_boundaries.py')
    run_script('tools/check_role_duplicates.py')
    assert_ascii_paths()
    run_script('tools/replay_validation.py')
    return 0


if __name__ == '__main__':
    sys.exit(main())
