#!/usr/bin/env python3
"""
Extract formal theorem/definition identifiers from LRA LaTeX notes.

This is a deterministic catalog pass for standardization prompts and
relationship extraction.  It does not modify source files.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Iterable


FORMAL_ENVS = (
    "definition",
    "theorem",
    "lemma",
    "proposition",
    "corollary",
    "axiom",
    "assumption",
    "consequence",
)

FORMAL_PATTERN = re.compile(
    r"\\begin\{("
    + "|".join(FORMAL_ENVS)
    + r")\*?\}(?:\[([^\]]*)\])?(.*?)(?=\\end\{\1\*?\})\\end\{\1\*?\}",
    re.S,
)
LABEL_PATTERN = re.compile(r"\\label\{([^}]+)\}")


def find_repo_root(start: Path) -> Path:
    current = start.resolve()
    if current.is_file():
        current = current.parent
    for path in [current, *current.parents]:
        if (path / "DESIGN.md").exists():
            return path
    raise SystemExit("Could not find repository root containing DESIGN.md")


def iter_tex_files(target: Path) -> Iterable[Path]:
    if target.is_file() and target.suffix == ".tex":
        yield target
        return
    if target.is_dir():
        for path in sorted(target.rglob("*.tex")):
            if path.name.endswith((".old.tex", ".tmp.tex")):
                continue
            yield path


def line_of(text: str, pos: int) -> int:
    return text.count("\n", 0, pos) + 1


def extract_catalog(files: list[Path], root: Path) -> dict:
    items = []
    duplicate_labels: dict[str, list[dict]] = {}
    by_label: dict[str, dict] = {}
    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        for match in FORMAL_PATTERN.finditer(text):
            body = match.group(3)
            label_match = LABEL_PATTERN.search(body)
            label = label_match.group(1) if label_match else ""
            item = {
                "label": label,
                "env": match.group(1),
                "title": (match.group(2) or "").strip(),
                "file": str(path.relative_to(root)).replace("\\", "/"),
                "line": line_of(text, match.start()),
            }
            items.append(item)
            if label:
                if label in by_label:
                    duplicate_labels.setdefault(label, [by_label[label]]).append(item)
                else:
                    by_label[label] = item
    return {
        "schema_version": 1,
        "item_count": len(items),
        "labeled_item_count": sum(1 for item in items if item["label"]),
        "duplicate_labels": duplicate_labels,
        "items": sorted(items, key=lambda item: (item["file"], item["line"], item["label"])),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract theorem/definition-style labels from .tex notes.")
    parser.add_argument("target", help="A .tex file or directory to scan.")
    parser.add_argument("--output", help="Optional JSON output path.")
    args = parser.parse_args()

    target = Path(args.target)
    if not target.is_absolute():
        target = (Path.cwd() / target).resolve()
    root = find_repo_root(target if target.exists() else Path.cwd())
    files = list(iter_tex_files(target))
    if not files:
        raise SystemExit(f"No .tex files found under {target}")
    catalog = extract_catalog(files, root)

    text = json.dumps(catalog, indent=2, ensure_ascii=False)
    if args.output:
        output = Path(args.output)
        if not output.is_absolute():
            output = (Path.cwd() / output).resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text, encoding="utf-8")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
