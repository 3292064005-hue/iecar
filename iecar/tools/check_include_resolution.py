#!/usr/bin/env python3
from __future__ import annotations

import fnmatch
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIO = ROOT / 'stm32f407_platformio'
SRC = PIO / 'src'

COMMON_SRC_FILTERS = [
    'COMMON/CHE/*_shared.c',
    'COMMON/ROLE_SHARED/USER/*.c',
    'COMMON/ROLE_SHARED/SYSTEM/*.c',
    'COMMON/ROLE_SHARED/SYSTEM/ANO_V7/*.c',
    'COMMON/ROLE_SHARED/SYSTEM/delay/*.c',
    'COMMON/ROLE_SHARED/SYSTEM/oled/*.c',
    'COMMON/ROLE_SHARED/SYSTEM/pid/*.c',
    'COMMON/ROLE_SHARED/SYSTEM/sys/*.c',
    'COMMON/ROLE_SHARED/MOTOR_shuanghuan/*.c',
    'COMMON/PLATFORM/syscalls_gcc.c',
]

ROLE_SRC_FILTERS = {
    'car1': COMMON_SRC_FILTERS + ['car1/SYSTEM/*.c', 'car1/SYSTEM/CHE/task2.c', 'car1/SYSTEM/CHE/task3.c'],
    'car2': COMMON_SRC_FILTERS + ['car2/SYSTEM/*.c', 'car2/SYSTEM/CHE/task2.c', 'car2/SYSTEM/CHE/task3.c'],
}

COMMON_INCLUDE_DIRS = [
    'src/COMMON/CHE',
    'src/COMMON/STRATEGY',
    'src/COMMON/PLATFORM',
    'src/COMMON/ROLE_SHARED/SYSTEM/STRATEGY/common',
    'src/COMMON/ROLE_SHARED/MOTOR_shuanghuan',
    'src/COMMON/ROLE_SHARED/SYSTEM/sys',
    'src/COMMON/ROLE_SHARED/SYSTEM/pid',
    'src/COMMON/ROLE_SHARED/SYSTEM/oled',
    'src/COMMON/ROLE_SHARED/SYSTEM/delay',
    'src/COMMON/ROLE_SHARED/SYSTEM/ANO_V7',
    'src/COMMON/ROLE_SHARED/SYSTEM/PLATFORM',
    'src/COMMON/ROLE_SHARED/SYSTEM',
    'src/COMMON/ROLE_SHARED/USER',
]

ROLE_INCLUDE_DIRS = {
    'car1': COMMON_INCLUDE_DIRS + [
        'src/car1/SYSTEM',
        'src/car1/SYSTEM/CHE',
        'src/car1/SYSTEM/PLATFORM',
        'src/car1/SYSTEM/STRATEGY/common',
        'src/car1/SYSTEM/STRATEGY/car1',
    ],
    'car2': COMMON_INCLUDE_DIRS + [
        'src/car2/SYSTEM',
        'src/car2/SYSTEM/CHE',
        'src/car2/SYSTEM/PLATFORM',
        'src/car2/SYSTEM/STRATEGY/common',
        'src/car2/SYSTEM/STRATEGY/car2',
    ],
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
SYSTEM_OR_CONDITIONAL_HEADERS = {
    'stm32f4xx.h', 'stdio.h', 'stdlib.h', 'string.h', 'stdint.h', 'math.h',
    'includes.h',
}


def _all_sources_for(role: str) -> list[Path]:
    rel_sources = [p.relative_to(SRC).as_posix() for p in SRC.rglob('*.c')]
    out: list[Path] = []
    for rel in rel_sources:
        if any(fnmatch.fnmatch(rel, pattern) for pattern in ROLE_SRC_FILTERS[role]):
            out.append(SRC / rel)
    return out


def _resolve_include(include_name: str, current_dir: Path, include_dirs: list[Path]) -> Path | None:
    candidates = [current_dir / include_name]
    candidates.extend(path / include_name for path in include_dirs)
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return None


def _scan_role(role: str) -> list[tuple[str, str]]:
    include_dirs = [PIO / path for path in ROLE_INCLUDE_DIRS[role]]
    unresolved: list[tuple[str, str]] = []
    visited: set[Path] = set()

    def scan(path: Path) -> None:
        resolved_path = path.resolve()
        if resolved_path in visited:
            return
        visited.add(resolved_path)
        text = path.read_text(encoding='utf-8', errors='ignore')
        for line in text.splitlines():
            match = INCLUDE_RE.match(line)
            if not match:
                continue
            include_name = match.group(1)
            if include_name in SYSTEM_OR_CONDITIONAL_HEADERS:
                continue
            if include_name.startswith(('stm32', 'core_', 'misc', 'system_')):
                continue
            target = _resolve_include(include_name, path.parent, include_dirs)
            if target is None:
                unresolved.append((path.relative_to(ROOT).as_posix(), include_name))
            else:
                scan(target)

    for source in _all_sources_for(role):
        scan(source)
    return unresolved


def _assert_platformio_has_include_dirs() -> None:
    ini = (PIO / 'platformio.ini').read_text(encoding='utf-8')
    required = [
        '-Isrc/COMMON/PLATFORM',
        '-Isrc/COMMON/ROLE_SHARED/SYSTEM/STRATEGY/common',
    ]
    for token in required:
        if token not in ini:
            raise SystemExit(f'platformio.ini missing include path required for shared-source build: {token}')


def main() -> int:
    _assert_platformio_has_include_dirs()
    failures: list[tuple[str, str, str]] = []
    for role in ('car1', 'car2'):
        for source, include_name in _scan_role(role):
            failures.append((role, source, include_name))
    if failures:
        print('Include resolution check failed: unresolved local includes remain.')
        for role, source, include_name in failures[:80]:
            print(f'  - {role}: {source} -> {include_name}')
        return 1
    print('Include resolution check passed for car1/car2 shared-source build graph.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
