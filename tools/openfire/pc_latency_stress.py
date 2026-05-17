#!/usr/bin/env python3
import argparse
import struct
import time
from collections import deque

MAGIC = b"OF"
TYPE_PING = 1
TYPE_PONG = 2
HDR_FMT = "<2sBHI"  # magic, type, seq, ts_us
HDR_SZ = struct.calcsize(HDR_FMT)


def now_us() -> int:
    return time.perf_counter_ns() // 1000


def percentile(sorted_vals, p):
    if not sorted_vals:
        return 0
    idx = int((len(sorted_vals) - 1) * p)
    return sorted_vals[idx]


def make_pkt(tp: int, seq: int, ts_us: int, payload: bytes) -> bytes:
    return struct.pack(HDR_FMT, MAGIC, tp, seq & 0xFFFF, ts_us & 0xFFFFFFFF) + payload


def parse_pkt(buf: bytes):
    if len(buf) < HDR_SZ:
        return None
    magic, tp, seq, ts = struct.unpack(HDR_FMT, buf[:HDR_SZ])
    if magic != MAGIC:
        return None
    return tp, seq, ts, buf[HDR_SZ:]


def run_device_mode(args):
    import serial
    ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    seq = 0
    rtts = []
    lost = 0
    sent = 0
    recv = 0
    deadline = time.time() + args.duration
    payload = bytes([0xA5]) * args.payload
    pending = {}

    print(f"[stress] start port={args.port} baud={args.baud} duration={args.duration}s")
    while time.time() < deadline:
        ts = now_us()
        pkt = make_pkt(TYPE_PING, seq, ts, payload)
        ser.write(pkt)
        pending[seq] = ts
        sent += 1

        resp = ser.read(HDR_SZ + args.payload)
        if resp:
            dec = parse_pkt(resp)
            if dec and dec[0] == TYPE_PONG:
                _, rseq, rts, _ = dec
                if rseq in pending:
                    rtt = now_us() - pending.pop(rseq)
                    rtts.append(rtt)
                    recv += 1
        # timeout cleanup
        if len(pending) > args.window:
            # drop oldest outstanding
            k = sorted(pending.keys())[0]
            pending.pop(k, None)
            lost += 1

        seq = (seq + 1) & 0xFFFF
        if args.interval_us > 0:
            time.sleep(args.interval_us / 1_000_000.0)

    lost += len(pending)
    rtts.sort()
    print(f"[stress] sent={sent} recv={recv} lost={lost}")
    print(f"[stress] rtt_us p50={percentile(rtts,0.50)} p95={percentile(rtts,0.95)} p99={percentile(rtts,0.99)} max={rtts[-1] if rtts else 0}")


def run_loopback_mode(args):
    """本机无板验证：构造虚拟延时分布，验证统计逻辑。"""
    q = deque()
    rtts = []
    start = time.time()
    seq = 0
    while time.time() - start < args.duration:
        t0 = now_us()
        q.append((seq, t0))
        # 模拟链路延时抖动
        sim = 300 + (seq % 11) * 40
        time.sleep(sim / 1_000_000.0)
        _, ts = q.popleft()
        rtts.append(now_us() - ts)
        seq += 1
    rtts.sort()
    print(f"[loopback] samples={len(rtts)} p50={percentile(rtts,0.5)} p95={percentile(rtts,0.95)} p99={percentile(rtts,0.99)}")


def main():
    ap = argparse.ArgumentParser(description="OpenFIRE TR531X latency stress tool")
    ap.add_argument("--port", help="serial port, e.g. COM7 or /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=512000)
    ap.add_argument("--duration", type=int, default=30)
    ap.add_argument("--payload", type=int, default=16)
    ap.add_argument("--interval-us", type=int, default=1000)
    ap.add_argument("--timeout", type=float, default=0.02)
    ap.add_argument("--window", type=int, default=256)
    ap.add_argument("--loopback", action="store_true")
    args = ap.parse_args()

    if args.loopback:
        run_loopback_mode(args)
        return

    if not args.port:
        raise SystemExit("--port is required unless --loopback is set")
    run_device_mode(args)


if __name__ == "__main__":
    main()
