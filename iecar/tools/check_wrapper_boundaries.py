#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "stm32f407_platformio" / "src"
WRAPPER_DIRS = [SRC / "car1" / "SYSTEM" / "CHE", SRC / "car2" / "SYSTEM" / "CHE"]
ALLOWED_SHARED_PREFIX = "../../../COMMON/CHE/"
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"\s*$')
COMMENT_RE = re.compile(r'/\*.*?\*/|//.*?$', re.S | re.M)


def _strip_comments(text: str) -> str:
    return COMMENT_RE.sub('', text)


def check_wrapper(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="ignore")
    if ALLOWED_SHARED_PREFIX not in text:
        return
    stripped = _strip_comments(text)
    include_targets: list[str] = []
    for lineno, line in enumerate(stripped.splitlines(), start=1):
        if not line.strip():
            continue
        match = INCLUDE_RE.match(line)
        if not match:
            raise AssertionError(f"wrapper contains non-include logic: {path}:{lineno}: {line.strip()}")
        include_targets.append(match.group(1))
    shared_includes = [target for target in include_targets if target.startswith(ALLOWED_SHARED_PREFIX)]
    if len(shared_includes) != 1:
        raise AssertionError(f"wrapper must include exactly one shared implementation: {path}")
    if not shared_includes[0].endswith('_shared.c'):
        raise AssertionError(f"wrapper shared include must target *_shared.c: {path}: {shared_includes[0]}")
    shared_pos = include_targets.index(shared_includes[0])
    if shared_pos != len(include_targets) - 1:
        raise AssertionError(f"shared implementation include must be the last include in thin wrapper: {path}")
    for target in include_targets[:shared_pos]:
        if target.startswith(ALLOWED_SHARED_PREFIX):
            continue
        if target.endswith('.c'):
            raise AssertionError(f"wrapper may not include non-shared .c file: {path}: {target}")


def main() -> None:
    for wrapper_dir in WRAPPER_DIRS:
        for path in wrapper_dir.glob("*.c"):
            check_wrapper(path)
    print("Wrapper boundary check passed.")


if __name__ == "__main__":
    import sys
    main()
    sys.exit(0)
