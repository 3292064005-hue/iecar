#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
CC_BIN="${CC:-cc}"
OUT="$(mktemp /tmp/drug_car_host_test.XXXXXX)"
rm -f "$OUT"
cleanup() { rm -f "$OUT"; }
trap cleanup EXIT
"$CC_BIN" -std=c99 -Wall -Wextra -Werror \
  -Istm32f407_platformio/include \
  -Istm32f407_platformio/test/host \
  -Istm32f407_platformio/src/COMMON/CHE \
  -Istm32f407_platformio/src/COMMON/STRATEGY \
  stm32f407_platformio/test/host/test_protocol_core.c \
  -o "$OUT"
"$OUT"
echo "Host C protocol tests passed."
