from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


THEOREM_LIKE_ENVS = {
    "definition": "Definition",
    "theorem": "Theorem",
    "lemma": "Lemma",
    "proposition": "Proposition",
    "corollary": "Corollary",
    "axiom": "Axiom",
}

BEGIN_RE = re.compile(
    r"^(?P<indent>\s*)\\begin\{(?P<env>definition|theorem|lemma|proposition|corollary|axiom)\*?\}"
    r"(?:\[(?P<title>[^\]]*)\])?"
)
LABEL_RE = re.compile(r"\\label\{(?P<label>[^}]+)\}")


@dataclass
class CommentBlockResult:
    files_seen: int
    files_changed: int
    blocks_added: int
    changed_files: list[Path]


def add_theorem_comment_blocks(
    path: Path,
    *,
    repo_root: Path,
    apply: bool = False,
    include_exercises: bool = False,
) -> CommentBlockResult:
    """Insert readable LaTeX comment headers above theorem-like environments."""

    tex_files = _iter_tex_files(path, include_exercises=include_exercises)
    changed_files: list[Path] = []
    blocks_added = 0
    files_seen = 0

    for tex_file in tex_files:
        files_seen += 1
        text = tex_file.read_text(encoding="utf-8")
        new_text, added = _add_blocks_to_text(text)
        if added == 0:
            continue

        blocks_added += added
        changed_files.append(_relative_to(tex_file, repo_root))
        if apply:
            tex_file.write_text(new_text, encoding="utf-8")

    return CommentBlockResult(
        files_seen=files_seen,
        files_changed=len(changed_files),
        blocks_added=blocks_added,
        changed_files=changed_files,
    )


def _iter_tex_files(path: Path, *, include_exercises: bool) -> list[Path]:
    if path.is_file():
        return [path] if path.suffix == ".tex" else []

    files = sorted(path.rglob("*.tex"))
    if include_exercises:
        return files

    return [tex_file for tex_file in files if not _is_exercise_or_capstone(tex_file)]


def _is_exercise_or_capstone(path: Path) -> bool:
    parts = {part.lower() for part in path.parts}
    name = path.name.lower()
    return "exercises" in parts or "capstone" in name


def _add_blocks_to_text(text: str) -> tuple[str, int]:
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    added = 0

    for index, line in enumerate(lines):
        match = BEGIN_RE.match(line)
        if match and not _has_existing_block(out):
            env = match.group("env")
            title = (match.group("title") or "").strip()
            label = _find_label(lines, index, env)
            out.extend(_build_comment_block(THEOREM_LIKE_ENVS[env], title, label, _line_ending(line)))
            added += 1
        out.append(line)

    return "".join(out), added


def _has_existing_block(previous_lines: list[str]) -> bool:
    window = "".join(previous_lines[-5:])
    return "% ---------------------------------------------------------" in window and (
        "% Label:" in window or "% Definition:" in window or "% Theorem:" in window
    )


def _find_label(lines: list[str], begin_index: int, env: str) -> str | None:
    end_re = re.compile(rf"^\s*\\end\{{{re.escape(env)}\*?\}}")
    for line in lines[begin_index : min(len(lines), begin_index + 20)]:
        label_match = LABEL_RE.search(line)
        if label_match:
            return label_match.group("label")
        if end_re.match(line):
            return None
    return None


def _build_comment_block(kind: str, title: str, label: str | None, newline: str) -> list[str]:
    name = title or "Untitled"
    lines = [
        "% ---------------------------------------------------------" + newline,
        f"% {kind}: {name}" + newline,
    ]
    if label:
        lines.append(f"% Label: {label}" + newline)
    lines.append("% ---------------------------------------------------------" + newline)
    return lines


def _line_ending(line: str) -> str:
    return "\r\n" if line.endswith("\r\n") else "\n"


def _relative_to(path: Path, root: Path) -> Path:
    try:
        return path.resolve().relative_to(root.resolve())
    except ValueError:
        return path.resolve()
