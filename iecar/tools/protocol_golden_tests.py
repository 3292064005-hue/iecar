#!/usr/bin/env python3
from __future__ import annotations

import protocol_contract as pc


def assert_equal(actual, expected, msg: str) -> None:
    if actual != expected:
        raise AssertionError(f"{msg}: expected {expected!r}, got {actual!r}")


def test_k210_wire_and_rx_buffer() -> None:
    wire = pc.pack_k210_wire(0x01, pc.K210_CONFIDENCE_MIN, 12, 34, 7)
    rxbuf = pc.pack_k210_rxbuf(0x01, pc.K210_CONFIDENCE_MIN, 12, 34, 7)
    assert_equal(len(wire), pc.K210_WIRE_FRAME_SIZE, "k210 wire length")
    assert_equal(len(rxbuf), pc.K210_RX_BUFFER_SIZE, "k210 rx buffer length")
    assert_equal(rxbuf[0], pc.K210_WIRE_FRAME_SIZE, "k210 local size marker")
    assert_equal(bytes(rxbuf[1:]), wire, "k210 rx payload equals wire")
    assert_equal(wire[0], pc.MAGIC0, "k210 magic0")
    assert_equal(wire[1], pc.MAGIC1, "k210 magic1")
    assert_equal(wire[2], pc.PROTOCOL_VERSION_BYTE, "k210 visual version")
    assert_equal(wire[10], pc.checksum8(wire[:10]), "k210 checksum")
    assert_equal(wire[11], pc.TAIL, "k210 tail")


def test_openmv_wire_and_rx_buffer() -> None:
    wire = pc.pack_openmv_wire(160, 121, 0, 3)
    rxbuf = pc.pack_openmv_rxbuf(160, 121, 0, 3)
    assert_equal(len(wire), pc.OPENMV_WIRE_FRAME_SIZE, "openmv wire length")
    assert_equal(len(rxbuf), pc.OPENMV_RX_BUFFER_SIZE, "openmv rx buffer length")
    assert_equal(rxbuf[0], pc.OPENMV_WIRE_FRAME_SIZE, "openmv local size marker")
    assert_equal(bytes(rxbuf[1:]), wire, "openmv rx payload equals wire")
    assert_equal(wire[2], pc.PROTOCOL_VERSION_BYTE, "openmv visual version")
    assert_equal(wire[10], pc.checksum8(wire[:10]), "openmv checksum")
    assert_equal(wire[11], pc.TAIL, "openmv tail")


def test_carlink() -> None:
    frame = pc.pack_carlink(pc.CARLINK_MSG_SET_DETECT, 5, 9, sender_role=1)
    assert_equal(len(frame), pc.CARLINK_WIRE_FRAME_SIZE, "carlink byte length")
    assert_equal(frame[5], 1, "carlink field5 sender role")
    assert_equal(frame[6], pc.PROTOCOL_VERSION_MAJOR, "carlink major version")
    assert_equal(frame[8], pc.checksum8(frame[:8]), "carlink checksum")
    assert_equal(frame[10], pc.TAIL, "carlink tail")


def main() -> None:
    test_k210_wire_and_rx_buffer()
    test_openmv_wire_and_rx_buffer()
    test_carlink()
    print("Protocol golden tests passed.")


if __name__ == "__main__":
    main()
