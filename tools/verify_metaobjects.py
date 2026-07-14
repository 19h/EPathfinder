#!/usr/bin/env python3
"""Compare recovered runtime Qt meta-objects with the ELF-derived inventory."""

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


def expected_method(method: dict, qt6_vector_alias: bool) -> tuple[str, str, str, str]:
    signature = f"{method['name']}({','.join(method['argument_types'])})"
    if qt6_vector_alias:
        # QVector is a QList alias in Qt 6, so runtime QMetaMethod strings use
        # QList even when the recovered header deliberately spells QVector as
        # the original Qt 5 MOC did.
        signature = signature.replace("QVector<", "QList<")
    return signature, method["kind"], method["access"], method["return_type"]


def actual_method(method: dict) -> tuple[str, str, str, str]:
    return (
        method["signature"],
        method["kind"],
        method["access"],
        method["return_type"],
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", type=Path, required=True)
    parser.add_argument("--dump", type=Path, required=True)
    args = parser.parse_args()

    expected = json.loads(args.inventory.read_text())
    actual = json.loads(subprocess.check_output([str(args.dump)], text=True))
    expected_by_class = {item["class_name"]: item for item in expected}
    actual_by_class = {item["class_name"]: item for item in actual}
    qt6_vector_alias = any(
        "QList<RoadElement>" in method["signature"]
        for item in actual
        for method in item["methods"]
    )

    failures: list[str] = []
    if set(expected_by_class) != set(actual_by_class):
        failures.append(
            "class set differs: missing="
            f"{sorted(set(expected_by_class) - set(actual_by_class))}, extra="
            f"{sorted(set(actual_by_class) - set(expected_by_class))}"
        )

    for name in sorted(set(expected_by_class) & set(actual_by_class)):
        expected_methods = [expected_method(item, qt6_vector_alias)
                            for item in expected_by_class[name]["methods"]]
        actual_methods = [actual_method(item)
                          for item in actual_by_class[name]["methods"]]
        if expected_methods != actual_methods:
            failures.append(
                f"{name}: expected {len(expected_methods)} methods, "
                f"actual {len(actual_methods)}\n"
                f"  expected={expected_methods}\n  actual={actual_methods}"
            )

    if failures:
        print("\n".join(failures))
        return 1
    method_count = sum(len(item["methods"]) for item in actual)
    suffix = " (Qt 6 QVector/QList alias normalized)" if qt6_vector_alias else ""
    print(f"exact meta-object match: {len(actual)} classes, {method_count} methods{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
