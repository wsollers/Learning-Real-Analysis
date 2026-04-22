"""
Patch a validated generated statement block into live source.

This is deliberately conservative:
  - generated block must pass deterministic validation
  - target label must exist in chapter.yaml
  - target source must contain exactly one matching label
  - replacement includes the formal environment or enclosing tcolorbox plus
    immediately following remark* support blocks
"""

from __future__ import annotations

import difflib
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml

from auditor import config
from auditor.config import THEOREM_LIKE_ENVS
from auditor.report import write_report
from auditor.validators.generated_block import (
    format_generated_validation_report,
    validate_generated_file,
)


@dataclass(frozen=True)
class GeneratedPatchResult:
    result: str
    label: str
    source_path: Path | None
    generated_path: Path
    validation: dict[str, Any]
    diff_path: Path | None = None
    message: str = ""


@dataclass(frozen=True)
class StatementGroupSpan:
    start: int
    end: int
    preserved_boundary: str = ""


def patch_generated(
    chapter_path: Path,
    label: str,
    generated_path: Path,
    *,
    apply: bool = False,
    expected_type: str | None = None,
    out_dir: Path | None = None,
) -> GeneratedPatchResult:
    chapter_root = _resolve_chapter_root(chapter_path)
    generated_path = generated_path.resolve()
    chapter_yaml = _load_chapter_yaml(chapter_root)
    entry = _find_environment_entry(chapter_yaml, label)
    if not entry:
        return GeneratedPatchResult(
            result="FAIL",
            label=label,
            source_path=None,
            generated_path=generated_path,
            validation={},
            message=f"Label `{label}` was not found in {chapter_root / 'chapter.yaml'}.",
        )

    artifact_type = expected_type or entry.get("type")
    validation = validate_generated_file(
        generated_path,
        artifact_type=artifact_type,
        expected_label=label,
    )
    if validation["result"] != "PASS":
        return GeneratedPatchResult(
            result="FAIL",
            label=label,
            source_path=chapter_root / entry["file"],
            generated_path=generated_path,
            validation=validation,
            message="Generated block failed deterministic validation.",
        )

    source_path = (chapter_root / entry["file"]).resolve()
    source_text = source_path.read_text(encoding="utf-8")
    span = _find_statement_group_span(source_text, label)
    if not span:
        return GeneratedPatchResult(
            result="FAIL",
            label=label,
            source_path=source_path,
            generated_path=generated_path,
            validation=validation,
            message=f"Could not locate replaceable statement group for `{label}`.",
        )

    start, end = span.start, span.end
    old_block = source_text[start:end].strip()
    new_block = generated_path.read_text(encoding="utf-8-sig").strip()
    replacement_block = new_block
    if span.preserved_boundary:
        replacement_block = replacement_block + "\n\n" + span.preserved_boundary.strip()

    diff_text = _unified_diff(
        old_block,
        replacement_block,
        fromfile=str(source_path),
        tofile=str(generated_path),
    )
    diff_path = _write_patch_artifacts(
        label=label,
        out_dir=out_dir,
        diff_text=diff_text,
        validation=validation,
    )

    if old_block == replacement_block:
        return GeneratedPatchResult(
            result="UNCHANGED",
            label=label,
            source_path=source_path,
            generated_path=generated_path,
            validation=validation,
            diff_path=diff_path,
            message="Live source already matches the generated block.",
        )

    patched = source_text[:start] + replacement_block + "\n" + source_text[end:]

    if apply:
        source_path.write_text(patched, encoding="utf-8")
        message = "Applied generated block to live source."
        result = "APPLIED"
    else:
        message = "Dry run only. Review the diff, then rerun with --apply."
        result = "DRY_RUN"

    return GeneratedPatchResult(
        result=result,
        label=label,
        source_path=source_path,
        generated_path=generated_path,
        validation=validation,
        diff_path=diff_path,
        message=message,
    )


def format_generated_patch_result(result: GeneratedPatchResult) -> str:
    lines = [
        "# Generated Patch Result",
        f"- **Result:** {result.result}",
        f"- **Label:** `{result.label}`",
        f"- **Generated:** `{result.generated_path}`",
    ]
    if result.source_path:
        lines.append(f"- **Source:** `{result.source_path}`")
    if result.diff_path:
        lines.append(f"- **Diff:** `{result.diff_path}`")
    lines += ["", result.message, ""]

    if result.validation:
        lines += ["## Validation", "", format_generated_validation_report(result.validation), ""]
    return "\n".join(lines)


def _resolve_chapter_root(chapter_path: Path) -> Path:
    path = chapter_path.resolve()
    if not path.is_absolute():
        path = (config.REPO_ROOT / chapter_path).resolve()
    if not path.exists():
        raise FileNotFoundError(f"Chapter path does not exist: {path}")
    return path


def _load_chapter_yaml(chapter_root: Path) -> dict[str, Any]:
    path = chapter_root / "chapter.yaml"
    if not path.exists():
        raise FileNotFoundError(f"chapter.yaml not found at {path}")
    return yaml.safe_load(path.read_text(encoding="utf-8")) or {}


def _find_environment_entry(chapter_yaml: dict[str, Any], label: str) -> dict[str, Any] | None:
    matches = [
        entry for entry in chapter_yaml.get("environments", [])
        if entry.get("label") == label
    ]
    if len(matches) != 1:
        return None
    return matches[0]


def _find_statement_group_span(tex: str, label: str) -> StatementGroupSpan | None:
    if len(re.findall(r"\\label\{" + re.escape(label) + r"\}", tex)) != 1:
        return None

    env_span = _find_env_span(tex, label)
    if not env_span:
        return None
    start, end, _env = env_span
    start, end = _expand_to_enclosing_tcolorbox(tex, start, end)
    end = _include_following_remarks(tex, end)
    end, preserved_boundary = _include_interstitial_figure_boundary(tex, end)
    return StatementGroupSpan(start=start, end=end, preserved_boundary=preserved_boundary)


def _find_env_span(tex: str, label: str) -> tuple[int, int, str] | None:
    label_match = re.search(r"\\label\{" + re.escape(label) + r"\}", tex)
    if not label_match:
        return None

    begin_re = re.compile(
        r"\\begin\{(" + "|".join(THEOREM_LIKE_ENVS) + r")\}(?:\[[^\]]*\])?",
        re.DOTALL,
    )
    begins = [match for match in begin_re.finditer(tex[: label_match.start()])]
    if not begins:
        return None

    begin = begins[-1]
    env = begin.group(1)
    end_match = re.search(r"\\end\{" + re.escape(env) + r"\}", tex[label_match.end():])
    if not end_match:
        return None
    end = label_match.end() + end_match.end()
    return begin.start(), end, env


def _expand_to_enclosing_tcolorbox(tex: str, start: int, end: int) -> tuple[int, int]:
    opens = list(re.finditer(r"\\begin\{tcolorbox\}(?:\[[^\]]*\])?", tex[:start], re.DOTALL))
    closes_before = list(re.finditer(r"\\end\{tcolorbox\}", tex[:start]))
    if not opens:
        return start, end
    last_open = opens[-1]
    last_close_before = closes_before[-1] if closes_before else None
    if last_close_before and last_close_before.end() > last_open.start():
        return start, end

    close_after = re.search(r"\\end\{tcolorbox\}", tex[end:])
    if not close_after:
        return start, end
    return last_open.start(), end + close_after.end()


def _include_following_remarks(tex: str, pos: int) -> int:
    cursor = pos
    while cursor < len(tex):
        whitespace = re.match(r"(?:\s|%[^\n]*(?:\n|$))*", tex[cursor:])
        cursor_after_ws = cursor + (whitespace.end() if whitespace else 0)
        remark = re.match(
            r"\\begin\{remark\*\}(?:\[[^\]]*\])?.*?\\end\{remark\*\}",
            tex[cursor_after_ws:],
            re.DOTALL,
        )
        if not remark:
            return cursor
        cursor = cursor_after_ws + remark.end()
    return cursor


def _include_interstitial_figure_boundary(tex: str, pos: int) -> tuple[int, str]:
    """Preserve figure inputs between adjacent formal blocks as boundary content.

    House rule: a figure input between two theorem-like environments belongs between
    statement groups, not inside either group. If stale remark blocks appear after
    such a figure and before the next theorem-like environment, replace those stale
    remarks while keeping the figure after the generated replacement block.
    """
    cursor = pos
    before_figure_ws = re.match(r"(?:\s|%[^\n]*(?:\n|$))*", tex[cursor:])
    figure_start = cursor + (before_figure_ws.end() if before_figure_ws else 0)

    figure_cursor = figure_start
    figures: list[str] = []
    while True:
        figure = re.match(r"\\input\{[^{}]*figure[^{}]*\}", tex[figure_cursor:])
        if not figure:
            break
        figures.append(figure.group(0))
        figure_cursor += figure.end()
        between = re.match(r"(?:\s|%[^\n]*(?:\n|$))*", tex[figure_cursor:])
        figure_cursor += between.end() if between else 0

    if not figures:
        return pos, ""

    stale_cursor = figure_cursor
    stale_count = 0
    while True:
        whitespace = re.match(r"(?:\s|%[^\n]*(?:\n|$))*", tex[stale_cursor:])
        cursor_after_ws = stale_cursor + (whitespace.end() if whitespace else 0)
        remark = re.match(
            r"\\begin\{remark\*\}(?:\[[^\]]*\])?.*?\\end\{remark\*\}",
            tex[cursor_after_ws:],
            re.DOTALL,
        )
        if not remark:
            stale_cursor = cursor_after_ws
            break
        stale_cursor = cursor_after_ws + remark.end()
        stale_count += 1

    if stale_count == 0:
        return pos, ""

    next_token = tex[stale_cursor:]
    next_is_formal = re.match(
        r"(?:\s|%[^\n]*(?:\n|$))*"
        r"(?:\\begin\{tcolorbox\}(?:\[[^\]]*\])?\s*)?"
        r"\\begin\{(" + "|".join(THEOREM_LIKE_ENVS) + r")\}(?:\[[^\]]*\])?",
        next_token,
        re.DOTALL,
    )
    if not next_is_formal:
        return pos, ""

    return stale_cursor, "\n\n".join(figures)


def _unified_diff(old: str, new: str, *, fromfile: str, tofile: str) -> str:
    lines = list(
        difflib.unified_diff(
            old.splitlines(),
            new.splitlines(),
            fromfile=fromfile,
            tofile=tofile,
            lineterm="",
        )
    )
    return "\n".join(lines) + ("\n" if lines else "")


def _write_patch_artifacts(
    *,
    label: str,
    out_dir: Path | None,
    diff_text: str,
    validation: dict[str, Any],
) -> Path:
    target_dir = out_dir or (config.REPO_ROOT / "scripts" / "tmp" / "generated-patches")
    target_dir.mkdir(parents=True, exist_ok=True)
    safe = label.replace(":", "-").replace("/", "-").replace("\\", "-")
    diff_path = target_dir / f"{safe}.diff"
    validation_path = target_dir / f"{safe}.validation.md"
    write_report(diff_text, diff_path)
    write_report(format_generated_validation_report(validation), validation_path)
    return diff_path
