# Legacy And Compatibility Policy

## Kept intentionally

| Item | Status | Reason | Guard |
|---|---|---|---|
| car2 USART1 no-ACK CarLink input | Kept | Confirmed debug compatibility inlet | `CAR_LINK_USART1_DEBUG_RX_ENABLED` |
| OpenMV line-lost low-speed motion | Kept | Confirmed low-speed line recovery strategy | `CAR_OPENMV_LINE_LOST_LOW_SPEED_ENABLED` |
| car1 legacy Task3 marker | Kept | Historical compatibility, not in normal allowed task mask | `CAR_LEGACY_TASK_MASK` |
| CHE shared-implementation adapters | Kept as role boundary | They provide role include context for CHE shared code and are thin-adapter checked | `check_wrapper_boundaries.py` |

## Removed or renamed

- Encoded/Chinese path names were replaced with ASCII paths:
  - `k210_code/`
  - `openmv_code/`
  - `openmv_code/line_follow.py`
  - `docs/implementation_migration.md`
  - `docs/maintenance_map.md`
  - `docs/legacy_cleanup.md`
  - `docs/platformio_migration.md`
- Generated protocol artifacts now target only ASCII paths.
- Delivery packages must not include `__pycache__`, `.pio`, `.git`, temporary logs or build products.

## Deletion conditions

1. `./tools/check_static.sh strict` passes on a host with PlatformIO CLI.
2. `pio run -e car1`, `pio run -e car2`, and `pio test -e native` are recorded for the release host.
3. Duplicate role files identified by `tools/check_role_duplicates.py` are migrated in small batches after each batch passes strict validation.
