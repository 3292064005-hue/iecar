#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
PYTHON_BIN=${PYTHON_BIN:-${PYTHON:-}}
if [ -z "$PYTHON_BIN" ]; then
  if [ -x /usr/bin/python3 ]; then
    PYTHON_BIN=/usr/bin/python3
  else
    PYTHON_BIN=python3
  fi
fi
PYTHON_FLAGS=${PYTHON_FLAGS:--S}
MODE=${1:-auto}
cd "$ROOT"

run_py() {
  "$PYTHON_BIN" $PYTHON_FLAGS "$@"
}

run_py - <<'PY'
from pathlib import Path
for rel in [
    'tools/replay_validation.py',
    'tools/protocol_contract.py',
    'tools/generate_protocol_contract.py',
    'tools/protocol_golden_tests.py',
    'tools/check_wrapper_boundaries.py',
]:
    path = Path(rel)
    compile(path.read_text(encoding='utf-8'), str(path), 'exec')
print('Python source syntax check passed without writing __pycache__.')
PY
run_py tools/generate_protocol_contract.py --check
run_py tools/protocol_golden_tests.py
run_py tools/check_wrapper_boundaries.py
run_py tools/replay_validation.py

if [ ! -f stm32f407_platformio/platformio.ini ]; then
  echo "missing stm32f407_platformio/platformio.ini" >&2
  exit 1
fi

run_host_c_tests() {
  local cc_bin="${CC:-cc}"
  if ! command -v "$cc_bin" >/dev/null 2>&1; then
    echo "C compiler '${cc_bin}' is required for host protocol tests." >&2
    exit 2
  fi
  local out
  out="$(mktemp /tmp/drug_car_host_test.XXXXXX)"
  "$cc_bin" -std=c99 -Wall -Wextra -Werror \
    -Istm32f407_platformio/include \
    -Istm32f407_platformio/test/host \
    -Istm32f407_platformio/src/COMMON/CHE \
    -Istm32f407_platformio/src/COMMON/STRATEGY \
    stm32f407_platformio/test/host/test_protocol_core.c \
    -o "$out"
  "$out"
  rm -f "$out"
}

run_host_c_tests

run_pio_checks() {
  (cd stm32f407_platformio && pio run -e car1 && pio run -e car2 && pio test -e native)
}

case "$MODE" in
  quick)
    echo "Quick static/protocol/host-C checks passed. PlatformIO checks intentionally skipped."
    ;;
  strict)
    if ! command -v pio >/dev/null 2>&1; then
      echo "PlatformIO CLI 'pio' is required for strict mode." >&2
      exit 2
    fi
    run_pio_checks
    ;;
  auto)
    if command -v pio >/dev/null 2>&1; then
      run_pio_checks
    else
      echo "PlatformIO CLI not found; completed quick static/protocol/host-C checks only. Use './tools/check_static.sh strict' on a PlatformIO host."
    fi
    ;;
  *)
    echo "usage: $0 [auto|quick|strict]" >&2
    exit 2
    ;;
esac
