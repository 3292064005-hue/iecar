# Architecture Boundaries

## Dependency direction

The STM32 firmware is organized around this dependency direction:

```text
Driver / ISR -> Protocol Parser -> Link Service -> Strategy -> Task State Machine -> Actuator Output
```

Rules:

1. ISR code copies bytes and marks frame boundaries only. Business parsing and ACK transmission must run from `CarLink_ServiceRx()` / `CarLink_ServiceTxQueue()` in task-loop context.
2. Protocol parsers validate frame structure only. They must not decide task states.
3. Link service may emit `car_link_event_t` events and compatibility mirrors (`leave_flag`, `go_flag`, `detect_num`), but task code should prefer the event mailbox when waiting for remote messages.
4. Strategy code decides turns, target validity and vote interpretation. It must not operate UART or GPIO directly.
5. Task code owns terminal states through `TaskRuntime_EnterSafeStopForTask()` and `TaskRuntime_EnterCompleteStopForTask()`.
6. Control output code may generate line-following speed output. OpenMV line-lost low-speed recovery is intentionally enabled by `CAR_OPENMV_LINE_LOST_LOW_SPEED_ENABLED` and is tracked by `car_line_lost_ticks`.

## Main closed loops

| Loop | Producer | Service/Parser | Consumer |
|---|---|---|---|
| K210 target frame | K210 `boot.py` | `CarProtocol_StreamFeedByte` + `CarProtocol_ParseK210RxBuffer` | Task vote logic |
| OpenMV line frame | OpenMV `line_follow.py` | `CarProtocol_StreamFeedByte` + `CarProtocol_ParseOpenMVRxBuffer` | `Car_BuildLineOutput` |
| CarLink USART2 | USART2 ISR | `CarProtocol_StreamFeedCarLinkByte` + `CarLink_ServiceRx` + `CarLink_ProcessFrame` | TaskLink wait/event APIs |
| Diagnostics | Protocol/task/link modules | `CarDiag_Record*` | `CarDiag_CopyRecent` / `CarDiag_ShowLatestSummary` |

## Compatibility boundaries

- `car2` USART1 no-ACK CarLink reception is an intentional debug compatibility inlet. It is gated by `CAR_LINK_USART1_DEBUG_RX_ENABLED` in the car2 role config; the ISR does not parse business payloads and only submits frames to the shared CarLink RX service.
- CarLink mirrors events to legacy globals only as a documented compatibility surface; event-mailbox consumption is the governed path.
- Role-shared external modules and USER entry points are built directly from `COMMON/ROLE_SHARED`; role-local duplicate C wrappers for those modules are not allowed.


## Role boundary governance

- Role-local CHE adapters and public headers have been removed. Shared CHE implementation and public interfaces live under `COMMON/CHE`; role-local CHE now contains only role-specific configuration and task-specific implementation files.
- Shared implementation belongs in `COMMON/CHE`, `COMMON/STRATEGY`, `COMMON/PLATFORM`, or `COMMON/ROLE_SHARED`; unmanaged duplicate role implementations fail the static gate.
- `tools/check_role_duplicates.py` fails on any bit-identical role file, unmanaged normalized duplicate implementation, or residual role-local CHE adapter/header.
- Task state tables must record state id, display name, motion-state flag, and generic timeout policy.
