# Implementation And Migration Notes

## Landed items

- P0-01: Removed encoded/Chinese path names from deployable project paths and updated protocol generation/replay/documentation references to ASCII paths.
- P0-02: Kept `quick` and `strict` validation split. `strict` fails hard when PlatformIO CLI is unavailable and therefore cannot be mistaken for a completed firmware build.
- P0-03: USART2 ISR now submits candidate CarLink bytes/frames only. CarLink parsing, business event emission and ACK transmission run through `CarLink_ServiceRx()` / `CarLink_ServiceTxQueue()` outside ISR context.
- P1-01: Added fixed-length stream resynchronization through `CarProtocol_StreamFeedByte` for K210/OpenMV and `CarProtocol_StreamFeedCarLinkByte` for CarLink, preserving legacy CarLink compatibility and fixed-buffer parser validation.
- P1-02: Added unified terminal helpers `TaskRuntime_EnterSafeStopForTask` and `TaskRuntime_EnterCompleteStopForTask`; Task1/Task2/Task3 terminal paths use task-id-aware records.
- P1-03: Added CarLink event mailbox (`car_link_event_t`) and task wait helper consumption while preserving legacy `leave_flag` / `go_flag` / `detect_num` mirrors. The car2 USART1 no-ACK debug inlet is documented and gated.
- P1-04: Added diagnosis consumer APIs: `CarDiag_CopyRecent`, `CarDiag_FormatRecord`, and `CarDiag_ShowLatestSummary`.
- P1-05: Preserved confirmed OpenMV line-lost low-speed recovery and made it explicit/configurable with counters.
- P2-01: Role-local C wrappers for external modules, USER entry points, CHE shared adapters, and duplicate public headers were removed. PlatformIO now builds shared external modules from `COMMON/ROLE_SHARED` and shared CHE implementation from `COMMON/CHE` directly.
- P2-02: Added architecture boundary document.
- P2-03: Updated legacy compatibility document with keep/remove conditions.

## Migration steps

1. Run `./tools/check_static.sh quick` after any protocol, task, or path change.
2. Run `./tools/check_static.sh strict` on a machine with PlatformIO CLI before flashing competition firmware.
3. For protocol changes, edit only `tools/protocol_contract.py`, then run `tools/generate_protocol_contract.py`.
4. For car2 USART1 debug reception, keep `CAR_LINK_USART1_DEBUG_RX_ENABLED` enabled only on debug-compatible builds.
5. For line-lost behavior, keep `CAR_OPENMV_LINE_LOST_LOW_SPEED_ENABLED=1` for the confirmed low-speed search strategy; set to 0 only for a safety-stop profile.

## Rollback

- Path rollback: the old encoded/Chinese paths are not retained. Rollback is by restoring the previous archive only.
- USART2 ISR rollback: revert `U2_DATA_DEAL` to direct `CarLink_ProcessFrame`; not recommended because it restores ISR-time ACK transmission.
- Event mailbox rollback: legacy flag mirrors are retained as an explicit compatibility surface, so event consumption can be bypassed by restoring `TaskLink_WaitForRemoteFlag` to direct flag polling.
- Line-lost rollback: set `CAR_OPENMV_LINE_LOST_LOW_SPEED_ENABLED` to 0 to stop on lost line without changing task code.


Host-C protocol regression is available as `./tools/check_static.sh host` or `./tools/run_host_c_tests.sh`; strict mode invokes it after PlatformIO preflight on hosts with `pio`.
