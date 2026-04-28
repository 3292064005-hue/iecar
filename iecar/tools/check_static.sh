#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
export PYTHONDONTWRITEBYTECODE=1
export PYTHONUNBUFFERED=1
PYTHON_BIN=${PYTHON_BIN:-${PYTHON:-}}
if [ -z "$PYTHON_BIN" ]; then
  if [ -x /usr/bin/python3 ]; then
    PYTHON_BIN=/usr/bin/python3
  else
    PYTHON_BIN=python3
  fi
fi
MODE=${1:-auto}
case "$MODE" in
  quick)
    "$PYTHON_BIN" -S tools/check_static.py quick
    echo "Quick static/protocol checks passed. Host-C is available via ./tools/run_host_c_tests.sh; PlatformIO checks intentionally skipped."
    exit 0
    ;;
  strict)
    "$PYTHON_BIN" -S tools/check_platformio_strict.py
    "$PYTHON_BIN" -S tools/check_static.py quick
    bash tools/run_host_c_tests.sh
    (cd stm32f407_platformio && pio run -e car1 && pio run -e car2 && pio test -e native)
    ;;
  auto)
    "$PYTHON_BIN" -S tools/check_static.py quick
    if command -v pio >/dev/null 2>&1; then
      "$PYTHON_BIN" -S tools/check_platformio_strict.py
      (cd stm32f407_platformio && pio run -e car1 && pio run -e car2 && pio test -e native)
    else
      echo "PlatformIO CLI not found; completed quick static/protocol checks only. Run './tools/check_static.sh host' for host-C and './tools/check_static.sh strict' on a PlatformIO host."
      exit 0
    fi
    ;;
  host)
    bash tools/run_host_c_tests.sh
    exit 0
    ;;
  *)
    echo "usage: $0 [auto|quick|strict|host]" >&2
    exit 2
    ;;
esac
