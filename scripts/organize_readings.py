#!/usr/bin/env python3
"""
organize_readings.py - Analyze and cautiously rename PDFs in a reading folder.

This script is designed for a local Ollama workflow. It:

1. Walks a reading folder looking for PDFs.
2. Extracts metadata and a text sample from each PDF.
3. Asks a local Ollama model for structured JSON metadata.
4. Writes a catalog JSON file plus a Markdown summary.
5. Optionally renames files when the metadata is confident enough.

Default behavior is dry-run: it analyzes and reports, but does not rename.

Examples:
    python scripts/organize_readings.py
    python scripts/organize_readings.py --root D:\\Readings --apply-renames
    python scripts/organize_readings.py --apply-from-catalog
    python scripts/organize_readings.py --model qwen2.5:14b --max-files 25

Requirements:
    pip install ollama pypdf
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import textwrap
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

try:
    from ollama import chat
except ImportError:  # pragma: no cover - import guard
    chat = None

try:
    from pypdf import PdfReader
except ImportError:  # pragma: no cover - import guard
    PdfReader = None


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ROOT = Path(r"D:\Readings")
DEFAULT_CATALOG = REPO_ROOT / "reports" / "reading-catalog.json"
DEFAULT_SUMMARY = REPO_ROOT / "reports" / "reading-catalog.md"
MAX_TITLE_LEN = 140
MAX_AUTHOR_LEN = 80
INVALID_FILENAME_CHARS = r'[<>:"/\\|?*]'
LECTURE_NOTE_KINDS = {
    "lecture_notes",
    "course_notes",
    "notes",
    "slides",
    "handout",
    "worksheet",
    "draft_notes",
    "unpublished_notes",
    "unknown",
}


@dataclass
class ReadingRecord:
    path: str
    original_name: str
    title: str
    edition: str | None
    authors: list[str]
    kind: str
    primary_topic: str
    secondary_topics: list[str]
    level: str
    prerequisites: list[str]
    summary: str
    recommended_for: str
    confidence: float
    rename_recommended: bool
    rename_reason: str
    rename_target: str | None
    rename_applied: bool
    metadata_sources: dict[str, str]
    notes: list[str]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT, help="Folder to scan for PDFs.")
    parser.add_argument("--model", default="qwen2.5:14b", help="Local Ollama model name.")
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG, help="Output JSON catalog.")
    parser.add_argument("--summary", type=Path, default=DEFAULT_SUMMARY, help="Output Markdown summary.")
    parser.add_argument("--max-files", type=int, default=None, help="Only process the first N PDFs.")
    parser.add_argument("--max-pages", type=int, default=8, help="Max pages to sample from each PDF.")
    parser.add_argument("--max-chars", type=int, default=12000, help="Max extracted characters sent to the model.")
    parser.add_argument(
        "--apply-renames",
        action="store_true",
        help="Actually rename files. Default is dry-run only.",
    )
    parser.add_argument(
        "--apply-from-catalog",
        action="store_true",
        help="Apply rename targets already stored in the JSON catalog, without re-running PDF extraction or Ollama.",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Allow overwrite-style renames only when the target filename does not already exist. "
        "This flag does not bypass that safety check; it only suppresses some conservative skips.",
    )
    return parser.parse_args()


def require_dependencies() -> None:
    missing = []
    if chat is None:
        missing.append("ollama")
    if PdfReader is None:
        missing.append("pypdf")
    if missing:
        joined = ", ".join(missing)
        raise SystemExit(
            f"Missing Python package(s): {joined}\n"
            f"Install with: pip install {' '.join(missing)}"
        )


def load_catalog_records(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        raise SystemExit(f"Catalog file does not exist: {path}")
    payload = json.loads(path.read_text(encoding="utf-8"))
    records = payload.get("records", [])
    if not isinstance(records, list):
        raise SystemExit(f"Catalog file is malformed: {path}")
    return records


def find_pdfs(root: Path, max_files: int | None) -> list[Path]:
    pdfs = sorted(p for p in root.rglob("*.pdf") if p.is_file())
    if max_files is not None:
        return pdfs[:max_files]
    return pdfs


def collapse_ws(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def stringify_note(value: Any) -> str:
    if isinstance(value, str):
        return collapse_ws(value)
    if isinstance(value, (int, float, bool)) or value is None:
        return collapse_ws(str(value))
    if isinstance(value, list):
        parts = [stringify_note(item) for item in value]
        parts = [part for part in parts if part]
        return collapse_ws("; ".join(parts))
    if isinstance(value, dict):
        try:
            return collapse_ws(json.dumps(value, ensure_ascii=False, sort_keys=True))
        except Exception:
            return collapse_ws(str(value))
    return collapse_ws(str(value))


def normalize_string_list(values: Any) -> list[str]:
    if values is None:
        return []
    if not isinstance(values, list):
        values = [values]
    normalized = [stringify_note(value) for value in values]
    return [value for value in normalized if value]


def extract_pdf_payload(path: Path, max_pages: int, max_chars: int) -> dict[str, Any]:
    reader = PdfReader(str(path))
    metadata = reader.metadata or {}
    pages = []
    for page in reader.pages[:max_pages]:
        try:
            text = page.extract_text() or ""
        except Exception:
            text = ""
        if text:
            pages.append(text)
        joined = "\n\n".join(pages)
        if len(joined) >= max_chars:
            break

    text_sample = "\n\n".join(pages)[:max_chars]
    return {
        "path": str(path),
        "filename": path.name,
        "metadata": {
            "title": collapse_ws(str(metadata.get("/Title", "") or "")),
            "author": collapse_ws(str(metadata.get("/Author", "") or "")),
            "subject": collapse_ws(str(metadata.get("/Subject", "") or "")),
            "creator": collapse_ws(str(metadata.get("/Creator", "") or "")),
            "producer": collapse_ws(str(metadata.get("/Producer", "") or "")),
        },
        "text_sample": text_sample,
    }


def build_prompt(payload: dict[str, Any]) -> str:
    schema = {
        "title": "Canonical book or document title, or empty string if uncertain",
        "edition": "Edition text such as '2nd Edition', or empty string if absent/uncertain",
        "authors": ["List of author names, or empty list if uncertain"],
        "kind": "One of: textbook, monograph, reference, lecture_notes, course_notes, paper, slides, handout, draft_notes, unknown",
        "primary_topic": "Short topic slug such as real-analysis, numerical-analysis, probability, stochastic-calculus, programming-systems, linear-algebra",
        "secondary_topics": ["Up to 4 shorter topic slugs"],
        "level": "One of: beginner, intermediate, advanced-undergraduate, graduate, research, unknown",
        "prerequisites": ["Up to 6 short prerequisite topics"],
        "summary": "1-3 sentences",
        "recommended_for": "Short phrase naming likely use",
        "confidence": "Number from 0.0 to 1.0",
        "rename_recommended": "true only if title and author metadata are strong enough for a conservative rename",
        "rename_reason": "Short explanation",
        "metadata_sources": {
            "title": "pdf_metadata | extracted_text | filename | unknown",
            "edition": "pdf_metadata | extracted_text | filename | unknown",
            "authors": "pdf_metadata | extracted_text | filename | unknown",
        },
        "notes": ["Optional cautions or ambiguities"],
    }
    return textwrap.dedent(
        f"""
        You are organizing a mathematics reading library.
        Read the PDF metadata and text sample and return STRICT JSON only.

        Conservative renaming policy:
        - rename_recommended must be false for lecture notes, course notes, slides, handouts, draft notes, or unknown documents
        - rename_recommended must be false if title or author is uncertain
        - if edition is missing or uncertain, return an empty string for edition
        - do not invent edition data
        - do not invent author names
        - prefer extracted text and PDF metadata over filename guesses

        JSON schema guidance:
        {json.dumps(schema, indent=2)}

        PDF payload:
        {json.dumps(payload, ensure_ascii=False, indent=2)}
        """
    ).strip()


def call_ollama(model: str, payload: dict[str, Any]) -> dict[str, Any]:
    response = chat(
        model=model,
        messages=[
            {
                "role": "user",
                "content": build_prompt(payload),
            }
        ],
        format="json",
    )
    raw = response["message"]["content"]
    return json.loads(raw)


def sanitize_filename_part(text: str, max_len: int) -> str:
    text = collapse_ws(text)
    text = text.replace("&", "and")
    text = re.sub(INVALID_FILENAME_CHARS, "", text)
    text = re.sub(r"\s+", " ", text).strip(" .-")
    return text[:max_len].rstrip(" .-")


def compact_title(title: str) -> str:
    title = collapse_ws(title)
    if not title:
        return ""
    title = re.split(r"\s*[:\-]\s*", title, maxsplit=1)[0]
    title = re.split(r"\s*\(\s*", title, maxsplit=1)[0]
    title = re.sub(r"\b(Second|Third|Fourth|Fifth|Sixth|Seventh|Eighth|Ninth|Tenth)\s+Edition\b", "", title, flags=re.IGNORECASE)
    title = re.sub(r"\b\d+(st|nd|rd|th)\s+Edition\b", "", title, flags=re.IGNORECASE)
    title = collapse_ws(title)
    return title


def compact_edition(edition: str) -> str:
    edition = collapse_ws(edition)
    if not edition:
        return ""
    lowered = edition.lower()
    digit_match = re.search(r"\b(\d+)(?:st|nd|rd|th)?\b", lowered)
    if digit_match:
        return f"e{digit_match.group(1)}"
    word_to_num = {
        "first": "1",
        "second": "2",
        "third": "3",
        "fourth": "4",
        "fifth": "5",
        "sixth": "6",
        "seventh": "7",
        "eighth": "8",
        "ninth": "9",
        "tenth": "10",
    }
    for word, number in word_to_num.items():
        if word in lowered:
            return f"e{number}"
    return ""


def normalize_author_segment(authors: list[str]) -> str:
    clean = [collapse_ws(a) for a in authors if collapse_ws(a)]
    if not clean:
        return ""
    def last_name(name: str) -> str:
        bits = [bit for bit in re.split(r"\s+", name) if bit]
        return bits[-1] if bits else name
    if len(clean) == 1:
        return last_name(clean[0])
    first = clean[0]
    surname = last_name(first)
    return surname


def should_skip_rename(data: dict[str, Any]) -> tuple[bool, list[str]]:
    reasons = []
    kind = collapse_ws(str(data.get("kind", "") or "")).lower()
    title = collapse_ws(str(data.get("title", "") or ""))
    edition = collapse_ws(str(data.get("edition", "") or ""))
    authors = data.get("authors", []) or []
    confidence = float(data.get("confidence", 0.0) or 0.0)
    rename_recommended = bool(data.get("rename_recommended", False))

    if kind in LECTURE_NOTE_KINDS:
        reasons.append(f"kind={kind}")
    if not rename_recommended:
        reasons.append("model_did_not_recommend_rename")
    if confidence < 0.78:
        reasons.append(f"low_confidence={confidence:.2f}")
    if not title:
        reasons.append("missing_title")
    if not authors:
        reasons.append("missing_authors")
    if edition and len(edition) > 40:
        reasons.append("edition_looks_unreliable")

    title_src = (data.get("metadata_sources", {}) or {}).get("title", "unknown")
    author_src = (data.get("metadata_sources", {}) or {}).get("authors", "unknown")
    if title_src == "filename":
        reasons.append("title_from_filename_only")
    if author_src == "filename":
        reasons.append("authors_from_filename_only")

    return (len(reasons) > 0, reasons)


def build_target_filename(data: dict[str, Any]) -> str | None:
    title = sanitize_filename_part(compact_title(str(data.get("title", "") or "")), MAX_TITLE_LEN)
    edition = sanitize_filename_part(compact_edition(str(data.get("edition", "") or "")), 10)
    author = sanitize_filename_part(normalize_author_segment(data.get("authors", []) or []), MAX_AUTHOR_LEN)
    if not title or not author:
        return None
    pieces = [title]
    if edition:
        pieces[0] = f"{pieces[0]} {edition}"
    return f"{pieces[0]} - {author}.pdf"


def safe_rename(path: Path, target_name: str, force: bool) -> tuple[bool, str]:
    target = path.with_name(target_name)
    if target == path:
        return (False, "already_named")
    if target.exists():
        return (False, "target_exists")
    if not force and path.stem.startswith("."):
        return (False, "hidden_like_name")
    path.rename(target)
    return (True, "renamed")


def apply_from_catalog(catalog_path: Path, root: Path, force: bool) -> int:
    records = load_catalog_records(catalog_path)
    planned = 0
    applied = 0
    skipped = 0

    print(f"Applying renames from catalog: {catalog_path}")
    print(f"Within root: {root}")
    print()

    for entry in records:
        original_path = Path(str(entry.get("path", "") or ""))
        rename_target = entry.get("rename_target")
        if not rename_target:
            continue
        planned += 1

        try:
            resolved = original_path.resolve()
        except Exception:
            resolved = original_path

        if root not in resolved.parents and resolved != root:
            print(f"SKIP  {original_path.name} -> {rename_target} [outside_root]")
            skipped += 1
            continue

        if not resolved.exists():
            print(f"SKIP  {original_path.name} -> {rename_target} [missing_source]")
            skipped += 1
            continue

        renamed, reason = safe_rename(resolved, str(rename_target), force)
        if renamed:
            print(f"OK    {resolved.name} -> {rename_target}")
            applied += 1
        else:
            print(f"SKIP  {resolved.name} -> {rename_target} [{reason}]")
            skipped += 1

    print()
    print(f"Rename targets in catalog: {planned}")
    print(f"Renames applied:          {applied}")
    print(f"Renames skipped:          {skipped}")
    return 0


def to_record(path: Path, data: dict[str, Any], rename_target: str | None, rename_applied: bool, notes: list[str]) -> ReadingRecord:
    metadata_sources = data.get("metadata_sources", {}) or {}
    model_notes = normalize_string_list(data.get("notes", []))
    return ReadingRecord(
        path=str(path),
        original_name=path.name,
        title=collapse_ws(str(data.get("title", "") or "")),
        edition=collapse_ws(str(data.get("edition", "") or "")) or None,
        authors=normalize_string_list(data.get("authors", [])),
        kind=collapse_ws(str(data.get("kind", "") or "")),
        primary_topic=collapse_ws(str(data.get("primary_topic", "") or "")),
        secondary_topics=normalize_string_list(data.get("secondary_topics", [])),
        level=collapse_ws(str(data.get("level", "") or "")),
        prerequisites=normalize_string_list(data.get("prerequisites", [])),
        summary=collapse_ws(str(data.get("summary", "") or "")),
        recommended_for=collapse_ws(str(data.get("recommended_for", "") or "")),
        confidence=float(data.get("confidence", 0.0) or 0.0),
        rename_recommended=bool(data.get("rename_recommended", False)),
        rename_reason=collapse_ws(str(data.get("rename_reason", "") or "")),
        rename_target=rename_target,
        rename_applied=rename_applied,
        metadata_sources={
            "title": collapse_ws(str(metadata_sources.get("title", "unknown"))),
            "edition": collapse_ws(str(metadata_sources.get("edition", "unknown"))),
            "authors": collapse_ws(str(metadata_sources.get("authors", "unknown"))),
        },
        notes=notes + model_notes,
    )


def write_json(path: Path, records: list[ReadingRecord]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "generated_at_epoch": int(time.time()),
        "records": [asdict(record) for record in records],
    }
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")


def write_markdown(path: Path, records: list[ReadingRecord], root: Path, applied: bool) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Reading Catalog",
        "",
        f"- Root: `{root}`",
        f"- Files analyzed: `{len(records)}`",
        f"- Renames applied: `{'yes' if applied else 'no (dry-run)'}`",
        "",
    ]
    by_topic: dict[str, list[ReadingRecord]] = {}
    for record in records:
        by_topic.setdefault(record.primary_topic or "unknown", []).append(record)

    for topic in sorted(by_topic):
        lines.append(f"## {topic}")
        lines.append("")
        for record in sorted(by_topic[topic], key=lambda r: r.title.lower() or r.original_name.lower()):
            rename_line = record.rename_target or "skip"
            status = "applied" if record.rename_applied else "planned"
            lines.append(f"- **{record.title or record.original_name}**")
            lines.append(f"  - File: `{record.original_name}`")
            lines.append(f"  - Kind: `{record.kind or 'unknown'}` | Level: `{record.level or 'unknown'}` | Confidence: `{record.confidence:.2f}`")
            lines.append(f"  - Authors: `{', '.join(record.authors) or 'unknown'}`")
            lines.append(f"  - Rename: `{rename_line}` ({status if record.rename_target else 'skipped'})")
            if record.summary:
                lines.append(f"  - Summary: {record.summary}")
            if record.notes:
                lines.append(f"  - Notes: {'; '.join(record.notes)}")
            lines.append("")
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    if not root.exists():
        raise SystemExit(f"Reading folder does not exist: {root}")

    if args.apply_from_catalog:
        return apply_from_catalog(args.catalog, root, args.force)

    require_dependencies()

    pdfs = find_pdfs(root, args.max_files)
    if not pdfs:
        print(f"No PDFs found under {root}")
        return 0

    print(f"Scanning {len(pdfs)} PDF(s) under {root}")
    print(f"Using local model: {args.model}")
    print(f"Rename mode: {'APPLY' if args.apply_renames else 'DRY-RUN'}")
    print()

    records: list[ReadingRecord] = []
    for index, pdf_path in enumerate(pdfs, start=1):
        print(f"[{index}/{len(pdfs)}] {pdf_path.name}")
        notes: list[str] = []
        rename_target = None
        rename_applied = False
        try:
            payload = extract_pdf_payload(pdf_path, args.max_pages, args.max_chars)
            if not payload["text_sample"] and not any(payload["metadata"].values()):
                notes.append("no_extractable_text_or_metadata")
            data = call_ollama(args.model, payload)
            skip_rename, skip_reasons = should_skip_rename(data)
            notes.extend(skip_reasons)
            if not skip_rename:
                rename_target = build_target_filename(data)
                if not rename_target:
                    notes.append("could_not_build_target_name")
                elif args.apply_renames:
                    rename_applied, reason = safe_rename(pdf_path, rename_target, args.force)
                    if not rename_applied:
                        notes.append(reason)
                else:
                    notes.append("dry_run")
        except Exception as exc:
            data = {
                "title": "",
                "edition": "",
                "authors": [],
                "kind": "unknown",
                "primary_topic": "unknown",
                "secondary_topics": [],
                "level": "unknown",
                "prerequisites": [],
                "summary": "",
                "recommended_for": "",
                "confidence": 0.0,
                "rename_recommended": False,
                "rename_reason": "",
                "metadata_sources": {},
                "notes": [],
            }
            notes.append(f"error={type(exc).__name__}: {exc}")

        record = to_record(pdf_path, data, rename_target, rename_applied, notes)
        records.append(record)

    write_json(args.catalog, records)
    write_markdown(args.summary, records, root, args.apply_renames)

    print()
    print(f"Wrote JSON catalog: {args.catalog}")
    print(f"Wrote summary:      {args.summary}")
    renamed = sum(1 for r in records if r.rename_applied)
    planned = sum(1 for r in records if r.rename_target)
    print(f"Rename targets:     {planned}")
    print(f"Renames applied:    {renamed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
