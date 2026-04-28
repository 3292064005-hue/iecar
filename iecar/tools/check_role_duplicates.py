#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import sys
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CAR1 = ROOT / 'stm32f407_platformio' / 'src' / 'car1'
CAR2 = ROOT / 'stm32f407_platformio' / 'src' / 'car2'
ROLE_BANNER = '/* ROLE_SCOPE:'

ALLOWED_ROLE_CHE_FILES = {'car_project_config.h', 'task2.c', 'task3.c'}


def assert_role_che_has_no_residual_adapters() -> list[str]:
    residual: list[str] = []
    for role_root in (CAR1, CAR2):
        che = role_root / 'SYSTEM' / 'CHE'
        if not che.exists():
            residual.append(f'{role_root.name}/SYSTEM/CHE missing')
            continue
        for path in sorted(che.iterdir()):
            if path.is_file() and path.name not in ALLOWED_ROLE_CHE_FILES:
                residual.append(f'{role_root.name}/SYSTEM/CHE/{path.name}')
    return residual

WRAPPER_ALLOWLIST = {
    'SYSTEM/CHE/DRIVER.c', 'SYSTEM/CHE/MOTOR_CTRL.c', 'SYSTEM/CHE/car_ctrl.c',
    'SYSTEM/CHE/car_diag.c', 'SYSTEM/CHE/car_init.c', 'SYSTEM/CHE/car_other.c',
    'SYSTEM/CHE/car_uart1.c', 'SYSTEM/CHE/car_uart2.c', 'SYSTEM/CHE/ctrl_num.c',
    'SYSTEM/CHE/k210it.c', 'SYSTEM/CHE/task1.c',
}


def read_text_lossy(path: Path) -> str:
    try:
        return path.read_text(encoding='utf-8')
    except UnicodeDecodeError:
        return path.read_text(errors='ignore')


def strip_role_banner(data: bytes) -> bytes:
    prefix = b'/* ROLE_SCOPE:'
    if data.startswith(prefix):
        end = data.find(b'*/')
        if end >= 0:
            after = end + 2
            if after < len(data) and data[after:after + 1] == b'\n':
                after += 1
            return data[after:]
    return data


def digest_normalized(path: Path) -> str:
    return hashlib.sha256(strip_role_banner(path.read_bytes())).hexdigest()


def is_common_wrapper(path: Path) -> bool:
    text = read_text_lossy(path)
    return 'COMMON/CHE' in text or 'COMMON/STRATEGY' in text or 'COMMON/PLATFORM' in text or 'COMMON/ROLE_SHARED' in text


def has_role_banner(path: Path) -> bool:
    return read_text_lossy(path).startswith(ROLE_BANNER)


def main() -> int:
    residual_role_che = assert_role_che_has_no_residual_adapters()
    if residual_role_che:
        print('Role duplicate policy error: residual role-local CHE adapters/headers remain:')
        for rel in residual_role_che[:40]:
            print(f'  - {rel}')
        return 1
    unmanaged: list[str] = []
    missing_banner: list[str] = []
    exact_duplicates: list[str] = []
    role_shared_includes: list[str] = []
    for p1 in CAR1.rglob('*'):
        if not p1.is_file():
            continue
        rel = p1.relative_to(CAR1).as_posix()
        p2 = CAR2 / rel
        if not p2.exists() or not p2.is_file():
            continue
        exact_same = hashlib.sha256(p1.read_bytes()).hexdigest() == hashlib.sha256(p2.read_bytes()).hexdigest()
        normalized_same = digest_normalized(p1) == digest_normalized(p2)
        text1 = read_text_lossy(p1)
        text2 = read_text_lossy(p2)
        if 'COMMON/ROLE_SHARED' in text1 or 'COMMON/ROLE_SHARED' in text2:
            role_shared_includes.append(rel)
        if exact_same:
            exact_duplicates.append(rel)
        if not normalized_same:
            continue
        suffix = Path(rel).suffix
        if not (has_role_banner(p1) and has_role_banner(p2)):
            missing_banner.append(rel)
            continue
        if suffix == '.c':
            if not (rel in WRAPPER_ALLOWLIST or (is_common_wrapper(p1) and is_common_wrapper(p2))):
                unmanaged.append(rel)
        elif suffix != '.h':
            unmanaged.append(rel)
    if exact_duplicates:
        print('Role duplicate policy error: bit-identical role files remain:')
        for rel in exact_duplicates[:40]:
            print(f'  - {rel}')
        return 1
    if missing_banner:
        print('Role duplicate policy error: normalized duplicate role files without ROLE_SCOPE banner:')
        for rel in missing_banner[:40]:
            print(f'  - {rel}')
        return 1
    if unmanaged:
        print('Role duplicate policy error: unmanaged duplicate role files remain:')
        for rel in unmanaged[:40]:
            print(f'  - {rel}')
        return 1
    if role_shared_includes:
        print('Role duplicate policy error: role-local C wrappers still include COMMON/ROLE_SHARED:')
        for rel in role_shared_includes[:40]:
            print(f'  - {rel}')
        return 1
    print('Role duplicate policy passed: 0 bit-identical role files remain.')
    print('Role duplicate policy passed: 0 unmanaged duplicate role files remain.')
    print('Role duplicate policy passed: 0 residual duplicate role implementations remain.')
    print('Role duplicate policy passed: 0 residual role-local CHE adapters/headers remain.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
