#!/usr/bin/env python3
"""Compare the ELF's generated MAVLink message table with vendored headers."""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import struct
import subprocess


ENTRY_PATTERN = re.compile(
    r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,"
    r"\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}"
)
SYMBOL_PATTERN = re.compile(
    r"^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+\w\s+"
    r"_ZZ21mavlink_get_msg_entryE20mavlink_message_crcs$"
)


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=pathlib.Path, default=pathlib.Path("EPathfinder"))
    parser.add_argument(
        "--vendor",
        type=pathlib.Path,
        default=pathlib.Path("vendor/mavlink-c-library-v2"),
    )
    return parser.parse_args()


def symbol_extent(binary: pathlib.Path) -> tuple[int, int]:
    output = subprocess.check_output(
        ["llvm-nm", "-n", "-S", "--defined-only", str(binary)], text=True
    )
    for line in output.splitlines():
        match = SYMBOL_PATTERN.match(line)
        if match:
            return int(match.group(1), 16), int(match.group(2), 16)
    raise RuntimeError("ELF MAVLink CRC-table symbol was not found")


def virtual_address_to_offset(blob: bytes, address: int, size: int) -> int:
    if blob[:4] != b"\x7fELF" or blob[4] != 2 or blob[5] != 1:
        raise RuntimeError("expected a little-endian ELF64 binary")
    header = struct.unpack_from("<HHIQQQIHHHHHH", blob, 16)
    program_offset = header[4]
    program_entry_size = header[8]
    program_count = header[9]
    for index in range(program_count):
        fields = struct.unpack_from(
            "<IIQQQQQQ", blob, program_offset + index * program_entry_size
        )
        segment_type, _, file_offset, virtual_address, _, file_size, _, _ = fields
        if (
            segment_type == 1
            and virtual_address <= address
            and address + size <= virtual_address + file_size
        ):
            return file_offset + address - virtual_address
    raise RuntimeError("MAVLink table is not contained in a file-backed LOAD segment")


def vendor_table(vendor: pathlib.Path) -> tuple[bytes, int, str]:
    dialect = vendor / "ardupilotmega" / "ardupilotmega.h"
    macro = next(
        line
        for line in dialect.read_text().splitlines()
        if line.startswith("#define MAVLINK_MESSAGE_CRCS ")
    )
    entries = [tuple(map(int, match)) for match in ENTRY_PATTERN.findall(macro)]
    table = b"".join(struct.pack("<IBBBBBBxx", *entry) for entry in entries)

    mavlink_header = (vendor / "ardupilotmega" / "mavlink.h").read_text()
    xml_hash = int(
        re.search(r"#define MAVLINK_PRIMARY_XML_HASH (\d+)", mavlink_header).group(1)
    )
    version_header = (vendor / "ardupilotmega" / "version.h").read_text()
    build_date = re.search(
        r'#define MAVLINK_BUILD_DATE "([^"]+)"', version_header
    ).group(1)
    return table, xml_hash, build_date


def main() -> int:
    args = arguments()
    address, size = symbol_extent(args.binary)
    blob = args.binary.read_bytes()
    offset = virtual_address_to_offset(blob, address, size)
    binary_table = blob[offset : offset + size]
    generated_table, xml_hash, build_date = vendor_table(args.vendor)

    if binary_table != generated_table:
        raise SystemExit(
            "MAVLink table mismatch: "
            f"ELF={len(binary_table)} bytes, vendor={len(generated_table)} bytes"
        )

    commit = subprocess.check_output(
        ["git", "-C", str(args.vendor), "rev-parse", "HEAD"], text=True
    ).strip()
    print(f"exact table match: {len(binary_table) // 12} entries, {len(binary_table)} bytes")
    print(f"table SHA-256: {hashlib.sha256(binary_table).hexdigest()}")
    print(f"vendor commit: {commit}")
    print(f"primary XML hash: {xml_hash}")
    print(f"generator build date: {build_date}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
