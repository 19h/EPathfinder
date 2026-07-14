#!/usr/bin/env python3
"""Build a reproducible symbol/decompilation inventory for EPathfinder.

The script is intentionally read-only with respect to the evidence artifacts.
It writes deterministic reports beneath --output.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import shutil
import subprocess
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable


FUNCTION_MARKER = re.compile(
    r"^// --- Function: (?P<name>.+) \(0x(?P<address>[0-9A-Fa-f]+)\) ---$"
)
NEEDED = re.compile(r"Shared library: \[(?P<name>[^]]+)\]")
BUILD_ID = re.compile(r"Build ID: (?P<id>[0-9a-f]+)")


@dataclass(frozen=True)
class Function:
    address: int
    size: int
    symbol_type: str
    mangled: str
    demangled: str
    owner: str
    provenance: str
    recovery_kind: str
    decompiled: bool


@dataclass(frozen=True)
class ObjectSymbol:
    address: int
    size: int
    name: str


def run(command: list[str]) -> str:
    return subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    ).stdout


def find_tool(name: str) -> str:
    result = shutil.which(name)
    if result is None:
        raise RuntimeError(f"required tool is unavailable: {name}")
    return result


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_nm(text: str) -> list[tuple[str, str, int, int]]:
    result: list[tuple[str, str, int, int]] = []
    for line in text.splitlines():
        fields = line.rsplit(maxsplit=3)
        if len(fields) != 4:
            continue
        name, symbol_type, address_text, size_text = fields
        if symbol_type not in {"T", "t", "W", "w"}:
            continue
        try:
            result.append(
                (name, symbol_type, int(address_text, 16), int(size_text, 16))
            )
        except ValueError:
            continue
    return result


def parse_readelf_functions(text: str) -> list[tuple[str, str, int, int]]:
    """Parse defined STT_FUNC entries from .symtab, excluding .dynsym copies."""
    result: list[tuple[str, str, int, int]] = []
    in_full_symbol_table = False
    for line in text.splitlines():
        if line.startswith("Symbol table '"):
            in_full_symbol_table = line.startswith("Symbol table '.symtab'")
            continue
        if not in_full_symbol_table:
            continue
        fields = line.split(maxsplit=7)
        if len(fields) != 8 or fields[3] != "FUNC" or fields[6] == "UND":
            continue
        try:
            address = int(fields[1], 16)
            size = int(fields[2], 10)
        except ValueError:
            continue
        binding = fields[4]
        symbol_type = {
            "GLOBAL": "T",
            "LOCAL": "t",
            "WEAK": "W",
        }.get(binding, binding)
        result.append((fields[7], symbol_type, address, size))
    return result


def parse_readelf_objects(text: str) -> list[ObjectSymbol]:
    result: list[ObjectSymbol] = []
    in_full_symbol_table = False
    for line in text.splitlines():
        if line.startswith("Symbol table '"):
            in_full_symbol_table = line.startswith("Symbol table '.symtab'")
            continue
        if not in_full_symbol_table:
            continue
        fields = line.split(maxsplit=7)
        if len(fields) != 8 or fields[3] != "OBJECT" or fields[6] == "UND":
            continue
        try:
            result.append(
                ObjectSymbol(
                    address=int(fields[1], 16),
                    size=int(fields[2], 10),
                    name=fields[7],
                )
            )
        except ValueError:
            continue
    return result


QMETATYPE_NAMES = {
    0: "UnknownType",
    1: "bool",
    2: "int",
    3: "uint",
    4: "qlonglong",
    5: "qulonglong",
    6: "double",
    7: "QChar",
    8: "QVariantMap",
    9: "QVariantList",
    10: "QString",
    11: "QStringList",
    12: "QByteArray",
    13: "QBitArray",
    14: "QDate",
    15: "QTime",
    16: "QDateTime",
    17: "QUrl",
    18: "QLocale",
    19: "QRect",
    20: "QRectF",
    21: "QSize",
    22: "QSizeF",
    23: "QLine",
    24: "QLineF",
    25: "QPoint",
    26: "QPointF",
    27: "QRegExp",
    28: "QVariantHash",
    29: "QEasingCurve",
    30: "QUuid",
    31: "void*",
    32: "long",
    33: "short",
    34: "char",
    35: "ulong",
    36: "ushort",
    37: "uchar",
    38: "float",
    39: "QObject*",
    40: "signed char",
    41: "QVariant",
    42: "QModelIndex",
    43: "void",
    44: "QRegularExpression",
    45: "QJsonValue",
    46: "QJsonObject",
    47: "QJsonArray",
    48: "QJsonDocument",
    49: "QByteArrayList",
    50: "QPersistentModelIndex",
    51: "std::nullptr_t",
}


def moc_suffix(symbol_name: str, token: str) -> str | None:
    position = symbol_name.find(token)
    return symbol_name[position + len(token) :] if position >= 0 else None


def parse_moc_strings(binary: bytes, symbol: ObjectSymbol) -> list[str]:
    if symbol.address + symbol.size > len(binary) or symbol.size < 24:
        raise ValueError(f"MOC string object is outside file data: {symbol.name}")
    first_offset = int.from_bytes(
        binary[symbol.address + 16 : symbol.address + 24],
        byteorder="little",
        signed=True,
    )
    if first_offset <= 0 or first_offset % 24 != 0 or first_offset > symbol.size:
        raise ValueError(f"invalid QByteArrayData extent in {symbol.name}")
    count = first_offset // 24
    result: list[str] = []
    for index in range(count):
        descriptor = symbol.address + index * 24
        length = int.from_bytes(
            binary[descriptor + 4 : descriptor + 8], "little", signed=True
        )
        relative = int.from_bytes(
            binary[descriptor + 16 : descriptor + 24], "little", signed=True
        )
        start = descriptor + relative
        end = start + length
        if length < 0 or start < symbol.address or end > symbol.address + symbol.size:
            raise ValueError(f"invalid MOC string descriptor in {symbol.name}")
        result.append(binary[start:end].decode("utf-8", errors="replace"))
    return result


def decode_meta_type(value: int, strings: list[str]) -> str:
    if value & 0x80000000:
        index = value & 0x7FFFFFFF
        return strings[index] if index < len(strings) else f"string[{index}]"
    return QMETATYPE_NAMES.get(value, f"QMetaType({value})")


def parse_moc_methods(binary: bytes,
                      symbol: ObjectSymbol,
                      strings: list[str]) -> dict[str, object]:
    raw = binary[symbol.address : symbol.address + symbol.size]
    if len(raw) % 4 != 0:
        raise ValueError(f"unaligned MOC metadata: {symbol.name}")
    words = [
        int.from_bytes(raw[offset : offset + 4], "little")
        for offset in range(0, len(raw), 4)
    ]
    if len(words) < 14:
        raise ValueError(f"short MOC metadata: {symbol.name}")
    method_count, method_offset, signal_count = words[4], words[5], words[13]
    property_count, property_offset = words[6], words[7]
    enum_count, enum_offset = words[8], words[9]
    methods: list[dict[str, object]] = []
    for index in range(method_count):
        base = method_offset + index * 5
        if base + 5 > len(words):
            raise ValueError(f"method table exceeds {symbol.name}")
        name_index, argc, parameters, tag_index, flags = words[base : base + 5]
        end = parameters + 1 + argc * 2
        if end > len(words):
            raise ValueError(f"parameter table exceeds {symbol.name}")
        access = {0: "private", 1: "protected", 2: "public"}.get(
            flags & 0x3, "unknown"
        )
        kind = {0: "method", 4: "signal", 8: "slot", 12: "constructor"}.get(
            flags & 0xC, "unknown"
        )
        argument_types = [
            decode_meta_type(words[parameters + 1 + arg], strings)
            for arg in range(argc)
        ]
        argument_names = [
            strings[words[parameters + 1 + argc + arg]]
            if words[parameters + 1 + argc + arg] < len(strings)
            else f"string[{words[parameters + 1 + argc + arg]}]"
            for arg in range(argc)
        ]
        methods.append(
            {
                "index": index,
                "name": strings[name_index] if name_index < len(strings) else f"string[{name_index}]",
                "kind": kind,
                "access": access,
                "return_type": decode_meta_type(words[parameters], strings),
                "argument_types": argument_types,
                "argument_names": argument_names,
                "tag": strings[tag_index] if tag_index < len(strings) else f"string[{tag_index}]",
                "flags": flags,
                "is_signal_by_index": index < signal_count,
            }
        )
    properties: list[dict[str, object]] = []
    for index in range(property_count):
        base = property_offset + index * 3
        if base + 3 > len(words):
            raise ValueError(f"property table exceeds {symbol.name}")
        name_index, type_value, flags = words[base : base + 3]
        properties.append(
            {
                "index": index,
                "name": strings[name_index]
                if name_index < len(strings)
                else f"string[{name_index}]",
                "type": decode_meta_type(type_value, strings),
                "flags": flags,
            }
        )
    enums: list[dict[str, object]] = []
    for index in range(enum_count):
        base = enum_offset + index * 4
        if base + 4 > len(words):
            raise ValueError(f"enum table exceeds {symbol.name}")
        name_index, flags, value_count, values_offset = words[base : base + 4]
        if values_offset + value_count * 2 > len(words):
            raise ValueError(f"enum values exceed {symbol.name}")
        values = []
        for value_index in range(value_count):
            value_name, value = words[
                values_offset + value_index * 2 : values_offset + value_index * 2 + 2
            ]
            values.append(
                {
                    "name": strings[value_name]
                    if value_name < len(strings)
                    else f"string[{value_name}]",
                    "value": value,
                }
            )
        enums.append(
            {
                "index": index,
                "name": strings[name_index]
                if name_index < len(strings)
                else f"string[{name_index}]",
                "flags": flags,
                "values": values,
            }
        )
    return {
        "revision": words[0],
        "class_name_index": words[1],
        "method_count": method_count,
        "signal_count": signal_count,
        "property_count": property_count,
        "properties": properties,
        "enum_count": enum_count,
        "enums": enums,
        "constructor_count": words[10],
        "flags": words[12],
        "methods": methods,
    }


def parse_metaobjects(binary: bytes,
                      objects: list[ObjectSymbol]) -> list[dict[str, object]]:
    string_symbols: dict[str, ObjectSymbol] = {}
    data_symbols: dict[str, ObjectSymbol] = {}
    for symbol in objects:
        suffix = moc_suffix(symbol.name, "qt_meta_stringdata_")
        if suffix is not None:
            string_symbols[suffix] = symbol
        suffix = moc_suffix(symbol.name, "qt_meta_data_")
        if suffix is not None:
            data_symbols[suffix] = symbol

    result: list[dict[str, object]] = []
    for suffix, string_symbol in sorted(string_symbols.items()):
        strings = parse_moc_strings(binary, string_symbol)
        item: dict[str, object] = {
            "symbol_suffix": suffix,
            "class_name": strings[0] if strings else suffix,
            "string_symbol": asdict(string_symbol),
            "strings": strings,
        }
        data_symbol = data_symbols.get(suffix)
        if data_symbol is not None:
            item["data_symbol"] = asdict(data_symbol)
            item.update(parse_moc_methods(binary, data_symbol, strings))
        result.append(item)
    return result


def demangle(names: Iterable[str], cxxfilt: str) -> list[str]:
    values = list(names)
    if not values:
        return []
    process = subprocess.run(
        [cxxfilt],
        input="\n".join(values) + "\n",
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return process.stdout.splitlines()


def parse_decompilation(path: Path) -> tuple[dict[int, str], int]:
    functions: dict[int, str] = {}
    line_count = 0
    with path.open("r", encoding="utf-8", errors="replace") as stream:
        for line_count, line in enumerate(stream, start=1):
            match = FUNCTION_MARKER.match(line.rstrip("\n"))
            if match:
                functions[int(match.group("address"), 16)] = match.group("name")
    return functions, line_count


def parse_file_symbols(readelf_symbols: str) -> list[str]:
    units: list[str] = []
    for line in readelf_symbols.splitlines():
        fields = line.split(maxsplit=7)
        if len(fields) >= 7 and fields[3] == "FILE":
            units.append(fields[7] if len(fields) == 8 else "<anonymous>")
    return units


def provenance(address: int) -> str:
    if 0x224C0 <= address < 0x24700:
        return "dynamic_plt"
    if 0x24700 <= address < 0x25C38:
        return "application_startup"
    if 0x25C38 <= address < 0x1BDF3C:
        return "application"
    if 0x1BDF3C <= address < 0x23DB20:
        return "livox_sdk_static"
    if 0x23DB20 <= address <= 0x23DBC8:
        return "toolchain_runtime"
    return "other"


def owner_of(demangled_name: str) -> str:
    head = demangled_name.split("(", 1)[0]
    if "::" not in head:
        return "<global>"
    owner = head.rsplit("::", 1)[0]
    # Remove a leading return type when c++filt emits one for unusual symbols.
    return owner.rsplit(" ", 1)[-1]


def recovery_kind(prov: str, mangled: str, demangled_name: str) -> str:
    if prov == "dynamic_plt":
        return "library_import"
    if prov == "livox_sdk_static":
        return "replace_with_livox_sdk"
    if prov in {"application_startup", "toolchain_runtime"}:
        return "generated_runtime"
    if prov != "application":
        return "unclassified"

    generated_tokens = (
        "qt_static_metacall",
        "qt_metacall",
        "qt_metacast",
        "qt_meta_stringdata_",
        "staticMetaObject",
        "QMetaTypeFunctionHelper",
        "QSlotObject",
        "qRegisterNormalizedMetaType",
        "typeinfo for ",
        "typeinfo name for ",
        "vtable for ",
    )
    if any(token in demangled_name for token in generated_tokens):
        return "qt_or_compiler_generated"

    generated_prefixes = (
        "_ZN5QList",
        "_ZN7QVector",
        "_ZN4QMap",
        "_ZN5QHash",
        "_ZN8QMapData",
        "_ZN8QMapNode",
        "_ZNSt",
        "_ZSt",
        "_ZN9__gnu_cxx",
        "_ZN9QtPrivate",
        "_ZN17QtMetaTypePrivate",
    )
    if mangled.startswith(generated_prefixes):
        return "template_instantiation"
    return "manual_source_candidate"


def unit_group(index: int, name: str) -> str:
    if Path(name).name in {"crtstuff.c", "<anonymous>"}:
        return "toolchain"
    if 119 <= index <= 140:
        return "livox_sdk"
    if 4 <= index <= 118:
        return "application_moc" if Path(name).name.startswith("moc_") else "application"
    return "toolchain"


def write_tsv(path: Path, functions: list[Function]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(
            [
                "address",
                "size",
                "type",
                "provenance",
                "recovery_kind",
                "decompiled",
                "owner",
                "mangled",
                "demangled",
            ]
        )
        for function in functions:
            writer.writerow(
                [
                    f"0x{function.address:x}",
                    function.size,
                    function.symbol_type,
                    function.provenance,
                    function.recovery_kind,
                    str(function.decompiled).lower(),
                    function.owner,
                    function.mangled,
                    function.demangled,
                ]
            )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--decompilation", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    binary = args.binary.resolve()
    decompilation = args.decompilation.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)

    readelf = find_tool("llvm-readelf")
    cxxfilt = find_tool("llvm-cxxfilt")

    elf_headers = run([readelf, "-h", "-d", "-n", "-p", ".comment", str(binary)])
    symbol_table = run([readelf, "--symbols", "--wide", str(binary)])
    decompiled_by_address, decompilation_lines = parse_decompilation(decompilation)
    raw_functions = parse_readelf_functions(symbol_table)
    demangled_names = demangle((item[0] for item in raw_functions), cxxfilt)

    functions: list[Function] = []
    for (name, symbol_type, address, size), demangled_name in zip(
        raw_functions, demangled_names, strict=True
    ):
        prov = provenance(address)
        functions.append(
            Function(
                address=address,
                size=size,
                symbol_type=symbol_type,
                mangled=name,
                demangled=demangled_name,
                owner=owner_of(demangled_name),
                provenance=prov,
                recovery_kind=recovery_kind(prov, name, demangled_name),
                decompiled=address in decompiled_by_address,
            )
        )
    functions.sort(key=lambda item: (item.address, item.mangled))

    units = parse_file_symbols(symbol_table)
    object_symbols = parse_readelf_objects(symbol_table)
    binary_bytes = binary.read_bytes()
    metaobjects = parse_metaobjects(binary_bytes, object_symbols)
    dependencies = NEEDED.findall(elf_headers)
    build_id_match = BUILD_ID.search(elf_headers)
    build_id = build_id_match.group("id") if build_id_match else "unknown"

    counts_by_provenance = Counter(item.provenance for item in functions)
    counts_by_kind = Counter(item.recovery_kind for item in functions)
    owner_counts = Counter(
        item.owner
        for item in functions
        if item.recovery_kind == "manual_source_candidate"
    )
    unit_counts = Counter(
        unit_group(index, name) for index, name in enumerate(units, start=1)
    )
    unique_function_addresses = {item.address for item in functions}
    decompiled_defined_addresses = {
        item.address for item in functions if item.decompiled
    }
    moc_method_count = sum(
        int(item.get("method_count", 0)) for item in metaobjects
    )
    moc_signal_count = sum(
        int(item.get("signal_count", 0)) for item in metaobjects
    )

    inventory = {
        "schema_version": 1,
        "artifacts": {
            "binary": {
                "path": str(binary),
                "sha256": sha256(binary),
                "build_id": build_id,
                "size_bytes": binary.stat().st_size,
            },
            "decompilation": {
                "path": str(decompilation),
                "sha256": sha256(decompilation),
                "size_bytes": decompilation.stat().st_size,
                "line_count": decompilation_lines,
                "function_marker_count": len(decompiled_by_address),
            },
        },
        "dynamic_dependencies": dependencies,
        "compilation_units": [
            {"index": index, "name": name, "group": unit_group(index, name)}
            for index, name in enumerate(units, start=1)
        ],
        "counts": {
            "defined_function_symbols": len(functions),
            "defined_function_rvas": len(unique_function_addresses),
            "decompiled_defined_rvas": len(decompiled_defined_addresses),
            "by_provenance": dict(sorted(counts_by_provenance.items())),
            "by_recovery_kind": dict(sorted(counts_by_kind.items())),
            "compilation_units_by_group": dict(sorted(unit_counts.items())),
            "metaobjects": len(metaobjects),
            "metaobject_methods": moc_method_count,
            "metaobject_signals": moc_signal_count,
        },
        "functions": [asdict(item) for item in functions],
    }
    with (output / "inventory.json").open("w", encoding="utf-8") as stream:
        json.dump(inventory, stream, indent=2, sort_keys=True)
        stream.write("\n")
    write_tsv(output / "functions.tsv", functions)
    with (output / "metaobjects.json").open("w", encoding="utf-8") as stream:
        json.dump(metaobjects, stream, indent=2, sort_keys=True)
        stream.write("\n")

    with (output / "moc_interfaces.md").open("w", encoding="utf-8") as stream:
        stream.write("# Recovered Qt meta-object interfaces\n\n")
        stream.write(
            f"Recovered {len(metaobjects)} meta-objects and {moc_method_count} "
            f"methods, including {moc_signal_count} signals.\n\n"
        )
        for metaobject in metaobjects:
            stream.write(f"## `{metaobject['class_name']}`\n\n")
            properties = metaobject.get("properties", [])
            if properties:
                stream.write("Properties:\n\n")
                stream.write("| Name | Type | Flags |\n|---|---|---:|\n")
                for prop in properties:
                    stream.write(
                        f"| `{prop['name']}` | `{prop['type']}` | "
                        f"`0x{prop['flags']:x}` |\n"
                    )
                stream.write("\n")
            enums = metaobject.get("enums", [])
            if enums:
                stream.write("Enumerations:\n\n")
                for enum in enums:
                    values = ", ".join(
                        f"{value['name']}={value['value']}" for value in enum["values"]
                    )
                    stream.write(
                        f"- `{enum['name']}` (`0x{enum['flags']:x}`): `{values}`\n"
                    )
                stream.write("\n")
            methods = metaobject.get("methods", [])
            if not methods:
                stream.write("No class-local signals, slots, or invokable methods.\n\n")
                continue
            stream.write("| Kind | Access | Signature | Flags |\n|---|---|---|---:|\n")
            for method in methods:
                arguments = ", ".join(
                    f"{argument_type} {argument_name}".rstrip()
                    for argument_type, argument_name in zip(
                        method["argument_types"],
                        method["argument_names"],
                        strict=True,
                    )
                )
                signature = (
                    f"{method['return_type']} {method['name']}({arguments})"
                ).replace("|", "\\|")
                stream.write(
                    f"| {method['kind']} | {method['access']} | `{signature}` | "
                    f"`0x{method['flags']:x}` |\n"
                )
            stream.write("\n")

    with (output / "compilation_units.tsv").open(
        "w", encoding="utf-8", newline=""
    ) as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(["index", "group", "name"])
        for index, name in enumerate(units, start=1):
            writer.writerow([index, unit_group(index, name), name])

    with (output / "summary.md").open("w", encoding="utf-8") as stream:
        stream.write("# Component inventory summary\n\n")
        stream.write(
            "This public summary reports architectural counts and component "
            "coverage.\n\n"
        )
        stream.write(
            f"- Application source concepts: {unit_counts.get('application', 0):,}\n"
        )
        stream.write(
            f"- Qt meta-object generation concepts: {unit_counts.get('application_moc', 0):,}\n"
        )
        stream.write(
            f"- Vendored Livox SDK source concepts: {unit_counts.get('livox_sdk', 0):,}\n"
        )
        stream.write(
            f"- Supporting runtime/toolchain concepts: {unit_counts.get('toolchain', 0):,}\n"
        )
        stream.write(f"- Qt meta-objects: {len(metaobjects):,}\n")
        stream.write(f"- Qt meta-object methods: {moc_method_count:,}\n")
        stream.write(f"- Qt signals: {moc_signal_count:,}\n\n")
        stream.write("## Largest application class surfaces\n\n")
        stream.write("| Class | Recovered function concepts |\n|---|---:|\n")
        for owner, count in owner_counts.most_common(50):
            stream.write(f"| `{owner}` | {count} |\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
