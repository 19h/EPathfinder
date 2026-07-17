#!/usr/bin/env python3
"""Scan non-sparse, unallocated ext4 blocks for forensic artifacts.

The input is never mounted or modified.  Sparse holes are skipped with
SEEK_DATA/SEEK_HOLE, ext4 block bitmaps select free blocks, and candidate
MAVLink AUTOPILOT_VERSION frames are accepted only after checksum validation.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import struct
import sys
from collections.abc import Iterator


DEFAULT_PATTERNS = (
    b"45INAV06",
    b"45INAV07",
    b"MAVLINK_MSG_ID_AUTOPILOT_VERSION:",
    b"board_version:",
    b"flight_custom_version:",
    b"saver_nano.json",
    b"saver_nx.json",
    b"capture.json",
    b"thunder_gaze.json",
    b"vnav/config.py",
)


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def u64(data: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", data, offset)[0]


class Ext4:
    def __init__(self, fd: int) -> None:
        superblock = os.pread(fd, 1024, 1024)
        if len(superblock) != 1024 or u16(superblock, 0x38) != 0xEF53:
            raise ValueError("input is not an ext4 filesystem image")

        self.fd = fd
        self.block_size = 1024 << u32(superblock, 0x18)
        self.first_data_block = u32(superblock, 0x14)
        self.blocks_per_group = u32(superblock, 0x20)
        blocks = u32(superblock, 0x04)
        feature_incompat = u32(superblock, 0x60)
        if feature_incompat & 0x80:
            blocks |= u32(superblock, 0x150) << 32
        self.blocks_count = blocks
        self.group_count = (
            blocks - self.first_data_block + self.blocks_per_group - 1
        ) // self.blocks_per_group
        desc_size = u16(superblock, 0xFE) if feature_incompat & 0x80 else 32
        self.desc_size = max(desc_size, 32)
        descriptor_block = 2 if self.block_size == 1024 else 1
        descriptor_bytes = self.group_count * self.desc_size
        self.descriptors = os.pread(
            fd, descriptor_bytes, descriptor_block * self.block_size
        )
        if len(self.descriptors) != descriptor_bytes:
            raise ValueError("short read of ext4 group descriptor table")

    def free_runs(self) -> Iterator[tuple[int, int]]:
        """Yield byte ranges for runs of blocks marked free by ext4."""
        pending_start: int | None = None
        pending_end = 0
        for group in range(self.group_count):
            descriptor = self.descriptors[
                group * self.desc_size : (group + 1) * self.desc_size
            ]
            bitmap_block = u32(descriptor, 0)
            if self.desc_size >= 64:
                bitmap_block |= u32(descriptor, 0x20) << 32
            bitmap = os.pread(
                self.fd, self.block_size, bitmap_block * self.block_size
            )
            if len(bitmap) != self.block_size:
                raise ValueError(f"short read of block bitmap for group {group}")

            group_first = self.first_data_block + group * self.blocks_per_group
            group_blocks = min(
                self.blocks_per_group, self.blocks_count - group_first
            )
            run_start: int | None = None
            for index in range(group_blocks):
                allocated = bitmap[index >> 3] & (1 << (index & 7))
                if not allocated and run_start is None:
                    run_start = index
                if allocated and run_start is not None:
                    start = (group_first + run_start) * self.block_size
                    end = (group_first + index) * self.block_size
                    if pending_start is None:
                        pending_start, pending_end = start, end
                    elif pending_end == start:
                        pending_end = end
                    else:
                        yield pending_start, pending_end
                        pending_start, pending_end = start, end
                    run_start = None
            if run_start is not None:
                start = (group_first + run_start) * self.block_size
                end = (group_first + group_blocks) * self.block_size
                if pending_start is None:
                    pending_start, pending_end = start, end
                elif pending_end == start:
                    pending_end = end
                else:
                    yield pending_start, pending_end
                    pending_start, pending_end = start, end
        if pending_start is not None:
            yield pending_start, pending_end


def sparse_data_runs(fd: int, size: int) -> Iterator[tuple[int, int]]:
    position = 0
    while position < size:
        try:
            start = os.lseek(fd, position, os.SEEK_DATA)
        except OSError:
            return
        try:
            end = os.lseek(fd, start, os.SEEK_HOLE)
        except OSError:
            end = size
        yield start, min(end, size)
        position = max(end, start + 1)


def intersections(
    first: Iterator[tuple[int, int]], second: Iterator[tuple[int, int]]
) -> Iterator[tuple[int, int]]:
    try:
        a = next(first)
        b = next(second)
        while True:
            start = max(a[0], b[0])
            end = min(a[1], b[1])
            if start < end:
                yield start, end
            if a[1] <= b[1]:
                a = next(first)
            else:
                b = next(second)
    except StopIteration:
        return


def crc_accumulate(byte: int, crc: int) -> int:
    tmp = byte ^ (crc & 0xFF)
    tmp ^= (tmp << 4) & 0xFF
    return (
        (crc >> 8)
        ^ (tmp << 8)
        ^ (tmp << 3)
        ^ (tmp >> 4)
    ) & 0xFFFF


def mavlink_crc(data: bytes, extra: int) -> int:
    crc = 0xFFFF
    for byte in data:
        crc = crc_accumulate(byte, crc)
    return crc_accumulate(extra, crc)


def printable(value: bytes) -> str:
    return "".join(chr(byte) if 32 <= byte < 127 else "." for byte in value)


def check_autopilot_version(blob: bytes, base: int) -> None:
    for magic in (0xFD, 0xFE):
        marker = bytes((magic,))
        offset = blob.find(marker)
        while offset >= 0:
            if magic == 0xFD:
                if offset + 12 > len(blob):
                    offset = blob.find(marker, offset + 1)
                    continue
                length = blob[offset + 1]
                if not (60 <= length <= 78):
                    offset = blob.find(marker, offset + 1)
                    continue
                if blob[offset + 7 : offset + 10] != b"\x94\x00\x00":
                    offset = blob.find(marker, offset + 1)
                    continue
                checksum_offset = offset + 10 + length
                if checksum_offset + 2 > len(blob):
                    offset = blob.find(marker, offset + 1)
                    continue
                expected = u16(blob, checksum_offset)
                actual = mavlink_crc(
                    blob[offset + 1 : checksum_offset], 178
                )
                payload_offset = offset + 10
                version = 2
            else:
                if offset + 8 > len(blob):
                    offset = blob.find(marker, offset + 1)
                    continue
                length = blob[offset + 1]
                if length != 60 or blob[offset + 5] != 148:
                    offset = blob.find(marker, offset + 1)
                    continue
                checksum_offset = offset + 6 + length
                if checksum_offset + 2 > len(blob):
                    offset = blob.find(marker, offset + 1)
                    continue
                expected = u16(blob, checksum_offset)
                actual = mavlink_crc(
                    blob[offset + 1 : checksum_offset], 178
                )
                payload_offset = offset + 6
                version = 1
            if actual != expected:
                offset = blob.find(marker, offset + 1)
                continue

            payload = blob[payload_offset : payload_offset + length]
            print(
                "MAVLINK_AUTOPILOT_VERSION"
                f" offset={base + offset} v={version} len={length}"
                f" sys={blob[offset + (5 if version == 2 else 3)]}"
                f" comp={blob[offset + (6 if version == 2 else 4)]}"
                f" capabilities=0x{u64(payload, 0):016x}"
                f" uid=0x{u64(payload, 8):016x}"
                f" flight_sw=0x{u32(payload, 16):08x}"
                f" middleware_sw=0x{u32(payload, 20):08x}"
                f" os_sw=0x{u32(payload, 24):08x}"
                f" board=0x{u32(payload, 28):08x}"
                f" vendor={u16(payload, 32)} product={u16(payload, 34)}"
                f" flight_custom={printable(payload[36:44])!r}"
                f" middleware_custom={printable(payload[44:52])!r}"
                f" os_custom={printable(payload[52:60])!r}"
                + (
                    f" uid2={payload[60:78].hex()}"
                    if length >= 78
                    else ""
                )
            )
            offset = blob.find(marker, offset + 1)


def scan_range(
    fd: int, start: int, end: int, patterns: tuple[bytes, ...], chunk_size: int
) -> None:
    overlap_size = max(max(map(len, patterns), default=1), 90) - 1
    position = start
    previous = b""
    while position < end:
        data = os.pread(fd, min(chunk_size, end - position), position)
        if not data:
            return
        blob = previous + data
        base = position - len(previous)
        for pattern in patterns:
            found = blob.find(pattern)
            while found >= 0:
                absolute = base + found
                if absolute >= start:
                    print(
                        f"PATTERN offset={absolute} value={pattern.decode('ascii')}"
                    )
                found = blob.find(pattern, found + 1)
        check_autopilot_version(blob, base)
        previous = blob[-overlap_size:]
        position += len(data)


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=pathlib.Path)
    parser.add_argument("--plan-only", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--start-offset", type=int, default=0)
    parser.add_argument("--chunk-mib", type=int, default=16)
    parser.add_argument(
        "--pattern", action="append", default=[], help="additional ASCII pattern"
    )
    return parser.parse_args()


def main() -> int:
    args = arguments()
    flags = os.O_RDONLY | getattr(os, "O_BINARY", 0)
    fd = os.open(args.image, flags)
    try:
        filesystem = Ext4(fd)
        size = os.fstat(fd).st_size
        runs = list(
            intersections(
                filesystem.free_runs(), sparse_data_runs(fd, size)
            )
        )
        total = sum(end - start for start, end in runs)
        print(
            f"image={args.image} block_size={filesystem.block_size} "
            f"blocks={filesystem.blocks_count} groups={filesystem.group_count} "
            f"non_sparse_free_runs={len(runs)} "
            f"non_sparse_free_bytes={total}"
        )
        if args.plan_only:
            return 0
        patterns = DEFAULT_PATTERNS + tuple(
            pattern.encode("ascii") for pattern in args.pattern
        )
        for index, (start, end) in enumerate(runs, 1):
            if end <= args.start_offset:
                continue
            start = max(start, args.start_offset)
            if args.verbose:
                print(
                    f"RUN {index}/{len(runs)} start={start} end={end} "
                    f"bytes={end-start}"
                )
            scan_range(fd, start, end, patterns, args.chunk_mib * 1024 * 1024)
        return 0
    finally:
        os.close(fd)


if __name__ == "__main__":
    sys.exit(main())
