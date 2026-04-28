#!/usr/bin/env python3
from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

ROOT = Path(__file__).resolve().parent.parent
STM32 = ROOT / 'stm32f407_platformio' / 'src'
COMMON_CFG = STM32 / 'COMMON' / 'CHE' / 'car_project_config_shared.h'
CAR1_CFG = STM32 / 'car1' / 'SYSTEM' / 'CHE' / 'car_project_config.h'
CAR2_CFG = STM32 / 'car2' / 'SYSTEM' / 'CHE' / 'car_project_config.h'
TASK1 = STM32 / 'COMMON' / 'CHE' / 'task1_shared.c'
CAR1_TASK2 = STM32 / 'car1' / 'SYSTEM' / 'CHE' / 'task2.c'
CAR1_TASK3 = STM32 / 'car1' / 'SYSTEM' / 'CHE' / 'task3.c'
CAR2_TASK2 = STM32 / 'car2' / 'SYSTEM' / 'CHE' / 'task2.c'
CAR2_TASK3 = STM32 / 'car2' / 'SYSTEM' / 'CHE' / 'task3.c'
SHARED_MAIN = STM32 / 'COMMON' / 'ROLE_SHARED' / 'USER' / 'main.c'
CTRL_NUM = STM32 / 'COMMON' / 'CHE' / 'ctrl_num_shared.c'
CTRL_HDR_SHARED = STM32 / 'COMMON' / 'CHE' / 'ctrl_num_shared.h'
BOOTSTRAP_SHARED = STM32 / 'COMMON' / 'STRATEGY' / 'task_bootstrap_common_shared.h'
LINK_RUNTIME_SHARED = STM32 / 'COMMON' / 'STRATEGY' / 'task_link_runtime_common_shared.h'
CAR_DIAG_SHARED = STM32 / 'COMMON' / 'CHE' / 'car_diag_shared.h'


def read(path: Path) -> str:
    return path.read_text(encoding='utf-8', errors='ignore')


def read_with_local_includes(path: Path) -> str:
    text = read(path)
    merged = [text]
    for inc in re.findall(r'#include\s+"([^"<>]+)"', text):
        target = (path.parent / inc).resolve()
        try:
            target.relative_to(ROOT.resolve())
        except ValueError:
            continue
        if target.exists() and target.suffix == '.c':
            merged.append(read(target))
    return '\n'.join(merged)


def parse_numeric_macros(path: Path) -> dict[str, int]:
    macros: dict[str, int] = {}
    pattern = re.compile(r'#define\s+(CAR_[A-Z0-9_]+)\s+\(?((?:0x[0-9A-Fa-f]+|[0-9]+))(?:[uUlLfF]*)\)?')
    for name, value in pattern.findall(read(path)):
        macros[name] = int(value, 0)
    return macros


def checksum8(payload: list[int]) -> int:
    return sum(payload[:8]) & 0xFF


def assert_true(cond: bool, msg: str) -> None:
    if not cond:
        raise AssertionError(msg)


@dataclass
class PendingTx:
    msg_type: int
    value: int
    seq: int
    retry: int
    deadline_ms: int


class CarLinkNode:
    def __init__(self, cfg: dict[str, int], role_id: int) -> None:
        self.cfg = cfg
        self.role_id = role_id
        self.now_ms = 0
        self.task = 2
        self.last_rx_seq = 0
        self.last_ack_seq = 0
        self.last_tx_seq = 0
        self.last_rx_time = 0
        self.last_tx_time = 0
        self.last_ack_time = 0
        self.last_heartbeat_tx_time = 0
        self.last_heartbeat_rx_time = 0
        self.tx_state = 'IDLE'
        self.pending: Optional[PendingTx] = None
        self.outbox: list[list[int]] = []

    def checksum(self, frame: list[int]) -> int:
        return checksum8(frame)

    def build_frame(self, msg_type: int, value: int, seq: int) -> list[int]:
        frame = [
            self.cfg['CAR_LINK_MAGIC0'],
            self.cfg['CAR_LINK_MAGIC1'],
            msg_type,
            value,
            seq,
            self.role_id,
            self.cfg['CAR_PROTOCOL_VERSION_MAJOR'],
            self.cfg['CAR_PROTOCOL_VERSION_MINOR'],
            0,
            0,
            self.cfg['CAR_LINK_TAIL'],
        ]
        frame[8] = self.checksum(frame)
        return frame

    def process_frame(self, frame: list[int], send_ack: bool = True) -> Optional[list[int]]:
        assert_true(len(frame) == self.cfg['CAR_LINK_FRAME_SIZE'], 'frame size mismatch during replay')
        assert_true(frame[0] == self.cfg['CAR_LINK_MAGIC0'] and frame[1] == self.cfg['CAR_LINK_MAGIC1'], 'magic mismatch during replay')
        assert_true(frame[10] == self.cfg['CAR_LINK_TAIL'], 'tail mismatch during replay')
        assert_true(frame[8] == self.checksum(frame), 'checksum mismatch during replay')
        assert_true(frame[5] in (1, 2), 'field5 sender role mismatch during replay')
        assert_true(frame[6] == self.cfg['CAR_PROTOCOL_VERSION_MAJOR'], 'protocol major mismatch during replay')
        msg_type = frame[2]
        value = frame[3]
        seq = frame[4]
        self.last_rx_time = self.now_ms
        self.last_rx_seq = seq
        if msg_type == self.cfg['CAR_LINK_MSG_ACK']:
            self.last_ack_seq = value
            self.last_ack_time = self.now_ms
            if self.pending and self.tx_state == 'PENDING' and value == self.pending.seq:
                self.tx_state = 'SUCCESS'
            return None
        if msg_type == self.cfg['CAR_LINK_MSG_HEARTBEAT']:
            self.last_heartbeat_rx_time = self.now_ms
        if send_ack:
            return self.build_frame(self.cfg['CAR_LINK_MSG_ACK'], seq, seq)
        return None

    def send_async(self, msg_type: int, value: int) -> bool:
        if self.tx_state == 'PENDING':
            return False
        self.last_tx_seq = (self.last_tx_seq + 1) & 0xFF
        self.pending = PendingTx(msg_type, value, self.last_tx_seq, 0, self.now_ms + self.cfg['CAR_LINK_ACK_TIMEOUT_MS'])
        self.tx_state = 'PENDING'
        self.last_tx_time = self.now_ms
        self.outbox.append(self.build_frame(msg_type, value, self.pending.seq))
        return True

    def service_tx(self) -> None:
        if self.tx_state != 'PENDING' or self.pending is None:
            return
        if self.last_ack_seq == self.pending.seq:
            self.last_ack_time = self.now_ms
            self.tx_state = 'SUCCESS'
            return
        if self.now_ms >= self.pending.deadline_ms:
            if self.pending.retry + 1 >= self.cfg['CAR_LINK_RETRY_COUNT']:
                self.tx_state = 'FAILED'
                return
            self.pending.retry += 1
            self.pending.deadline_ms = self.now_ms + self.cfg['CAR_LINK_ACK_TIMEOUT_MS'] + self.cfg['CAR_LINK_RETRY_INTERVAL_MS']
            self.last_tx_time = self.now_ms
            self.outbox.append(self.build_frame(self.pending.msg_type, self.pending.value, self.pending.seq))

    def service_heartbeat(self) -> None:
        if self.now_ms < self.cfg['CAR_LINK_HEARTBEAT_STARTUP_GRACE_MS']:
            return
        if self.tx_state == 'PENDING':
            return
        if self.now_ms - self.last_tx_time < self.cfg['CAR_LINK_HEARTBEAT_INTERVAL_MS']:
            return
        if self.now_ms - self.last_heartbeat_tx_time < self.cfg['CAR_LINK_HEARTBEAT_INTERVAL_MS']:
            return
        self.last_tx_seq = (self.last_tx_seq + 1) & 0xFF
        self.last_heartbeat_tx_time = self.now_ms
        self.last_tx_time = self.now_ms
        self.outbox.append(self.build_frame(self.cfg['CAR_LINK_MSG_HEARTBEAT'], self.role_id, self.last_tx_seq))

    def health(self) -> str:
        idle_ms = self.now_ms - self.last_rx_time
        if idle_ms <= self.cfg['CAR_LINK_FRESHNESS_MS']:
            return 'HEALTHY'
        if idle_ms <= self.cfg['CAR_LINK_DEGRADED_TIMEOUT_MS']:
            return 'DEGRADED'
        return 'LOST'


def deliver(sender: CarLinkNode, receiver: CarLinkNode, drop_ack: bool = False) -> None:
    out = sender.outbox[:]
    sender.outbox.clear()
    for frame in out:
        ack = receiver.process_frame(frame, send_ack=True)
        if ack is not None and not drop_ack:
            sender.process_frame(ack, send_ack=False)


def simulate_wait(timeout_ms: int, signal_at_ms: Optional[int]) -> str:
    start_ms = 0
    flag = False
    for now_ms in range(0, timeout_ms + 1000, 50):
        if signal_at_ms is not None and now_ms >= signal_at_ms:
            flag = True
        if flag:
            return 'READY'
        if now_ms - start_ms >= timeout_ms:
            return 'TIMEOUT'
    return 'PENDING'


def build_k210_frame(cfg: dict[str, int], mask: int, conf: int, x: int, y: int, seq: int) -> list[int]:
    wire = [
        cfg['CAR_K210_MAGIC0'],
        cfg['CAR_K210_MAGIC1'],
        cfg['CAR_PROTOCOL_VERSION_BYTE'],
        mask,
        conf,
        (x >> 8) & 0xFF,
        x & 0xFF,
        (y >> 8) & 0xFF,
        y & 0xFF,
        seq,
    ]
    checksum = sum(wire[:10]) & 0xFF
    return [cfg['CAR_K210_FRAME_SIZE'], *wire, checksum, cfg['CAR_K210_TAIL']]


def parse_k210_reliable(cfg: dict[str, int], frame: list[int], now_ms: int, rx_ms: int) -> bool:
    assert_true(frame[0] == cfg['CAR_K210_FRAME_SIZE'], 'k210 size mismatch')
    assert_true(frame[1] == cfg['CAR_K210_MAGIC0'] and frame[2] == cfg['CAR_K210_MAGIC1'], 'k210 magic mismatch')
    assert_true(frame[3] == cfg['CAR_PROTOCOL_VERSION_BYTE'], 'k210 visual version mismatch')
    assert_true(frame[12] == cfg['CAR_K210_TAIL'], 'k210 tail mismatch')
    assert_true((sum(frame[1:11]) & 0xFF) == frame[11], 'k210 checksum mismatch')
    mask = frame[4]
    conf = frame[5]
    x = (frame[6] << 8) | frame[7]
    y = (frame[8] << 8) | frame[9]
    fresh = (now_ms - rx_ms) <= cfg['CAR_K210_FRESHNESS_MS']
    pos_ok = cfg['CAR_K210_POSITION_MIN_X'] <= x <= cfg['CAR_K210_POSITION_MAX_X'] and cfg['CAR_K210_POSITION_MIN_Y'] <= y <= cfg['CAR_K210_POSITION_MAX_Y']
    conf_ok = conf >= cfg['CAR_K210_CONFIDENCE_MIN']
    return bool(mask and fresh and pos_ok and conf_ok)

def test_role_contracts() -> None:
    car1 = read(CAR1_CFG)
    car2 = read(CAR2_CFG)
    shared_main = read(SHARED_MAIN)
    platformio_ini = read(ROOT / 'stm32f407_platformio' / 'platformio.ini')
    assert_true('CAR_ALLOWED_TASK_MASK        (CAR_TASK_MASK_TASK1 | CAR_TASK_MASK_TASK2)' in car1, 'car1 allowed task mask mismatch')
    assert_true('CAR_LEGACY_TASK_MASK         (CAR_TASK_MASK_TASK3)' in car1, 'car1 legacy task mask mismatch')
    assert_true('CAR_ALLOWED_TASK_MASK        (CAR_TASK_MASK_TASK2 | CAR_TASK_MASK_TASK3)' in car2, 'car2 allowed task mask mismatch')
    assert_true(not (STM32 / 'car1' / 'USER' / 'main.c').exists(), 'car1 role-local main.c wrapper should be removed')
    assert_true(not (STM32 / 'car2' / 'USER' / 'main.c').exists(), 'car2 role-local main.c wrapper should be removed')
    assert_true('+<COMMON/CHE/*_shared.c>' in platformio_ini, 'shared CHE source filter missing')
    assert_true('+<COMMON/ROLE_SHARED/USER/*.c>' in platformio_ini, 'shared role USER source filter missing')
    assert_true('+<COMMON/ROLE_SHARED/SYSTEM/*.c>' in platformio_ini, 'shared role SYSTEM source filter missing')
    assert_true('TaskBootstrap_ReadSelector' in shared_main and 'TaskBootstrap_DispatchTaskWithContract' in shared_main, 'shared main missing bootstrap contract')
    assert_true('#include "car_mainchain.h"' in shared_main and '#include "task_bootstrap_common.h"' in shared_main, 'shared main must use role include-path headers')
    assert_true('../SYSTEM/' not in shared_main, 'shared main contains role-relative includes that break from COMMON/ROLE_SHARED')
    assert_true('task_bootstrap_entry_t' in read(BOOTSTRAP_SHARED), 'bootstrap shared layer missing')


def test_k210_contract(common: dict[str, int]) -> None:
    good = build_k210_frame(common, 0x01, common['CAR_K210_CONFIDENCE_MIN'], common['CAR_K210_POSITION_MIN_X'], common['CAR_K210_POSITION_MIN_Y'], 7)
    bad_conf = build_k210_frame(common, 0x01, common['CAR_K210_CONFIDENCE_MIN'] - 1, common['CAR_K210_POSITION_MIN_X'], common['CAR_K210_POSITION_MIN_Y'], 7)
    bad_pos = build_k210_frame(common, 0x01, common['CAR_K210_CONFIDENCE_MIN'], common['CAR_K210_POSITION_MAX_X'] + 1, common['CAR_K210_POSITION_MIN_Y'], 7)
    assert_true(parse_k210_reliable(common, good, 100, 0), 'reliable K210 observation failed')
    assert_true(not parse_k210_reliable(common, bad_conf, 100, 0), 'low-confidence K210 observation slipped through')
    assert_true(not parse_k210_reliable(common, bad_pos, 100, 0), 'out-of-range K210 observation slipped through')
    assert_true(not parse_k210_reliable(common, good, common['CAR_K210_FRESHNESS_MS'] + 1000, 0), 'stale K210 observation slipped through')
    k210_src = read(STM32 / 'COMMON' / 'CHE' / 'k210it_shared.c')
    assert_true('K210_GetObservation' in k210_src and 'K210_ObservationIsReliable' in k210_src, 'K210 observation API missing')
    assert_true('CarProtocol_ParseK210RxBuffer' in k210_src, 'K210 parser not using shared protocol core')
    assert_true('k210_reset_counter_array(counter);' in k210_src, 'K210 invalid/low-confidence frame does not clear stale vote counters')
    assert_true('PROTOCOL_VERSION_BYTE' in read(ROOT / 'k210_code' / 'boot.py'), 'K210 wire frame version byte missing')




def test_openmv_first_frame_contract(common: dict[str, int]) -> None:
    uart2 = read(STM32 / 'COMMON' / 'CHE' / 'car_uart2_shared.c')
    car_ctrl = read(STM32 / 'COMMON' / 'CHE' / 'car_ctrl_shared.c')
    assert_true('openmv_has_valid_frame' in uart2, 'OpenMV first-frame gate missing')
    assert_true('CarProtocol_ParseOpenMVRxBuffer' in uart2, 'OpenMV parser not using shared protocol core')
    assert_true('PROTOCOL_VERSION_BYTE' in read(ROOT / 'openmv_code' / 'line_follow.py'), 'OpenMV wire frame version byte missing')
    assert_true('OpenMV_GetStatus' in uart2, 'OpenMV status API missing')
    assert_true('CAR_LINE_EVENT_OPENMV_NO_FRAME' in read(STM32 / 'COMMON' / 'CHE' / 'car_ctrl.h'), 'line no-frame event missing')
    assert_true('Car_LineEventIsVisionFault' in car_ctrl, 'vision fault helper missing')
    assert_true('car_line_lost_ticks' in car_ctrl and 'CAR_OPENMV_LINE_LOST_LOW_SPEED_ENABLED' in car_ctrl, 'line-lost low-speed policy not surfaced')
    assert_true(common['CAR_OPENMV_REQUIRE_FIRST_FRAME'] == 1, 'OpenMV first-frame gate disabled')


def test_diag_contract() -> None:
    diag = read(CAR_DIAG_SHARED)
    assert_true('CAR_DIAG_LINK_DUP' in diag and 'CAR_DIAG_TASK_STATE' in diag, 'diagnostic event enum incomplete')
    diag_impl = read(STM32 / 'COMMON' / 'CHE' / 'car_diag_shared.c')
    assert_true('CarDiag_RecordThrottled' in diag, 'diagnostic throttling API missing')
    assert_true('CarDiag_CopyRecent' in diag and 'CarDiag_ShowLatestSummary' in diag, 'diagnostic consumer APIs missing')
    assert_true('__get_PRIMASK()' in diag_impl and '__set_PRIMASK(primask)' in diag_impl, 'diagnostic critical sections must restore prior IRQ state')
    assert_true((STM32 / 'COMMON' / 'CHE' / 'car_diag.h').exists(), 'shared diagnostic public header missing')
    for car in ('car1', 'car2'):
        assert_true(not (STM32 / car / 'SYSTEM' / 'CHE' / 'car_diag.c').exists(), f'{car} diagnostic wrapper should be removed')
        mainchain = read(STM32 / car / 'SYSTEM' / 'PLATFORM' / 'car_mainchain.h')
        assert_true('CAR_MAINCHAIN_DIAG_HEADER "car_diag.h"' in mainchain, f'{car} diagnostic header must resolve through common include path')

def test_car_link_contract(common: dict[str, int]) -> None:
    assert_true(common['CAR_LINK_ACK_TIMEOUT_MS'] < common['CAR_LINK_DEGRADED_TIMEOUT_MS'] < common['CAR_LINK_LOST_TIMEOUT_MS'], 'car link timeout ordering invalid')
    ctrl = read(CTRL_NUM)
    hdr = read(CTRL_HDR_SHARED)
    assert_true('CAR_LINK_MSG_HEARTBEAT' in ctrl and 'CarLink_ServiceHeartbeat' in ctrl, 'heartbeat keepalive not implemented')
    assert_true('CAR_LINK_LOST_TIMEOUT_MS' in ctrl and 'return CAR_LINK_LOST' in ctrl, 'link lost timeout configuration is not wired into health logic')
    assert_true('CarLink_SubmitRxFrameFromIsr' in ctrl and 'CarLink_ServiceRx' in ctrl, 'USART2 ISR is not decoupled from CarLink service')
    assert_true('__get_PRIMASK()' in ctrl and '__set_PRIMASK(primask)' in ctrl, 'CarLink RX critical section must restore prior IRQ state')
    assert_true('CarLink_ConsumeEventType' in ctrl and 'car_link_event_queue' in ctrl, 'CarLink event mailbox missing')
    assert_true('CarLink_ServiceHeartbeat' in hdr, 'heartbeat API not exposed')
    assert_true('CarLink_ServiceRx' in hdr and 'CarLink_TakeEvent' in hdr, 'CarLink service/event APIs not exposed')
    assert_true('car_link_duplicate_count' in hdr and 'car_link_unauthorized_count' in hdr, 'CarLink diagnostic counters not exposed')
    assert_true('car_link_message_is_allowed' in ctrl and 'car_link_is_duplicate' in ctrl, 'CarLink role/idempotency guards missing')
    assert_true('CarProtocol_NormalizeCarLinkSenderRole' in ctrl, 'CarLink compat sender role normalization missing')

    a = CarLinkNode(common, role_id=1)
    b = CarLinkNode(common, role_id=2)
    horizon = common['CAR_LINK_LOST_TIMEOUT_MS'] + common['CAR_LINK_HEARTBEAT_INTERVAL_MS'] * 4
    for now_ms in range(0, horizon + 1, 50):
        a.now_ms = now_ms
        b.now_ms = now_ms
        a.service_heartbeat()
        b.service_heartbeat()
        deliver(a, b)
        deliver(b, a)
    assert_true(a.health() != 'LOST' and b.health() != 'LOST', 'heartbeat replay did not keep link alive')
    assert_true(a.last_heartbeat_rx_time > 0 and b.last_heartbeat_rx_time > 0, 'heartbeat replay never exchanged frames')

    lone = CarLinkNode(common, role_id=1)
    lone.send_async(common['CAR_LINK_MSG_SET_DETECT'], 3)
    attempts = 1
    for now_ms in range(0, common['CAR_LINK_ACK_TIMEOUT_MS'] * (common['CAR_LINK_RETRY_COUNT'] + 2), 10):
        lone.now_ms = now_ms
        prev = len(lone.outbox)
        lone.service_tx()
        if len(lone.outbox) > prev:
            attempts += len(lone.outbox) - prev
            lone.outbox.clear()
        if lone.tx_state == 'FAILED':
            break
    assert_true(lone.tx_state == 'FAILED', 'async send should fail without ACK replay')
    assert_true(attempts == common['CAR_LINK_RETRY_COUNT'], 'retry count replay mismatch')

    a = CarLinkNode(common, role_id=1)
    b = CarLinkNode(common, role_id=2)
    assert_true(a.send_async(common['CAR_LINK_MSG_SET_DETECT'], 5), 'async send setup failed')
    first_delivery = True
    for now_ms in range(0, common['CAR_LINK_ACK_TIMEOUT_MS'] * 3, 10):
        a.now_ms = now_ms
        b.now_ms = now_ms
        a.service_tx()
        if a.outbox:
            deliver(a, b, drop_ack=first_delivery)
            first_delivery = False
        deliver(b, a)
        if a.tx_state == 'SUCCESS':
            break
    assert_true(a.tx_state == 'SUCCESS', 'async send did not recover after replayed ACK')


def test_state_machine_contracts() -> None:
    task1 = read(TASK1)
    car1_task2 = read(CAR1_TASK2)
    car1_task3 = read(CAR1_TASK3)
    car2_task2 = read(CAR2_TASK2)
    car2_task3 = read(CAR2_TASK3)
    assert_true('CarLink_ServiceHeartbeat();' in task1, 'task1 missing keepalive service')
    assert_true('TaskRuntime_RecordStateFromTable' in task1 and 'task1_state_desc' in task1, 'task1 missing unified state table recording')
    assert_true('is_motion_state' in read(STM32 / 'COMMON' / 'STRATEGY' / 'task_runtime_common_shared.h') and 'timeout_ms' in read(STM32 / 'COMMON' / 'STRATEGY' / 'task_runtime_common_shared.h'), 'task runtime state descriptor missing motion/timeout fields')
    assert_true('TaskLink_ServiceKeepalive("T2")' in car1_task2, 'car1 task2 missing shared link runtime')
    assert_true('TaskLink_ServiceKeepalive("T3")' in car1_task3, 'car1 task3 missing shared link runtime')
    assert_true('TaskLink_WaitForRemoteFlag' in car2_task2, 'car2 task2 missing shared remote-wait runtime')
    assert_true(car2_task3.count('TaskLink_WaitForRemoteFlag') >= 2, 'car2 task3 missing shared remote-wait runtime')
    assert_true('TASK2_STATE_WAIT_TX_RESULT' in car1_task2 and 'TASK2_STATE_SAFE_STOP' in car1_task2, 'car1 task2 state machine contract missing')
    assert_true('TASK3_STATE_COMPLETE_STOP' in car2_task3 and 'TASK3_STATE_SAFE_STOP' in car2_task3, 'car2 task3 finish/safe-stop split missing')
    assert_true('TaskRuntime_RecordStateFromTable' in car1_task2 and 'task2_state_desc' in car1_task2, 'car1 task2 missing unified state table recording')
    assert_true('TaskRuntime_RecordStateFromTable' in car2_task2 and 'task2_state_desc' in car2_task2, 'car2 task2 missing unified state table recording')
    assert_true('TaskRuntime_RecordStateFromTable' in car1_task3 and 'task3_state_desc' in car1_task3, 'car1 task3 missing unified state table recording')
    assert_true('TaskRuntime_RecordStateFromTable' in car2_task3 and 'task3_state_desc' in car2_task3, 'car2 task3 missing unified state table recording')
    assert_true('Task2_RequestCompleteStop' in car2_task2, 'car2 task2 normal completion helper missing')
    assert_true('Task2_RequestSafeStop("stop")' not in car2_task2, 'car2 task2 still maps normal entry stop to safe stop')
    assert_true('K210_ObservationIsReliable(K210_CAMERA_LEFT)' in car2_task3, 'car2 task3 missing K210 reliability gate')
    assert_true('legacy' in car1_task3, 'car1 task3 legacy marker missing')
    link_runtime = read(LINK_RUNTIME_SHARED)
    assert_true('TaskLink_ServiceKeepalive' in link_runtime, 'shared link runtime layer missing')
    assert_true('TaskLink_WaitForRemoteMessage' in link_runtime and 'CarLink_ConsumeEventType' in link_runtime, 'remote wait does not consume event mailbox')

    assert_true(simulate_wait(5000, 1500) == 'READY', 'state wait replay should complete on scheduled signal')
    assert_true(simulate_wait(3000, None) == 'TIMEOUT', 'state wait replay should timeout without signal')


def test_cleanup_contract() -> None:
    allowed_role_che = {'car_project_config.h', 'task2.c', 'task3.c'}
    for car in ('car1', 'car2'):
        che_files = {p.name for p in (STM32 / car / 'SYSTEM' / 'CHE').iterdir() if p.is_file()}
        assert_true(che_files <= allowed_role_che, f'{car} still has residual role-local CHE adapters/headers: {sorted(che_files - allowed_role_che)}')
        for path in (STM32 / car).rglob('*.c'):
            assert_true('COMMON/ROLE_SHARED' not in read(path), f'role-local wrapper still includes COMMON/ROLE_SHARED: {path.relative_to(STM32)}')
            assert_true('COMMON/CHE/' not in read(path), f'role-local C still includes COMMON/CHE implementation: {path.relative_to(STM32)}')
    typo_label = ROOT / 'k210_code' / 'lables.txt'
    assert_true(not typo_label.exists(), 'duplicate typo labels file should be removed')
    assert_true((ROOT / 'stm32f407_platformio' / 'test' / 'test_native' / 'test_protocol_contract.cpp').exists(), 'PlatformIO native test path mismatch')
    platformio_ini = read(ROOT / 'stm32f407_platformio' / 'platformio.ini')
    assert_true('platformio/ststm32@^19.0.0' in platformio_ini, 'strict PlatformIO STM32 platform constraint is not pinned')
    assert_true((ROOT / 'tools' / 'platformio_strict_manifest.json').exists(), 'strict PlatformIO manifest missing')
    ctrl_public = read(STM32 / 'COMMON' / 'CHE' / 'ctrl_num.h')
    diag_public = read(STM32 / 'COMMON' / 'CHE' / 'car_diag.h')
    assert_true('../PLATFORM/car_platform.h' not in ctrl_public, 'ctrl_num.h uses stale role-relative platform include after COMMON migration')
    assert_true('../PLATFORM/car_platform.h' not in diag_public, 'car_diag.h uses stale role-relative platform include after COMMON migration')
    assert_true('#define CAR_CTRLNUM_PLATFORM_HEADER "car_platform.h"' in ctrl_public, 'ctrl_num.h must resolve platform header through include path')
    assert_true('#define CAR_DIAG_PLATFORM_HEADER "car_platform.h"' in diag_public, 'car_diag.h must resolve platform header through include path')



def assert_legacy_detached(project_root: Path) -> None:
    platformio_ini = (project_root / 'stm32f407_platformio' / 'platformio.ini').read_text(encoding='utf-8', errors='ignore')
    removed_headers = [
        'ak8975.h', 'attitude_up.h', 'ICM20602.h', 'SPL06_001.h', 'imu_spi.h',
        'balance_init.h', 'balance_control.h', 'onefly_control.h', 'banqiu.h',
        'my_algorithm.h', 'fenglibai.h', 'balance_car.h',
    ]
    legacy_dirs = [
        'flycontrol_v1', 'balance_car_bujin', 'one_fly_contorl', 'banqiu',
        'gxt_algorithm', 'fenglibai', 'balance_car'
    ]
    sysh = (project_root / 'stm32f407_platformio' / 'src' / 'COMMON' / 'ROLE_SHARED' / 'SYSTEM' / 'sys' / 'sys.h').read_text(encoding='utf-8', errors='ignore')
    for header in removed_headers:
        if header in sysh:
            raise AssertionError(f'shared sys.h still imports legacy header: {header}')
    for car_name in ('car1', 'car2'):
        for legacy_dir in legacy_dirs:
            if (project_root / 'stm32f407_platformio' / 'src' / car_name / 'SYSTEM' / legacy_dir).exists():
                raise AssertionError(f'{car_name}: legacy directory still present: {legacy_dir}')
    for token in ('flycontrol_v1', 'balance_car_bujin', 'one_fly_contorl', 'banqiu', 'gxt_algorithm', 'fenglibai', 'balance_car'):
        if token in platformio_ini:
            raise AssertionError(f'platformio.ini should not reference legacy module: {token}')

def main() -> None:
    common = parse_numeric_macros(COMMON_CFG)
    common.update(parse_numeric_macros(STM32 / 'COMMON' / 'CHE' / 'protocol_contract_generated.h'))
    needed = [
        'CAR_LINK_ACK_TIMEOUT_MS', 'CAR_LINK_DEGRADED_TIMEOUT_MS', 'CAR_LINK_LOST_TIMEOUT_MS',
        'CAR_LINK_MAGIC0', 'CAR_LINK_MAGIC1', 'CAR_LINK_TAIL', 'CAR_PROJECT_VERSION_MAJOR',
        'CAR_PROJECT_VERSION_MINOR', 'CAR_K210_CONFIDENCE_MIN', 'CAR_K210_POSITION_MIN_X',
        'CAR_K210_POSITION_MAX_X', 'CAR_K210_POSITION_MIN_Y', 'CAR_K210_POSITION_MAX_Y',
        'CAR_LINK_MSG_ACK', 'CAR_LINK_MSG_SET_DETECT', 'CAR_LINK_MSG_HEARTBEAT', 'CAR_K210_FRAME_SIZE',
        'CAR_K210_MAGIC0', 'CAR_K210_MAGIC1', 'CAR_K210_TAIL', 'CAR_K210_FRESHNESS_MS',
        'CAR_LINK_HEARTBEAT_INTERVAL_MS', 'CAR_LINK_HEARTBEAT_STARTUP_GRACE_MS', 'CAR_LINK_RETRY_COUNT',
        'CAR_LINK_RETRY_INTERVAL_MS', 'CAR_LINK_FRESHNESS_MS', 'CAR_PROTOCOL_VERSION_MAJOR', 'CAR_PROTOCOL_VERSION_MINOR', 'CAR_PROTOCOL_VERSION_BYTE', 'CAR_LINK_STRICT_ROLE_MATRIX', 'CAR_OPENMV_REQUIRE_FIRST_FRAME'
    ]
    for item in needed:
        assert_true(item in common, f'missing macro: {item}')
    test_role_contracts()
    assert_legacy_detached(ROOT)
    test_k210_contract(common)
    test_openmv_first_frame_contract(common)
    test_diag_contract()
    test_car_link_contract(common)
    test_state_machine_contracts()
    test_cleanup_contract()
    print('Protocol replay and state simulation passed.')


if __name__ == '__main__':
    import sys
    main()
    sys.exit(0)
