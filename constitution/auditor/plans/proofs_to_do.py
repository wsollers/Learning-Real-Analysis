from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable

import yaml


DEFAULT_TYPES = {"thm", "lem", "prop", "cor"}
TODO_MARKERS = ("TODO", "TBD", "proof omitted", "to be written")


@dataclass(frozen=True)
class ProofTodo:
    volume: str
    chapter: str
    chapter_path: Path
    label: str
    kind: str
    title: str
    source_file: str
    proof_file: str
    reason: str


def find_proofs_to_do(
    repo_root: Path,
    *,
    types: Iterable[str] = DEFAULT_TYPES,
    include_existing_todo: bool = True,
) -> list[ProofTodo]:
    """Find theorem-like chapter entries whose proofs still need work."""

    wanted_types = {t.strip() for t in types if t.strip()}
    todos: list[ProofTodo] = []

    for yaml_path in sorted(repo_root.rglob("chapter.yaml")):
        if _skip_path(yaml_path):
            continue
        chapter_root = yaml_path.parent
        data = yaml.safe_load(yaml_path.read_text(encoding="utf-8")) or {}
        environments = data.get("environments") or []
        if not isinstance(environments, list):
            continue

        chapter_path = _chapter_display_path(repo_root, chapter_root)
        volume = str(data.get("volume") or _infer_volume(repo_root, chapter_root))
        chapter = str(data.get("subject") or chapter_root.name)

        for entry in environments:
            if not isinstance(entry, dict):
                continue
            kind = str(entry.get("type") or "").strip()
            if kind not in wanted_types:
                continue

            label = str(entry.get("label") or "").strip()
            title = str(entry.get("display_title") or "").strip()
            source_file = str(entry.get("file") or "").strip()
            proof_file = str(entry.get("proof_file") or "").strip()

            if _is_capstoneish(label, title, source_file, proof_file):
                continue

            reason = ""
            if not proof_file or proof_file.lower() == "null":
                reason = "No proof file listed in chapter.yaml."
            else:
                proof_path = chapter_root / proof_file
                if not proof_path.exists():
                    reason = "Proof file is listed but missing on disk."
                elif include_existing_todo and _contains_todo_marker(proof_path):
                    reason = "Proof file exists but still contains a TODO marker."

            if not reason:
                continue

            todos.append(
                ProofTodo(
                    volume=volume,
                    chapter=chapter,
                    chapter_path=chapter_path,
                    label=label,
                    kind=kind,
                    title=title,
                    source_file=source_file,
                    proof_file=proof_file,
                    reason=reason,
                )
            )

    return todos


def write_proofs_to_do_markdown(
    todos: list[ProofTodo],
    repo_root: Path,
    output_path: Path | None = None,
) -> Path:
    out = output_path or (repo_root / "proofs-to-do.md")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(format_proofs_to_do_markdown(todos), encoding="utf-8")
    return out


def format_proofs_to_do_markdown(todos: list[ProofTodo]) -> str:
    lines: list[str] = [
        "# Proofs To Do",
        "",
        f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
        "",
        "Includes theorem-like results (`thm`, `lem`, `prop`, `cor`) with no proof file, a missing proof file, or an existing proof file that still contains a TODO marker. Capstones and exercises are excluded.",
        "",
        f"Total: {len(todos)}",
        "",
    ]

    grouped: dict[tuple[str, str, str], list[ProofTodo]] = {}
    for item in todos:
        key = (item.volume, item.chapter, item.chapter_path.as_posix())
        grouped.setdefault(key, []).append(item)

    for (volume, chapter, chapter_path), items in sorted(grouped.items()):
        lines += [
            f"## {volume} / {chapter}",
            "",
            f"Chapter: `{chapter_path}`",
            "",
            "| Type | Label | Title | Reason | Source | Proof file |",
            "| --- | --- | --- | --- | --- | --- |",
        ]
        for item in sorted(items, key=lambda x: (x.source_file, x.label)):
            lines.append(
                "| "
                + " | ".join(
                    [
                        _md(item.kind),
                        f"`{_md(item.label)}`",
                        _md(item.title or item.label),
                        _md(item.reason),
                        f"`{_md(item.source_file)}`",
                        f"`{_md(item.proof_file or '(none)')}`",
                    ]
                )
                + " |"
            )
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def _contains_todo_marker(path: Path) -> bool:
    text = path.read_text(encoding="utf-8", errors="ignore")
    lowered = text.lower()
    return any(marker.lower() in lowered for marker in TODO_MARKERS)


def _skip_path(path: Path) -> bool:
    parts = {p.lower() for p in path.parts}
    return bool(parts & {".git", ".explorer", "node_modules", "__pycache__"})


def _is_capstoneish(*values: str) -> bool:
    haystack = " ".join(v.lower() for v in values if v)
    return "capstone" in haystack or "proofs\\exercises" in haystack or "proofs/exercises" in haystack


def _infer_volume(repo_root: Path, chapter_root: Path) -> str:
    try:
        rel = chapter_root.relative_to(repo_root)
    except ValueError:
        return ""
    return rel.parts[0] if rel.parts else ""


def _chapter_display_path(repo_root: Path, chapter_root: Path) -> Path:
    try:
        return chapter_root.relative_to(repo_root)
    except ValueError:
        return chapter_root


def _md(value: str) -> str:
    return str(value).replace("|", "\\|").replace("\n", " ")
