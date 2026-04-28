# Full Closure Status

This document records the current no-residual closure state for the confirmed optimization plan.

| Item | Closure status | Enforcement |
|---|---|---|
| P0-01 ASCII paths | Fully closed | ASCII path guard fails on `#U` or non-ASCII toolchain paths. |
| P0-02 strict gate | Fully closed as a release gate | `strict` requires PlatformIO CLI and fails hard when it is unavailable. No firmware-build success is claimed without `pio`. |
| P0-03 USART2 ISR split | Fully closed | USART2 ISR submits bytes/frames only; CarLink parsing, event emission and ACK transmission run outside ISR context. |
| P1-01 stream parser | Fully closed | Host/native tests cover sticky halves, contiguous frames, internal sliding resync after bad frames, and CarLink legacy/checksum-aware stream resync. |
| P1-02 task terminal state | Fully closed | Task1/Task2/Task3 use task-id-aware terminal helpers and state-table records. |
| P1-03 debug inlet and event mailbox | Fully closed | car2 USART1 debug inlet is gated and only submits frames; no business parsing runs in that ISR. |
| P1-04 CarDiag consumer path | Fully closed | Diagnostic ring operations use short critical sections and expose copy/format/display consumers. |
| P1-05 line-lost low-speed search | Fully closed | Confirmed low-speed search remains enabled and configurable. |
| P2-01 duplicate governance | Fully closed | Role-local duplicate C wrappers, CHE adapters, and duplicate public headers are removed; bit-identical, unmanaged duplicate, or residual role-local CHE files fail the static gate. |
| P2-02 architecture docs | Fully closed | Architecture rules document ISR, protocol, service, strategy, task and role-boundary constraints. |
| P2-03 legacy docs | Fully closed | Legacy/debug surfaces are explicitly gated and documented. |
| ROLE_SHARED source migration | Fully closed | Shared USER/external-module sources are compiled directly from `COMMON/ROLE_SHARED`; shared CHE sources and public CHE headers are compiled/resolved directly from `COMMON/CHE`. |

Remaining validation boundary: PlatformIO firmware build and hardware execution are not claimed unless `./tools/check_static.sh strict` passes on a PlatformIO host.


Validation note: `tools/check_include_resolution.py` is part of quick/static validation and guards the car1/car2 shared-source include graph, including COMMON/PLATFORM and ROLE_SHARED strategy include paths.


Host-C protocol regression is available as `./tools/check_static.sh host` or `./tools/run_host_c_tests.sh`; strict mode invokes it after PlatformIO preflight on hosts with `pio`.
