#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PYTHON_BIN = os.environ.get('PYTHON_BIN') or os.environ.get('PYTHON') or sys.executable
PYTHON_FLAGS = os.environ.get('PYTHON_FLAGS', '-S')


def shell_quote(value: str) -> str:
    return "'" + value.replace("'", "'\\''") + "'"


def run_shell(cmd: str, cwd: Path | None = None) -> None:
    proc = subprocess.run(cmd, shell=True, cwd=str(cwd) if cwd is not None else None)
    if proc.returncode != 0:
        raise SystemExit(proc.returncode)


def run_py(script: str, args: list[str] | None = None) -> None:
    cmd = [PYTHON_BIN, '-S', str(ROOT / script)] + list(args or [])
    proc = subprocess.run(cmd, cwd=str(ROOT))
    if proc.returncode != 0:
        raise SystemExit(proc.returncode)


def assert_ascii_paths() -> None:
    for path in ROOT.rglob('*'):
        name = path.name
        if '#U' in name:
            raise SystemExit(f'encoded Unicode path name remains: {path.relative_to(ROOT)}')
        try:
            name.encode('ascii')
        except UnicodeEncodeError as exc:
            raise SystemExit(f'non-ASCII toolchain path name remains: {path.relative_to(ROOT)}') from exc
    print('ASCII path-name guard passed.')


def run_host_c_tests() -> None:
    cc_bin = os.environ.get('CC', 'cc')
    cc_path = shutil.which(cc_bin)
    if cc_path is None:
        raise SystemExit(f"C compiler '{cc_bin}' is required for host protocol tests.")
    out = Path(tempfile.gettempdir()) / f"drug_car_host_test_{os.getpid()}"
    compile_args = [
        cc_path,
        '-std=c99', '-Wall', '-Wextra', '-Werror',
        '-Istm32f407_platformio/include',
        '-Istm32f407_platformio/test/host',
        '-Istm32f407_platformio/src/COMMON/CHE',
        '-Istm32f407_platformio/src/COMMON/STRATEGY',
        'stm32f407_platformio/test/host/test_protocol_core.c',
        '-o', str(out),
    ]
    try:
        subprocess.run(compile_args, check=True, timeout=30, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        subprocess.run([str(out)], check=True, timeout=10, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    finally:
        out.unlink(missing_ok=True)


def run_static_suite() -> None:
    os.chdir(ROOT)
    run_py('tools/check_python_syntax.py')
    run_py('tools/generate_protocol_contract.py', ['--check'])
    run_py('tools/protocol_golden_tests.py')
    run_py('tools/check_wrapper_boundaries.py')
    run_py('tools/check_role_duplicates.py')
    run_py('tools/check_include_resolution.py')
    assert_ascii_paths()
    run_py('tools/replay_validation.py')
    if not (ROOT / 'stm32f407_platformio' / 'platformio.ini').is_file():
        raise SystemExit('missing stm32f407_platformio/platformio.ini')


def run_pio_checks() -> None:
    run_py('tools/check_platformio_strict.py')
    pio_cwd = ROOT / 'stm32f407_platformio'
    run_shell('pio run -e car1', cwd=pio_cwd)
    run_shell('pio run -e car2', cwd=pio_cwd)
    run_shell('pio test -e native', cwd=pio_cwd)


def main(argv: list[str]) -> int:
    mode = argv[1] if len(argv) > 1 else 'auto'
    os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
    os.environ['PYTHONUNBUFFERED'] = '1'
    if mode == 'strict':
        # Fail fast when the PlatformIO toolchain is absent. This keeps strict
        # mode semantically hard-failing instead of spending time in quick
        # checks and then timing out in constrained shells. On a PlatformIO host,
        # the full static suite still runs before firmware build/test commands.
        run_py('tools/check_platformio_strict.py')
        run_static_suite()
        pio_cwd = ROOT / 'stm32f407_platformio'
        run_shell('pio run -e car1', cwd=pio_cwd)
        run_shell('pio run -e car2', cwd=pio_cwd)
        run_shell('pio test -e native', cwd=pio_cwd)
        return 0
    run_static_suite()
    if mode == 'quick':
        print('Quick static/protocol checks passed. PlatformIO checks intentionally skipped.')
        return 0
    if mode == 'auto':
        if shutil.which('pio') is not None:
            run_pio_checks()
        else:
            print("PlatformIO CLI not found; completed quick static/protocol checks only. Use './tools/check_static.sh quick' for host-C and './tools/check_static.sh strict' on a PlatformIO host.")
        return 0
    print('usage: check_static.py [auto|quick|strict]', file=sys.stderr)
    return 2


if __name__ == '__main__':
    try:
        rc = main(sys.argv)
    except SystemExit as exc:
        if isinstance(exc.code, str):
            print(exc.code, file=sys.stderr)
            rc = 1
        elif exc.code is None:
            rc = 0
        else:
            rc = int(exc.code)
    sys.exit(rc)
