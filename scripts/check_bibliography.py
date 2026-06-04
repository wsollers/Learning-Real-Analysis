#!/usr/bin/env python3
"""Check volume BibTeX files for duplicate keys and likely duplicate works."""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path


ENTRY_RE = re.compile(r"@(?P<type>[A-Za-z]+)\s*\{\s*(?P<key>[^,\s]+)\s*,", re.MULTILINE)
FIELD_RE = re.compile(r"(?P<name>[A-Za-z]+)\s*=\s*(?P<value>\{(?:[^{}]|\{[^{}]*\})*\}|\"[^\"]*\")", re.DOTALL)


def entry_spans(text: str):
    matches = list(ENTRY_RE.finditer(text))
    for index, match in enumerate(matches):
        start = match.start()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        yield match.group("type"), match.group("key"), text[start:end]


def clean_value(value: str) -> str:
    value = value.strip()
    if (value.startswith("{") and value.endswith("}")) or (value.startswith('"') and value.endswith('"')):
        value = value[1:-1]
    value = re.sub(r"\\[A-Za-z]+", " ", value)
    value = re.sub(r"[{}\\\"'`~^]", "", value)
    value = re.sub(r"\s+", " ", value)
    return value.strip()


def normalize(value: str) -> str:
    value = clean_value(value).lower()
    value = re.sub(r"[^a-z0-9]+", " ", value)
    return re.sub(r"\s+", " ", value).strip()


def parse_fields(body: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for match in FIELD_RE.finditer(body):
        fields[match.group("name").lower()] = clean_value(match.group("value"))
    return fields


def load_entries(bib_dir: Path):
    entries = []
    for path in sorted(bib_dir.glob("*.bib")):
        if path.name == "analysis.bib":
            continue
        text = path.read_text(encoding="utf-8")
        for entry_type, key, body in entry_spans(text):
            fields = parse_fields(body)
            entries.append(
                {
                    "file": path,
                    "type": entry_type,
                    "key": key,
                    "fields": fields,
                    "body": body,
                }
            )
    return entries


def main() -> int:
    parser = argparse.ArgumentParser(description="Check a volume-owned LRA bibliography shard.")
    parser.add_argument("--bib-dir", default="bibliography", help="Directory containing .bib files.")
    parser.add_argument("--find", help="Case-insensitive search over keys, titles, authors, years, and bodies.")
    args = parser.parse_args()

    bib_dir = Path(args.bib_dir)
    entries = load_entries(bib_dir)

    if args.find:
        needle = args.find.lower()
        for entry in entries:
            fields = entry["fields"]
            haystack = " ".join(
                [
                    entry["key"],
                    fields.get("title", ""),
                    fields.get("author", ""),
                    fields.get("editor", ""),
                    fields.get("year", ""),
                    entry["body"],
                ]
            ).lower()
            if needle in haystack:
                print(f"{entry['key']} [{entry['file']}]")
                title = fields.get("title")
                author = fields.get("author") or fields.get("editor")
                year = fields.get("year")
                if author:
                    print(f"  author/editor: {author}")
                if title:
                    print(f"  title: {title}")
                if year:
                    print(f"  year: {year}")
        return 0

    by_key = defaultdict(list)
    by_work = defaultdict(list)
    for entry in entries:
        fields = entry["fields"]
        by_key[entry["key"]].append(entry)
        title = normalize(fields.get("title", ""))
        creator = normalize(fields.get("author", fields.get("editor", "")))
        year = normalize(fields.get("year", ""))
        if title and creator:
            by_work[(title, creator, year)].append(entry)

    failures = 0
    for key, matches in sorted(by_key.items()):
        if len(matches) > 1:
            failures += 1
            print(f"Duplicate key: {key}")
            for match in matches:
                print(f"  {match['file']}")

    likely_duplicates = [(work, matches) for work, matches in by_work.items() if len(matches) > 1]
    if likely_duplicates:
        print("\nLikely duplicate works:")
        for (_, _, _), matches in sorted(likely_duplicates, key=lambda item: item[1][0]["key"].lower()):
            keys = ", ".join(match["key"] for match in matches)
            files = ", ".join(str(match["file"]) for match in matches)
            print(f"  {keys}")
            print(f"    {files}")

    print(f"\nChecked {len(entries)} entries in {bib_dir}.")
    if failures:
        print(f"Failed: {failures} duplicate key group(s).")
        return 1
    print("No duplicate BibTeX keys found.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
