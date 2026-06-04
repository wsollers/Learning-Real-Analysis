#!/usr/bin/env python3
"""Validate LRA Anki flashcard CSV files."""

from __future__ import annotations

import csv
import re
import sys
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXPECTED_HEADERS = ["Front", "Back", "Tags"]
MATH_DELIM = ""
RULE_IDENTIFICATION_FILES = {
    ROOT / "derivatives" / "derivative-rule-identification.csv",
    ROOT / "integrals" / "integral-rule-identification.csv",
    ROOT / "odes" / "ode-rule-identification.csv",
}
CSV_FILES = sorted(
    path
    for path in ROOT.rglob("*.csv")
    if ".git" not in path.parts and path.is_file()
)
MATH_OUTSIDE_PATTERN = re.compile(r"(\\|[_^{}=]|(?<![A-Za-z])\d+(?![A-Za-z]))")

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def strip_math_spans(value: str) -> tuple[str, bool]:
    parts = value.split(MATH_DELIM)
    if len(parts) % 2 == 0:
        return value, False
    outside = "".join(parts[0::2])
    return outside, True


def math_is_wrapped(value: str) -> bool:
    outside, balanced = strip_math_spans(value)
    if not balanced:
        return False
    return MATH_OUTSIDE_PATTERN.search(outside) is None


def validate_file(path: Path) -> tuple[int, list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []

    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames != EXPECTED_HEADERS:
            errors.append(
                f"{relative(path)}: expected headers {EXPECTED_HEADERS}, got {reader.fieldnames}"
            )
            return 0, errors, warnings

        rows = list(reader)

    for index, row in enumerate(rows, start=2):
        for field in ("Front", "Back"):
            value = row[field].strip()
            if not value:
                errors.append(f"{relative(path)}:{index}: blank {field}")
            elif not math_is_wrapped(value):
                errors.append(
                    f"{relative(path)}:{index}: mathematical content is not fully wrapped in {MATH_DELIM}...{MATH_DELIM}: {field}={value!r}"
                )

    front_counts = Counter(row["Front"] for row in rows)
    for front, count in sorted(front_counts.items()):
        if count > 1:
            warnings.append(
                f"{relative(path)}: duplicate Front appears {count} times: {front!r}"
            )

    if path not in RULE_IDENTIFICATION_FILES:
        pairs = Counter((row["Front"], row["Back"]) for row in rows)
        for front, back in list(pairs):
            if pairs[(back, front)] < pairs[(front, back)]:
                errors.append(
                    f"{relative(path)}: missing reverse card for Front={front!r}, Back={back!r}"
                )

    return len(rows), errors, warnings


def main() -> int:
    if not CSV_FILES:
        print("No CSV files found.")
        return 1

    all_errors: list[str] = []
    all_warnings: list[str] = []

    print("Flashcard counts:")
    for path in CSV_FILES:
        count, errors, warnings = validate_file(path)
        print(f"- {relative(path)}: {count}")
        all_errors.extend(errors)
        all_warnings.extend(warnings)

    if all_warnings:
        print("\nWarnings:")
        for warning in all_warnings:
            print(f"- {warning}")

    if all_errors:
        print("\nErrors:")
        for error in all_errors:
            print(f"- {error}")
        return 1

    print("\nValidation passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
