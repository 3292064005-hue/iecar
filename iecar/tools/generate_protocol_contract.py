#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / 'tools') not in sys.path:
    sys.path.insert(0, str(ROOT / 'tools'))
import protocol_contract as pc

TARGETS = {
    ROOT / 'stm32f407_platformio/src/COMMON/CHE/protocol_contract_generated.h': pc.render_c_generated_header,
    ROOT / 'stm32f407_platformio/include/protocol_contract_host.h': pc.render_host_header,
    ROOT / 'k210代码/protocol_contract.py': lambda: pc.render_device_python('k210'),
    ROOT / 'opemv代码/protocol_contract.py': lambda: pc.render_device_python('openmv'),
}


def main() -> int:
    parser = argparse.ArgumentParser(description='Generate or verify protocol contract artifacts.')
    parser.add_argument('--check', action='store_true', help='fail if generated artifacts are stale')
    args = parser.parse_args()

    stale = []
    for path, renderer in TARGETS.items():
        expected = renderer()
        if args.check:
            actual = path.read_text(encoding='utf-8') if path.exists() else None
            if actual != expected:
                stale.append(str(path.relative_to(ROOT)))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(expected, encoding='utf-8')
    if stale:
        for item in stale:
            print(f'generated protocol artifact is stale: {item}', file=sys.stderr)
        return 1
    if args.check:
        print('Protocol generated artifacts are synchronized.')
    else:
        print('Protocol generated artifacts updated.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
