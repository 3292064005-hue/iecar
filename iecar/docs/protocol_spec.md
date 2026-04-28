# 三端协议规格 v2.3

本文档是 STM32、K210、OpenMV 与双车 CarLink 的协议契约入口。`tools/protocol_contract.py` 是单源配置；执行 `tools/generate_protocol_contract.py` 会同步生成：

- `stm32f407_platformio/src/COMMON/CHE/protocol_contract_generated.h`
- `stm32f407_platformio/include/protocol_contract_host.h`
- `k210_code/protocol_contract.py`
- `openmv_code/protocol_contract.py`

`./tools/check_static.sh quick` 会执行 `generate_protocol_contract.py --check`，若任一生成物与单源配置不一致则失败。

## 通用规则

- 帧头：`0xAA 0xFF`
- 帧尾：`0xCC`
- checksum：8 位累加和，取低 8 位。
- 视觉端 wire frame 是真实 UART 线上发送的字节；STM32 RX buffer 会在 `buf[0]` 额外写入本地接收长度，该长度字节不在线缆上传输。
- STM32 对 v2 视觉帧执行 `PROTOCOL_VERSION_BYTE` 校验；`CAR_VISUAL_PROTOCOL_COMPAT_V1` 可临时接收旧 11 字节无版本视觉帧。
- STM32 对 CarLink 正式帧执行 major version 校验；`CAR_PROTOCOL_COMPAT_V1` 可临时接收旧 zero-checksum 帧。

## K210 → STM32 wire frame，12 字节

| 线缆字节 | 含义 |
|---:|---|
| 0 | `0xAA` |
| 1 | `0xFF` |
| 2 | visual protocol version byte：`(major << 4) | minor`，当前 `0x23` |
| 3 | 类别 bitmask，bit0~bit7 对应 1~8 |
| 4 | 最佳目标置信度，0~100 |
| 5~6 | 最佳目标中心 x |
| 7~8 | 最佳目标中心 y |
| 9 | seq |
| 10 | `sum(byte0..byte9) & 0xFF` |
| 11 | `0xCC` |

STM32 本地 RX buffer 对应 13 字节：`buf[0] = 12`，`buf[1..12] = wire[0..11]`。STM32 侧必须同时满足首帧存在、freshness、置信度和坐标范围，才能把 K210 观测作为任务决策输入。低置信度或脏坐标帧会清空本帧 mask 并清理对应投票窗口，避免 stale vote 继续影响策略。

## OpenMV → STM32 wire frame，12 字节

| 线缆字节 | 含义 |
|---:|---|
| 0 | `0xAA` |
| 1 | `0xFF` |
| 2 | visual protocol version byte：`(major << 4) | minor`，当前 `0x23` |
| 3~4 | 巡线中心 x |
| 5~6 | 东侧/右侧 y |
| 7~8 | 西侧/左侧 y |
| 9 | 黑块计数 |
| 10 | `sum(byte0..byte9) & 0xFF` |
| 11 | `0xCC` |

STM32 本地 RX buffer 对应 13 字节：`buf[0] = 12`，`buf[1..12] = wire[0..11]`。STM32 侧区分 `NO_FRAME`、`FRESH`、`STALE`，首帧前不得把零值 `openmvdata` 当作有效巡线输入。

## CarLink STM32 ↔ STM32 wire frame，11 字节

| 字节 | 含义 |
|---:|---|
| 0 | `0xAA` |
| 1 | `0xFF` |
| 2 | msg type：`1=SET_DETECT`, `2=LEAVE`, `3=GO`, `4=HEARTBEAT`, `0x7F=ACK` |
| 3 | value；ACK 帧中为被确认 seq |
| 4 | seq |
| 5 | sender role：`1=car1`, `2=car2`；兼容旧 task-field 帧时先归一化为 expected peer role |
| 6 | protocol major |
| 7 | protocol minor |
| 8 | `sum(byte0..byte7) & 0xFF` |
| 9 | 保留，固定 0 |
| 10 | `0xCC` |

角色矩阵：car1 可向 car2 发送 `SET_DETECT`、`LEAVE`、`GO`；两端均可发送 `HEARTBEAT` 和 `ACK`。重复业务帧按 seq 幂等处理：补发 ACK，但不重复写业务 flag。

## Receive service model

- K210 and OpenMV RXNE paths feed bytes into `CarProtocol_StreamFeedByte`, which can resynchronize after noise prefixes and handle frames split across receive interrupts. The existing fixed-buffer parsers remain the final authority for field extraction and compatibility checks.
- USART2 CarLink RXNE/IDLE handling does not execute business logic directly. `U2_DATA_DEAL()` submits bytes through the stream parser and enqueues complete candidate frames. `CarLink_ServiceRx()` performs validation and event emission outside ISR context.
- ACK frames are queued by `CarLink_SendAck()` and sent by `CarLink_ServiceTxQueue()` from non-ISR context.
- car2 USART1 no-ACK CarLink input is a debug compatibility inlet. When `CAR_LINK_USART1_DEBUG_RX_ENABLED` is enabled, the ISR only frames bytes and submits them with `send_ack=0`; `CarLink_ServiceRx()` performs the actual parsing outside interrupt context.


## CarLink stream boundary

CarLink reception uses `CarProtocol_StreamFeedCarLinkByte()` before queue submission. The helper accepts current checksum/version frames and legacy no-checksum frames, and it performs sliding-window resync on bad tail/checksum/version frames before business parsing runs in `CarLink_ServiceRx()`.
