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

try:
    import fitz
except ImportError:  # pragma: no cover - import guard
    fitz = None


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ROOT = Path(r"D:\Readings")
DEFAULT_CATALOG = REPO_ROOT / "reports" / "reading-catalog.json"
DEFAULT_SUMMARY = REPO_ROOT / "reports" / "reading-catalog.md"
MAX_TITLE_LEN = 140
MAX_AUTHOR_LEN = 80
INVALID_FILENAME_CHARS = r'[<>:"/\\|?*]'
SUSPICIOUS_FILENAME_PATTERNS = (
    "bok%3a",
    "preview-",
    "mbk-",
    "lecturenotes",
    "intro",
    "numbers",
)
COMMON_TITLE_WORDS = {
    "analysis",
    "algebra",
    "topology",
    "geometry",
    "integral",
    "measure",
    "theory",
    "calculus",
    "probability",
    "statistics",
    "methods",
    "introduction",
}
TITLE_BLACKLIST_TOKENS = (
    ".pdf",
    ".indd",
    "printpdf",
    "book_printpdf",
    "department of mathematics",
    "faculty of",
    "autumn term",
    "spring term",
    "winter term",
    "summer term",
    "math 5",
    "math 4",
    "lecture notes",
    "course notes",
    "uni00",
)
AUTHOR_BLACKLIST_TOKENS = (
    "edition",
    "board",
    "readings",
    "editor",
    "editors",
    "series",
    "department",
    "faculty",
    "university",
    ".pdf",
    ".indd",
    "uni00",
)
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
RENAMEABLE_KINDS = {
    "textbook",
    "monograph",
    "reference",
    "paper",
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


def fix_mojibake(text: str) -> str:
    text = collapse_ws(text)
    if not text:
        return ""
    suspicious = ("Ã", "â", "Ð", "Ñ", "�")
    if not any(marker in text for marker in suspicious):
        return text
    for source, target in (("latin-1", "utf-8"), ("cp1252", "utf-8")):
        try:
            repaired = text.encode(source).decode(target)
        except Exception:
            continue
        if repaired and repaired != text:
            text = repaired
            break
    return collapse_ws(text)


def stringify_note(value: Any) -> str:
    if isinstance(value, str):
        return fix_mojibake(value)
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


def filename_looks_suspicious(path: Path) -> bool:
    lowered = path.name.lower()
    return any(token in lowered for token in SUSPICIOUS_FILENAME_PATTERNS)


def normalize_page_text(text: str) -> str:
    text = text.replace("\x00", " ")
    text = text.replace("\ufb01", "fi").replace("\ufb02", "fl")
    text = re.sub(r"[ \t]+", " ", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def split_stuck_words(text: str) -> str:
    text = text.replace("ﬁ", "fi").replace("ﬂ", "fl")
    text = re.sub(r"(?<=[a-z])(?=[A-Z])", " ", text)
    text = re.sub(r"(?<=\.)(?=[A-Z])", " ", text)
    text = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", " ", text)
    return collapse_ws(text)


def extract_isbns(text: str) -> list[str]:
    matches = re.findall(r"97[89][-\s]?\d[-\s]?\d{2,5}[-\s]?\d{2,7}[-\s]?\d{1,7}[-\s]?\d", text)
    found: list[str] = []
    for match in matches:
        cleaned = re.sub(r"\s+", "", match)
        if cleaned not in found:
            found.append(cleaned)
    return found[:4]


def normalize_outline_title(title: str) -> str:
    title = split_stuck_words(title)
    title = title.strip(" .-")
    return title


def useful_outline_title(title: str) -> bool:
    lowered = normalize_outline_title(title).lower()
    if not lowered:
        return False
    if lowered in {"introduction", "contents", "table of contents", "list of figures", "preface", "index"}:
        return False
    if lowered.startswith("chapter "):
        return False
    return True


def looks_like_title_line(line: str) -> bool:
    line = split_stuck_words(line)
    if not line or len(line) < 4 or len(line) > 160:
        return False
    lowered = line.lower()
    blacklist = (
        "springer",
        "issn",
        "isbn",
        "doi",
        "mathematics subject classification",
        "copyright",
        "compact textbooks in mathematics",
        "under exclusive license",
        "faculty of",
        "university",
        "the editor",
        "editor",
        "editors",
        "series",
    )
    if any(token in lowered for token in blacklist):
        return False
    if lowered.startswith("with "):
        return False
    if re.search(r"[{}\[\]=<>]|\\", line):
        return False
    if any(symbol in line for symbol in ("∈", "≤", "≥", "→", "↦")):
        return False
    if sum(ch.isalpha() for ch in line) < 4:
        return False
    return True


def is_series_line(line: str) -> bool:
    lowered = split_stuck_words(line).lower()
    return any(
        token in lowered
        for token in (
            "compact textbooks in mathematics",
            "springer",
            "birkhäuser advanced texts",
            "basler lehrbücher",
            "basler lehrbucher",
            "undergraduate texts in mathematics",
            "graduate texts in mathematics",
            "lecture notes",
            "series",
        )
    )


def looks_like_author_line(line: str) -> bool:
    line = split_stuck_words(line)
    if not line or len(line) > 80:
        return False
    lowered = line.lower()
    if is_series_line(line):
        return False
    if any(
        token in lowered
        for token in (
            "faculty of",
            "department of",
            "university",
            "isbn",
            "issn",
            "copyright",
            "introduction",
            "contents",
        )
    ):
        return False
    words = [word for word in re.split(r"\s+", line) if word]
    if not 2 <= len(words) <= 5:
        return False
    if not all(re.match(r"^[A-Z][A-Za-z.'-]*$", word) for word in words):
        return False
    if words[-1].lower().strip(".") in COMMON_TITLE_WORDS:
        return False
    return sum(1 for word in words if len(word) > 1) >= 2


def is_edition_line(line: str) -> bool:
    lowered = split_stuck_words(line).lower()
    if "edition" in lowered:
        return True
    return False


def pick_title_lines(lines: list[str], author_index: int | None) -> list[str]:
    indexed = [(idx, split_stuck_words(line)) for idx, line in enumerate(lines)]
    filtered = [
        (idx, line)
        for idx, line in indexed
        if looks_like_title_line(line) and not looks_like_author_line(line) and not is_series_line(line)
    ]
    if not filtered:
        return []

    if author_index is not None:
        after = [line for idx, line in filtered if author_index < idx <= author_index + 3]
        if after:
            return after[:3]
        before = [line for idx, line in filtered if author_index - 3 <= idx < author_index]
        if before:
            return before[-3:]

    return [line for _, line in filtered[:3]]


def infer_front_matter(pages: list[dict[str, Any]], metadata: dict[str, str]) -> dict[str, Any]:
    cleaned_pages: list[list[str]] = []
    lines: list[str] = []
    selected_page_index: int | None = None
    for page in pages[:6]:
        page_lines = [split_stuck_words(part) for part in page.get("text", "").splitlines()]
        page_lines = [line for line in page_lines if line]
        cleaned_pages.append(page_lines)
        lines.extend(page_lines)

    inferred_title = ""
    inferred_authors: list[str] = []
    inferred_edition = ""

    metadata_title = fix_mojibake(metadata.get("title", ""))
    metadata_author = fix_mojibake(metadata.get("author", ""))

    if metadata_title and len(metadata_title) > 3:
        inferred_title = metadata_title
    else:
        for page_index, page_lines in enumerate(cleaned_pages[:5]):
            if not page_lines or len(page_lines) > 12:
                continue
            if any("editor" in line.lower() for line in page_lines):
                continue
            filtered_lines = [line for line in page_lines if not is_series_line(line)]
            if not filtered_lines:
                continue
            edition_indices = [i for i, line in enumerate(filtered_lines) if is_edition_line(line)]
            author_index = next((i for i, line in enumerate(filtered_lines) if looks_like_author_line(line)), None)
            title_lines = pick_title_lines(filtered_lines, author_index)
            if edition_indices:
                non_edition = [line for line in filtered_lines if not is_edition_line(line)]
                author_candidates = [line for line in non_edition if looks_like_author_line(line)]
                title_candidates = [
                    line
                    for line in non_edition
                    if looks_like_title_line(line) and not looks_like_author_line(line)
                ]
                if author_candidates and title_candidates:
                    inferred_title = collapse_ws(" ".join(title_candidates[:3]))
                    selected_page_index = page_index
                    break
            if title_lines:
                inferred_title = collapse_ws(" ".join(title_lines[:3]))
                selected_page_index = page_index
                break
        if not inferred_title:
            title_candidates = [line for line in lines[:40] if looks_like_title_line(line)]
            ranked = sorted(
                title_candidates,
                key=lambda line: (
                    line.istitle(),
                    2 <= len(line.split()) <= 8,
                    len(line),
                ),
                reverse=True,
            )
            if ranked:
                inferred_title = ranked[0]

    if metadata_author:
        inferred_authors = [name.strip() for name in re.split(r"\s*(?:,|;| and )\s*", metadata_author) if name.strip()]
    else:
        candidate_page_sets: list[list[str]] = []
        if selected_page_index is not None:
            candidate_page_sets.append(cleaned_pages[selected_page_index])
            if selected_page_index + 1 < len(cleaned_pages):
                candidate_page_sets.append(cleaned_pages[selected_page_index + 1])
        candidate_page_sets.extend(cleaned_pages[:5])

        seen_keys: set[tuple[str, ...]] = set()
        for page_lines in candidate_page_sets:
            key = tuple(page_lines)
            if key in seen_keys:
                continue
            seen_keys.add(key)
            if any("editor" in line.lower() for line in page_lines):
                continue
            author_lines = [line for line in page_lines if looks_like_author_line(line)]
            if author_lines:
                inferred_authors = [author_lines[0]]
                break

    joined = "\n".join(lines)
    edition_match = re.search(r"\b(\d+)(?:st|nd|rd|th)?\s+Edition\b", joined, re.IGNORECASE)
    if edition_match:
        inferred_edition = edition_match.group(0)
    else:
        word_match = re.search(
            r"\b(First|Second|Third|Fourth|Fifth|Sixth|Seventh|Eighth|Ninth|Tenth)\s+Edition\b",
            joined,
            re.IGNORECASE,
        )
        if word_match:
            inferred_edition = word_match.group(0)

    return {
        "title": fix_mojibake(inferred_title),
        "authors": [fix_mojibake(author) for author in inferred_authors if fix_mojibake(author)],
        "edition": fix_mojibake(inferred_edition),
        "isbns": extract_isbns(joined),
    }


def extract_layout_payload(path: Path, page_limit: int, char_limit: int) -> dict[str, Any] | None:
    if fitz is None:
        return None

    doc = fitz.open(path)
    page_records: list[dict[str, Any]] = []
    text_parts: list[str] = []
    outline_titles: list[str] = []

    try:
        toc = doc.get_toc(simple=True)
    except Exception:
        toc = []

    for entry in toc[:20]:
        if len(entry) >= 2:
            title = normalize_outline_title(str(entry[1]))
            if useful_outline_title(title) and title not in outline_titles:
                outline_titles.append(title)

    for index in range(min(page_limit, doc.page_count)):
        page = doc.load_page(index)
        text = normalize_page_text(page.get_text("text") or "")
        if text:
            text_parts.append(text)

        lines: list[dict[str, Any]] = []
        try:
            page_dict = page.get_text("dict")
        except Exception:
            page_dict = {}
        for block in page_dict.get("blocks", []):
            if block.get("type") != 0:
                continue
            for line in block.get("lines", []):
                spans = [span for span in line.get("spans", []) if collapse_ws(str(span.get("text", "") or ""))]
                if not spans:
                    continue
                line_text = split_stuck_words(" ".join(collapse_ws(str(span.get("text", "") or "")) for span in spans))
                if not line_text:
                    continue
                bbox = line.get("bbox") or block.get("bbox") or (0, 0, 0, 0)
                lines.append(
                    {
                        "text": line_text,
                        "size": max(float(span.get("size", 0.0) or 0.0) for span in spans),
                        "y0": float(bbox[1]),
                    }
                )
        lines.sort(key=lambda item: (item["y0"], -item["size"]))
        page_records.append(
            {
                "page": index + 1,
                "text": text[:4000],
                "lines": lines[:60],
            }
        )
        if len("\n\n".join(text_parts)) >= char_limit:
            break

    metadata = {
        "title": fix_mojibake(str(doc.metadata.get("title", "") or "")),
        "author": fix_mojibake(str(doc.metadata.get("author", "") or "")),
        "subject": fix_mojibake(str(doc.metadata.get("subject", "") or "")),
        "creator": fix_mojibake(str(doc.metadata.get("creator", "") or "")),
        "producer": fix_mojibake(str(doc.metadata.get("producer", "") or "")),
    }

    front_matter = infer_front_matter_from_layout(page_records, metadata, outline_titles)
    return {
        "metadata": metadata,
        "page_records": page_records,
        "outline_titles": outline_titles[:8],
        "text_sample": "\n\n".join(text_parts)[:char_limit],
        "front_matter": front_matter,
    }


def infer_front_matter_from_layout(
    pages: list[dict[str, Any]],
    metadata: dict[str, str],
    outline_titles: list[str],
) -> dict[str, Any]:
    inferred_title = ""
    inferred_authors: list[str] = []
    inferred_edition = ""

    metadata_title = fix_mojibake(metadata.get("title", ""))
    metadata_author = fix_mojibake(metadata.get("author", ""))

    if metadata_title and is_plausible_book_title(metadata_title):
        inferred_title = metadata_title

    selected_page_index: int | None = None
    if not inferred_title:
        best_score = float("-inf")
        best_title_lines: list[str] = []
        for page_index, page in enumerate(pages[:5]):
            lines = page.get("lines", []) or []
            if not lines:
                continue
            page_texts = [str(item.get("text", "") or "") for item in lines]
            if any("editor" in text.lower() for text in page_texts):
                continue
            filtered = [item for item in lines if not is_series_line(str(item.get("text", "") or ""))]
            if not filtered:
                continue
            max_size = max(float(item.get("size", 0.0) or 0.0) for item in filtered)
            title_candidates = [
                item
                for item in filtered
                if looks_like_title_line(str(item.get("text", "") or ""))
                and not looks_like_author_line(str(item.get("text", "") or ""))
            ]
            if not title_candidates:
                continue
            prominent = [item for item in title_candidates if float(item.get("size", 0.0) or 0.0) >= max_size - 1.0]
            if not prominent:
                prominent = sorted(title_candidates, key=lambda item: float(item.get("size", 0.0) or 0.0), reverse=True)[:3]
            prominent.sort(key=lambda item: float(item.get("y0", 0.0) or 0.0))
            title_lines = [split_stuck_words(str(item.get("text", "") or "")) for item in prominent[:3]]
            candidate_title = collapse_ws(" ".join(title_lines))
            if not is_plausible_book_title(candidate_title):
                continue
            score = max(float(item.get("size", 0.0) or 0.0) for item in prominent) - page_index * 0.5
            if any(is_edition_line(text) for text in page_texts):
                score += 1.0
            if score > best_score:
                best_score = score
                best_title_lines = title_lines
                selected_page_index = page_index
        if best_title_lines:
            inferred_title = collapse_ws(" ".join(best_title_lines))

    if not inferred_title:
        for title in outline_titles:
            if is_plausible_book_title(title):
                inferred_title = title
                break

    if metadata_author:
        author_candidates = [part.strip() for part in re.split(r"\s*(?:,|;| and )\s*", metadata_author) if part.strip()]
        good = [part for part in author_candidates if is_plausible_author_name(part)]
        if good:
            inferred_authors = good

    if not inferred_authors:
        candidate_pages: list[dict[str, Any]] = []
        if selected_page_index is not None and selected_page_index < len(pages):
            candidate_pages.append(pages[selected_page_index])
            if selected_page_index + 1 < len(pages):
                candidate_pages.append(pages[selected_page_index + 1])
        candidate_pages.extend(pages[:5])

        seen: set[int] = set()
        for page in candidate_pages:
            page_no = int(page.get("page", 0) or 0)
            if page_no in seen:
                continue
            seen.add(page_no)
            lines = page.get("lines", []) or []
            page_texts = [str(item.get("text", "") or "") for item in lines]
            if any("editor" in text.lower() for text in page_texts):
                continue
            author_lines = [
                split_stuck_words(str(item.get("text", "") or ""))
                for item in lines
                if looks_like_author_line(str(item.get("text", "") or ""))
            ]
            plausible = [line for line in author_lines if is_plausible_author_name(line)]
            if plausible:
                inferred_authors = [plausible[0]]
                break

    combined_text = "\n".join(page.get("text", "") for page in pages[:8])
    edition_match = re.search(r"\b(\d+)(?:st|nd|rd|th)?\s+Edition\b", combined_text, re.IGNORECASE)
    if edition_match:
        inferred_edition = edition_match.group(0)
    else:
        word_match = re.search(
            r"\b(First|Second|Third|Fourth|Fifth|Sixth|Seventh|Eighth|Ninth|Tenth)\s+Edition\b",
            combined_text,
            re.IGNORECASE,
        )
        if word_match:
            inferred_edition = word_match.group(0)

    return {
        "title": fix_mojibake(inferred_title),
        "authors": [fix_mojibake(author) for author in inferred_authors if fix_mojibake(author)],
        "edition": fix_mojibake(inferred_edition),
        "isbns": extract_isbns(combined_text),
    }


def extract_pdf_payload(path: Path, max_pages: int, max_chars: int) -> dict[str, Any]:
    page_limit = max_pages
    char_limit = max_chars
    if filename_looks_suspicious(path):
        page_limit = max(page_limit, 15)
        char_limit = max(char_limit, 25000)

    layout_payload = extract_layout_payload(path, page_limit, char_limit)
    if layout_payload is not None:
        normalized_metadata = layout_payload["metadata"]
        page_records = layout_payload["page_records"]
        text_sample = layout_payload["text_sample"]
        front_matter = layout_payload["front_matter"]
        outline_titles = layout_payload["outline_titles"]
    else:
        reader = PdfReader(str(path))
        metadata = reader.metadata or {}
        pages = []
        page_records = []
        for i, page in enumerate(reader.pages[:page_limit], start=1):
            try:
                text = normalize_page_text(page.extract_text() or "")
            except Exception:
                text = ""
            if text:
                pages.append(text)
                page_records.append({"page": i, "text": text[:4000]})
            joined = "\n\n".join(pages)
            if len(joined) >= char_limit:
                break

        text_sample = "\n\n".join(pages)[:char_limit]
        normalized_metadata = {
            "title": fix_mojibake(str(metadata.get("/Title", "") or "")),
            "author": fix_mojibake(str(metadata.get("/Author", "") or "")),
            "subject": fix_mojibake(str(metadata.get("/Subject", "") or "")),
            "creator": fix_mojibake(str(metadata.get("/Creator", "") or "")),
            "producer": fix_mojibake(str(metadata.get("/Producer", "") or "")),
        }
        front_matter = infer_front_matter(page_records, normalized_metadata)
        outline_titles = []
    return {
        "path": str(path),
        "filename": path.name,
        "metadata": normalized_metadata,
        "front_matter": front_matter,
        "page_samples": page_records[:8],
        "outline_titles": outline_titles,
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
        - prefer front_matter, outline_titles, extracted text, and PDF metadata over filename guesses
        - for book-like PDFs with clear title pages and copyright pages, set rename_recommended=true even if the filename is garbage
        - Springer-style ebooks often reveal their best metadata on title and copyright pages; use those pages heavily
        - outline_titles usually come from the PDF bookmarks/table of contents and can help confirm the subject, but title pages still dominate title/author extraction

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
    title = fix_mojibake(title)
    if not title:
        return ""
    title = re.split(r"\s*[:\-]\s*", title, maxsplit=1)[0]
    title = re.split(r"\s*\(\s*", title, maxsplit=1)[0]
    title = re.sub(r"\b(Second|Third|Fourth|Fifth|Sixth|Seventh|Eighth|Ninth|Tenth)\s+Edition\b", "", title, flags=re.IGNORECASE)
    title = re.sub(r"\b\d+(st|nd|rd|th)\s+Edition\b", "", title, flags=re.IGNORECASE)
    title = collapse_ws(title)
    return title


def is_plausible_book_title(title: str) -> bool:
    title = compact_title(title)
    lowered = title.lower()
    if not title or len(title) < 4:
        return False
    if any(token in lowered for token in TITLE_BLACKLIST_TOKENS):
        return False
    if re.match(r"^(97[89]|[0-9_./-]+$)", lowered):
        return False
    if re.search(r"\b\d{4}\b", title) and len(title.split()) <= 4:
        return False
    if sum(ch.isalpha() for ch in title) < 4:
        return False
    if len(title.split()) == 1 and re.fullmatch(r"[A-Z][a-z]+(?:\s+[A-Z][a-z]+)?", title):
        return False
    return True


def compact_edition(edition: str) -> str:
    edition = fix_mojibake(edition)
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
    clean = [fix_mojibake(a) for a in authors if fix_mojibake(a)]
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


def is_plausible_author_name(name: str) -> bool:
    name = fix_mojibake(name)
    lowered = name.lower()
    if not name or len(name) < 3 or len(name) > 60:
        return False
    if any(token in lowered for token in AUTHOR_BLACKLIST_TOKENS):
        return False
    if re.search(r"\d", name):
        return False
    words = [word for word in re.split(r"\s+", name) if word]
    if not 2 <= len(words) <= 5:
        return False
    if words[-1].lower().strip(".") in COMMON_TITLE_WORDS:
        return False
    return True


def has_strong_rename_metadata(data: dict[str, Any]) -> bool:
    kind = collapse_ws(str(data.get("kind", "") or "")).lower()
    title = compact_title(str(data.get("title", "") or ""))
    authors = normalize_string_list(data.get("authors", []))
    confidence = float(data.get("confidence", 0.0) or 0.0)
    metadata_sources = data.get("metadata_sources", {}) or {}
    title_src = collapse_ws(str(metadata_sources.get("title", "unknown"))).lower()
    author_src = collapse_ws(str(metadata_sources.get("authors", "unknown"))).lower()

    if kind not in RENAMEABLE_KINDS:
        return False
    if confidence < 0.85:
        return False
    if not title or not authors:
        return False
    if title_src == "filename" or author_src == "filename":
        return False
    if not is_plausible_book_title(title):
        return False
    if not any(is_plausible_author_name(author) for author in authors):
        return False
    return True


def enrich_model_output(data: dict[str, Any], payload: dict[str, Any]) -> dict[str, Any]:
    enriched = dict(data)
    front_matter = payload.get("front_matter", {}) or {}
    metadata_sources = dict(enriched.get("metadata_sources", {}) or {})

    title = fix_mojibake(str(enriched.get("title", "") or ""))
    if not title and front_matter.get("title"):
        enriched["title"] = front_matter["title"]
        metadata_sources["title"] = "front_matter"

    authors = normalize_string_list(enriched.get("authors", []))
    if not authors and front_matter.get("authors"):
        enriched["authors"] = front_matter["authors"]
        metadata_sources["authors"] = "front_matter"

    edition = fix_mojibake(str(enriched.get("edition", "") or ""))
    if not edition and front_matter.get("edition"):
        enriched["edition"] = front_matter["edition"]
        metadata_sources["edition"] = "front_matter"

    kind = collapse_ws(str(enriched.get("kind", "") or "")).lower()
    if not kind and front_matter.get("title") and front_matter.get("authors"):
        enriched["kind"] = "textbook"

    confidence = float(enriched.get("confidence", 0.0) or 0.0)
    if confidence < 0.5 and front_matter.get("title") and front_matter.get("authors"):
        enriched["confidence"] = 0.9

    if not enriched.get("rename_reason") and front_matter.get("title") and front_matter.get("authors"):
        enriched["rename_reason"] = "Strong front-matter metadata extracted from title and copyright pages"

    enriched["metadata_sources"] = metadata_sources
    return enriched


def should_skip_rename(data: dict[str, Any]) -> tuple[bool, list[str]]:
    reasons = []
    kind = collapse_ws(str(data.get("kind", "") or "")).lower()
    title = fix_mojibake(str(data.get("title", "") or ""))
    edition = fix_mojibake(str(data.get("edition", "") or ""))
    authors = data.get("authors", []) or []
    confidence = float(data.get("confidence", 0.0) or 0.0)
    rename_recommended = bool(data.get("rename_recommended", False))
    strong_metadata = has_strong_rename_metadata(data)

    if kind in LECTURE_NOTE_KINDS:
        reasons.append(f"kind={kind}")
    if not rename_recommended and not strong_metadata:
        reasons.append("model_did_not_recommend_rename")
    if confidence < 0.78:
        reasons.append(f"low_confidence={confidence:.2f}")
    if not title:
        reasons.append("missing_title")
    if not authors:
        reasons.append("missing_authors")
    if title and not is_plausible_book_title(title):
        reasons.append("implausible_title")
    if authors and not any(is_plausible_author_name(author) for author in authors):
        reasons.append("implausible_authors")
    if edition and len(edition) > 40:
        reasons.append("edition_looks_unreliable")

    title_src = (data.get("metadata_sources", {}) or {}).get("title", "unknown")
    author_src = (data.get("metadata_sources", {}) or {}).get("authors", "unknown")
    if title_src == "filename":
        reasons.append("title_from_filename_only")
    if author_src == "filename":
        reasons.append("authors_from_filename_only")

    if strong_metadata and "model_did_not_recommend_rename" in reasons:
        reasons = [reason for reason in reasons if reason != "model_did_not_recommend_rename"]
        reasons.append("heuristic_override_strong_metadata")

    return (len(reasons) > 0, reasons)


def build_target_filename(data: dict[str, Any]) -> str | None:
    title = sanitize_filename_part(compact_title(str(data.get("title", "") or "")), MAX_TITLE_LEN)
    edition = sanitize_filename_part(compact_edition(str(data.get("edition", "") or "")), 10)
    author = sanitize_filename_part(normalize_author_segment(data.get("authors", []) or []), MAX_AUTHOR_LEN)
    authors = normalize_string_list(data.get("authors", []))
    if not title or not author:
        return None
    if not is_plausible_book_title(title):
        return None
    if not any(is_plausible_author_name(name) for name in authors):
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
        title=fix_mojibake(str(data.get("title", "") or "")),
        edition=fix_mojibake(str(data.get("edition", "") or "")) or None,
        authors=normalize_string_list(data.get("authors", [])),
        kind=collapse_ws(str(data.get("kind", "") or "")),
        primary_topic=collapse_ws(str(data.get("primary_topic", "") or "")),
        secondary_topics=normalize_string_list(data.get("secondary_topics", [])),
        level=collapse_ws(str(data.get("level", "") or "")),
        prerequisites=normalize_string_list(data.get("prerequisites", [])),
        summary=fix_mojibake(str(data.get("summary", "") or "")),
        recommended_for=fix_mojibake(str(data.get("recommended_for", "") or "")),
        confidence=float(data.get("confidence", 0.0) or 0.0),
        rename_recommended=bool(data.get("rename_recommended", False)),
        rename_reason=fix_mojibake(str(data.get("rename_reason", "") or "")),
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
            data = enrich_model_output(data, payload)
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
