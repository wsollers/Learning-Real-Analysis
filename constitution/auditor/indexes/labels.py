"""
labels.py
Builds a formal mathematical label index from LaTeX source.

This index is intentionally narrow: it includes only theorem-like mathematical
objects that are legitimate dependency targets:
  definition, theorem, lemma, proposition, corollary, axiom.
It excludes proofs, examples, exercises, figures, ordinary remarks, and
navigation-only labels.
"""

from __future__ import annotations

import json
import re
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Any

from auditor import config


FORMAL_ENVS = {
    "definition": "def",
    "theorem": "thm",
    "lemma": "lem",
    "proposition": "prop",
    "corollary": "cor",
    "axiom": "ax",
}

ENV_OPEN = re.compile(
    r"\\begin\{(" + "|".join(FORMAL_ENVS) + r")\}"
    r"(?:\[([^\]]*)\])?",
    re.IGNORECASE,
)
LABEL = re.compile(r"\\label\{([^}]+)\}")
SECTION = re.compile(r"\\section\*?\{([^}]*)\}")
SUBSECTION = re.compile(r"\\subsection\*?\{([^}]*)\}")


@dataclass
class FormalLabel:
    label: str
    environment: str
    artifact_type: str
    title: str
    file: str
    line: int
    chapter: str
    volume: str
    section: str | None
    subsection: str | None


@dataclass
class IndexIssue:
    type: str
    file: str
    line: int
    message: str
    label: str | None = None
    environment: str | None = None


def build_label_index(scope_path: Path) -> dict[str, Any]:
    scope = scope_path.resolve()
    tex_files = _tex_files(scope)

    items: list[FormalLabel] = []
    issues: list[IndexIssue] = []

    for tex_file in tex_files:
        file_items, file_issues = _scan_file(tex_file)
        items.extend(file_items)
        issues.extend(file_issues)

    duplicates = _duplicate_issues(items)
    issues.extend(duplicates)

    items_sorted = sorted(items, key=lambda item: (item.file.lower(), item.line, item.label))
    issues_sorted = sorted(issues, key=lambda issue: (issue.file.lower(), issue.line, issue.type, issue.label or ""))

    return {
        "scope": _display_path(scope),
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "included_environments": list(FORMAL_ENVS),
        "count": len(items_sorted),
        "items": [asdict(item) for item in items_sorted],
        "issues": [asdict(issue) for issue in issues_sorted],
        "duplicates": [
            asdict(issue) for issue in issues_sorted if issue.type == "duplicate_label"
        ],
        "missing_labels": [
            asdict(issue) for issue in issues_sorted if issue.type == "missing_label"
        ],
        "prefix_mismatches": [
            asdict(issue) for issue in issues_sorted if issue.type == "prefix_mismatch"
        ],
    }


def write_label_index(index: dict[str, Any], output_dir: Path | None = None) -> tuple[Path, Path]:
    out_dir = output_dir or (config.REPORTS_DIR / "indexes")
    out_dir.mkdir(parents=True, exist_ok=True)
    slug = _scope_slug(index["scope"])
    json_path = out_dir / f"{slug}-formal-label-index.json"
    md_path = out_dir / f"{slug}-formal-label-index.md"

    json_path.write_text(json.dumps(index, indent=2, ensure_ascii=False), encoding="utf-8")
    md_path.write_text(format_label_index_markdown(index), encoding="utf-8")
    return json_path, md_path


def format_label_index_markdown(index: dict[str, Any]) -> str:
    issues = index.get("issues", [])
    lines = [
        "# Formal Label Index",
        f"- **Scope:** `{index['scope']}`",
        f"- **Generated:** {index['generated_at']}",
        f"- **Formal items:** {index['count']}",
        f"- **Issues:** {len(issues)}",
        "",
        "Included environments: `definition`, `theorem`, `lemma`, `proposition`, `corollary`, `axiom`.",
        "",
    ]

    if issues:
        issue_counts = Counter(issue["type"] for issue in issues)
        lines += ["## Issues", ""]
        for issue_type, count in issue_counts.most_common():
            lines.append(f"- `{issue_type}`: {count}")
        lines.append("")
        lines += ["| Type | Label | Environment | File | Line | Message |", "|---|---|---|---|---:|---|"]
        for issue in issues:
            lines.append(
                "| "
                + " | ".join([
                    f"`{issue['type']}`",
                    f"`{issue.get('label') or ''}`",
                    f"`{issue.get('environment') or ''}`",
                    f"`{issue['file']}`",
                    str(issue["line"]),
                    issue["message"].replace("|", "\\|"),
                ])
                + " |"
            )
        lines.append("")
    else:
        lines += ["## Issues", "", "_None._", ""]

    by_chapter = defaultdict(list)
    for item in index.get("items", []):
        by_chapter[item.get("chapter") or ""].append(item)

    lines += ["## Items", ""]
    for chapter in sorted(by_chapter):
        title = chapter or "(unknown chapter)"
        lines += [f"### {title}", ""]
        lines += ["| Label | Type | Title | File | Line |", "|---|---|---|---|---:|"]
        for item in sorted(by_chapter[chapter], key=lambda row: (row["file"].lower(), row["line"], row["label"])):
            lines.append(
                "| "
                + " | ".join([
                    f"`{item['label']}`",
                    f"`{item['artifact_type']}`",
                    item["title"].replace("|", "\\|"),
                    f"`{item['file']}`",
                    str(item["line"]),
                ])
                + " |"
            )
        lines.append("")

    return "\n".join(lines)


def _tex_files(scope: Path) -> list[Path]:
    if scope.is_file():
        return [scope] if scope.suffix.lower() == ".tex" else []
    return sorted(
        path for path in scope.rglob("*.tex")
        if _should_scan(path)
    )


def _should_scan(path: Path) -> bool:
    parts = {part.lower() for part in path.parts}
    if ".git" in parts:
        return False
    return True


def _scan_file(tex_file: Path) -> tuple[list[FormalLabel], list[IndexIssue]]:
    rel = _display_path(tex_file)
    text = tex_file.read_text(encoding="utf-8")
    line_starts = _line_starts(text)
    items: list[FormalLabel] = []
    issues: list[IndexIssue] = []

    sections = _section_markers(text, SECTION, line_starts)
    subsections = _section_markers(text, SUBSECTION, line_starts)

    for match in ENV_OPEN.finditer(text):
        env = match.group(1).lower()
        title = (match.group(2) or "").strip()
        line = _line_number(line_starts, match.start())
        end_match = re.search(r"\\end\{" + re.escape(env) + r"\}", text[match.end():])
        if end_match:
            body_end = match.end() + end_match.start()
            search_area = text[match.end():body_end]
        else:
            search_area = text[match.end(): match.end() + 1000]
            issues.append(IndexIssue(
                type="malformed_environment",
                file=rel,
                line=line,
                environment=env,
                message=f"Could not find matching \\end{{{env}}}.",
            ))

        labels = list(LABEL.finditer(search_area))
        if not labels:
            issues.append(IndexIssue(
                type="missing_label",
                file=rel,
                line=line,
                environment=env,
                message=f"Formal environment \\begin{{{env}}} has no label.",
            ))
            continue

        label = labels[0].group(1)
        artifact_type = FORMAL_ENVS[env]
        expected_prefix = artifact_type
        actual_prefix = label.split(":", 1)[0] if ":" in label else ""
        if actual_prefix != expected_prefix:
            issues.append(IndexIssue(
                type="prefix_mismatch",
                file=rel,
                line=line,
                label=label,
                environment=env,
                message=f"Expected label prefix {expected_prefix}: for {env}, got {actual_prefix}:.",
            ))

        if len(labels) > 1:
            issues.append(IndexIssue(
                type="multiple_labels",
                file=rel,
                line=line,
                label=label,
                environment=env,
                message=f"Formal environment contains {len(labels)} labels; using the first.",
            ))

        items.append(FormalLabel(
            label=label,
            environment=env,
            artifact_type=artifact_type,
            title=title or label,
            file=rel,
            line=line,
            chapter=_chapter_name(tex_file),
            volume=_volume_name(tex_file),
            section=_active_marker(sections, line),
            subsection=_active_marker(subsections, line),
        ))

    return items, issues


def _duplicate_issues(items: list[FormalLabel]) -> list[IndexIssue]:
    grouped: dict[str, list[FormalLabel]] = defaultdict(list)
    for item in items:
        grouped[item.label].append(item)

    issues: list[IndexIssue] = []
    for label, matches in grouped.items():
        if len(matches) <= 1:
            continue
        locations = ", ".join(f"{item.file}:{item.line}" for item in matches)
        for item in matches:
            issues.append(IndexIssue(
                type="duplicate_label",
                file=item.file,
                line=item.line,
                label=label,
                environment=item.environment,
                message=f"Duplicate formal label appears at: {locations}.",
            ))
    return issues


def _line_starts(text: str) -> list[int]:
    starts = [0]
    for match in re.finditer("\n", text):
        starts.append(match.end())
    return starts


def _line_number(line_starts: list[int], offset: int) -> int:
    lo, hi = 0, len(line_starts)
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if line_starts[mid] <= offset:
            lo = mid
        else:
            hi = mid
    return lo + 1


def _section_markers(text: str, pattern: re.Pattern[str], line_starts: list[int]) -> list[tuple[int, str]]:
    markers = []
    for match in pattern.finditer(text):
        markers.append((_line_number(line_starts, match.start()), match.group(1).strip()))
    return markers


def _active_marker(markers: list[tuple[int, str]], line: int) -> str | None:
    active = None
    for marker_line, title in markers:
        if marker_line <= line:
            active = title
        else:
            break
    return active


def _chapter_name(path: Path) -> str:
    parts = list(path.parts)
    lowered = [part.lower() for part in parts]
    if "analysis" in lowered:
        index = lowered.index("analysis")
        if index + 1 < len(parts):
            return parts[index + 1]
    return ""


def _volume_name(path: Path) -> str:
    for part in path.parts:
        if part.lower().startswith("volume-"):
            return part
    return ""


def _display_path(path: Path) -> str:
    resolved = path.resolve()
    try:
        return str(resolved.relative_to(config.REPO_ROOT)).replace("\\", "/")
    except ValueError:
        return str(resolved).replace("\\", "/")


def _scope_slug(scope: str) -> str:
    slug = re.sub(r"[^A-Za-z0-9]+", "-", scope).strip("-").lower()
    return slug or "repo"

