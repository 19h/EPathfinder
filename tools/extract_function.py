#!/usr/bin/env python3
"""Print one complete CodeDumper function block by name or hexadecimal RVA."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


MARKER = re.compile(
    r"^// --- Function: (?P<name>.+) \(0x(?P<address>[0-9A-Fa-f]+)\) ---$"
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("decompilation", type=Path)
    parser.add_argument("query", help="exact mangled name or hexadecimal RVA")
    args = parser.parse_args()

    query = args.query.lower().removeprefix("0x")
    collecting = False
    found = False
    with args.decompilation.open("r", encoding="utf-8", errors="replace") as stream:
        for line in stream:
            marker = MARKER.match(line.rstrip("\n"))
            if marker and not collecting:
                collecting = (
                    marker.group("name") == args.query
                    or marker.group("address").lower() == query
                )
                found = found or collecting
            if collecting:
                print(line, end="")
                if line.startswith("// --- End Function:"):
                    return 0
    return 0 if found else 1


if __name__ == "__main__":
    raise SystemExit(main())

