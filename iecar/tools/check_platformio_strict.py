#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import shutil
import subprocess
import sys
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'tools' / 'platformio_strict_manifest.json'
PLATFORMIO_INI = ROOT / 'stm32f407_platformio' / 'platformio.ini'


def parse_version(text: str) -> tuple[int, int, int]:
    match = re.search(r'(\d+)\.(\d+)\.(\d+)', text)
    if not match:
        raise ValueError(f'cannot parse PlatformIO version from: {text!r}')
    return tuple(int(match.group(i)) for i in range(1, 4))


def load_manifest() -> dict:
    return json.loads(MANIFEST.read_text(encoding='utf-8'))


def assert_envs_and_platform(manifest: dict, ini_text: str) -> None:
    for env in manifest['required_envs']:
        if f'[env:{env}]' not in ini_text:
            raise SystemExit(f'platformio.ini missing required environment [env:{env}]')
    constraint = manifest['ststm32_platform_constraint']
    platform_lines = [line.strip() for line in ini_text.splitlines() if line.strip().startswith('platform =')]
    firmware_lines = [line for line in platform_lines if 'ststm32' in line]
    if not firmware_lines:
        raise SystemExit('platformio.ini missing ststm32 platform line for firmware environments')
    for line in firmware_lines:
        if constraint not in line:
            raise SystemExit(f'ststm32 platform is not pinned to manifest constraint {constraint}: {line}')
    for token in manifest.get('required_build_src_filters', []):
        if token not in ini_text:
            raise SystemExit(f'platformio.ini missing required build_src_filter token: {token}')
    for token in manifest.get('forbidden_build_src_filters', []):
        if token in ini_text:
            raise SystemExit(f'platformio.ini still contains forbidden role-local build_src_filter token: {token}')
    for token in manifest.get('required_build_flags', []):
        if token not in ini_text:
            raise SystemExit(f'platformio.ini missing required build flag/include path token: {token}')


def assert_pio_version(manifest: dict) -> None:
    pio = shutil.which('pio')
    if pio is None:
        raise SystemExit("PlatformIO CLI 'pio' is required for strict mode.")
    proc = subprocess.run([pio, '--version'], check=False, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if proc.returncode != 0:
        raise SystemExit(proc.stdout.strip() or 'pio --version failed')
    got = parse_version(proc.stdout)
    min_v = parse_version(manifest['platformio_cli_min'])
    max_v = parse_version(manifest['platformio_cli_max_exclusive'])
    if got < min_v or got >= max_v:
        raise SystemExit(f'PlatformIO CLI version {got} outside supported range [{min_v}, {max_v})')


def main() -> int:
    manifest = load_manifest()
    if not PLATFORMIO_INI.exists():
        raise SystemExit('missing stm32f407_platformio/platformio.ini')
    ini_text = PLATFORMIO_INI.read_text(encoding='utf-8')
    assert_envs_and_platform(manifest, ini_text)
    assert_pio_version(manifest)
    print('PlatformIO strict preflight passed.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
