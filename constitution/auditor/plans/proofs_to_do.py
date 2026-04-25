from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable
import re

import yaml


DEFAULT_TYPES = {"thm", "lem", "prop", "cor"}
TODO_MARKERS = ("TODO", "TBD", "proof omitted", "to be written")


@dataclass(frozen=True)
class ProofTodo:
    volume: str
    chapter: str
    chapter_path: Path
    chapter_order: int
    note_order: int
    label_order: int
    label: str
    kind: str
    title: str
    source_file: str
    proof_file: str
    reason: str
    dependency_labels: tuple[str, ...]


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
        note_order_map = _build_note_order_map(chapter_root)
        label_order_cache: dict[tuple[str, str], int] = {}
        dependency_cache: dict[tuple[str, str, str], tuple[str, ...]] = {}

        chapter_path = _chapter_display_path(repo_root, chapter_root)
        volume = str(data.get("volume") or _infer_volume(repo_root, chapter_root))
        chapter = str(data.get("subject") or chapter_root.name)

        for index, entry in enumerate(environments):
            if not isinstance(entry, dict):
                continue
            kind = str(entry.get("type") or "").strip()
            if kind not in wanted_types:
                continue

            label = str(entry.get("label") or "").strip()
            title = str(entry.get("display_title") or "").strip()
            source_file = str(entry.get("file") or "").strip()
            proof_file = str(entry.get("proof_file") or "").strip()
            note_order = note_order_map.get(_norm_rel_path(source_file), index)
            label_order = _find_label_order(
                chapter_root,
                source_file,
                label,
                cache=label_order_cache,
            )

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

            dependency_labels = _extract_dependency_labels(
                chapter_root,
                source_file,
                proof_file,
                label,
                cache=dependency_cache,
            )

            todos.append(
                ProofTodo(
                    volume=volume,
                    chapter=chapter,
                    chapter_path=chapter_path,
                    chapter_order=index,
                    note_order=note_order,
                    label_order=label_order,
                    label=label,
                    kind=kind,
                    title=title,
                    source_file=source_file,
                    proof_file=proof_file,
                    reason=reason,
                    dependency_labels=dependency_labels,
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
        ordered_items = _order_items_with_dependencies(items)
        lines += [
            f"## {volume} / {chapter}",
            "",
            f"Chapter: `{chapter_path}`",
            "",
            "| Type | Label | Title | Reason | Source | Proof file |",
            "| --- | --- | --- | --- | --- | --- |",
        ]
        for item in ordered_items:
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


def _norm_rel_path(path: str) -> str:
    return path.replace("\\", "/").strip()


def _build_note_order_map(chapter_root: Path) -> dict[str, int]:
    notes_index = chapter_root / "notes" / "index.tex"
    if not notes_index.exists():
        return {}
    ordered_files: list[str] = []
    seen: set[str] = set()
    _collect_note_inputs(chapter_root, notes_index, ordered_files, seen)
    return {rel: i for i, rel in enumerate(ordered_files)}


_INPUT_RE = re.compile(r"\\input\{([^}]+)\}")


def _collect_note_inputs(
    chapter_root: Path,
    tex_file: Path,
    ordered_files: list[str],
    seen: set[str],
) -> None:
    try:
        text = tex_file.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return
    for match in _INPUT_RE.finditer(text):
        raw = match.group(1).strip()
        if not raw:
            continue
        rel = raw if raw.endswith(".tex") else f"{raw}.tex"
        rel = _norm_rel_path(rel)
        abs_path = chapter_root.parent.parent / rel if rel.startswith("volume-") else chapter_root / rel
        try:
            rel_to_chapter = _norm_rel_path(str(abs_path.relative_to(chapter_root)))
        except ValueError:
            rel_to_chapter = rel
        if not abs_path.exists():
            continue
        if abs_path.name == "index.tex":
            key = f"index::{rel_to_chapter}"
            if key in seen:
                continue
            seen.add(key)
            _collect_note_inputs(chapter_root, abs_path, ordered_files, seen)
            continue
        if rel_to_chapter not in seen:
            seen.add(rel_to_chapter)
            ordered_files.append(rel_to_chapter)


def _find_label_order(
    chapter_root: Path,
    source_file: str,
    label: str,
    *,
    cache: dict[tuple[str, str], int],
) -> int:
    key = (_norm_rel_path(source_file), label)
    if key in cache:
        return cache[key]
    if not source_file or not label:
        cache[key] = 10**9
        return cache[key]
    path = chapter_root / source_file
    if not path.exists():
        cache[key] = 10**9
        return cache[key]
    needle = f"\\label{{{label}}}"
    try:
        for lineno, line in enumerate(path.read_text(encoding='utf-8', errors='ignore').splitlines()):
            if needle in line:
                cache[key] = lineno
                return lineno
    except OSError:
        pass
    cache[key] = 10**9
    return cache[key]


_HYPERREF_RE = re.compile(r"\\hyperref\[([^\]]+)\]")
_REMARK_RE = re.compile(r"\\begin\{remark\*\}\[([^\]]+)\](.*?)\\end\{remark\*\}", re.DOTALL)


def _extract_dependency_labels(
    chapter_root: Path,
    source_file: str,
    proof_file: str,
    theorem_label: str,
    *,
    cache: dict[tuple[str, str, str], tuple[str, ...]],
) -> tuple[str, ...]:
    key = (_norm_rel_path(source_file), _norm_rel_path(proof_file or ""), theorem_label)
    if key in cache:
        return cache[key]

    candidates: list[Path] = []
    if proof_file and proof_file.lower() != "null":
        candidates.append(chapter_root / proof_file)
    if source_file:
        candidates.append(chapter_root / source_file)

    for path in candidates:
        if not path.exists():
            continue
        labels = _dependency_labels_from_file(path)
        if labels:
            cache[key] = labels
            return labels

    cache[key] = ()
    return ()


def _dependency_labels_from_file(path: Path) -> tuple[str, ...]:
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return ()

    for title, body in _REMARK_RE.findall(text):
        if title.strip().lower() != "dependencies":
            continue
        labels: list[str] = []
        for label in _HYPERREF_RE.findall(body):
            clean = label.strip()
            if clean and clean not in labels:
                labels.append(clean)
        if labels:
            return tuple(labels)
        lowered = body.lower()
        if "no local dependencies" in lowered:
            return ()
    return ()


def _order_items_with_dependencies(items: list[ProofTodo]) -> list[ProofTodo]:
    items_by_label = {item.label: item for item in items}
    in_degree = {item.label: 0 for item in items}
    outgoing: dict[str, set[str]] = {item.label: set() for item in items}

    for item in items:
        for dep in item.dependency_labels:
            if dep not in items_by_label:
                continue
            if item.label in outgoing[dep]:
                continue
            outgoing[dep].add(item.label)
            in_degree[item.label] += 1

    available = [
        item for item in items
        if in_degree[item.label] == 0
    ]
    ordered: list[ProofTodo] = []
    seen: set[str] = set()

    while available:
        available.sort(key=_todo_sort_key)
        current = available.pop(0)
        if current.label in seen:
            continue
        seen.add(current.label)
        ordered.append(current)
        for follower_label in sorted(outgoing[current.label], key=lambda lbl: _todo_sort_key(items_by_label[lbl])):
            in_degree[follower_label] -= 1
            if in_degree[follower_label] == 0:
                available.append(items_by_label[follower_label])

    if len(ordered) == len(items):
        return ordered

    remaining = [item for item in items if item.label not in seen]
    remaining.sort(key=_todo_sort_key)
    ordered.extend(remaining)
    return ordered


def _todo_sort_key(item: ProofTodo) -> tuple[int, int, int, str, str]:
    return (
        item.note_order,
        item.label_order,
        item.chapter_order,
        item.source_file,
        item.label,
    )
