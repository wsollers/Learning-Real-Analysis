"""
Batch helper for generated statement replacements.

Plan format may be JSON or YAML:

chapter_path: volume-iii/analysis/bounding
volume: volume-iii
chapter: bounding
items:
  - label: def:lower-bound
    type: def
    subject: "Lower bound of a subset..."
    generated: scripts/tmp/prompt-runs/lower-bound-definition-generated.tex
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml

from auditor import config
from auditor.generators.statement import generate_statement
from auditor.patchers.generated import (
    GeneratedPatchResult,
    format_generated_patch_result,
    patch_generated,
)
from auditor.report import write_report
from auditor.validators.generated_block import validate_generated_file


@dataclass
class BatchItemResult:
    label: str
    status: str
    generated_path: Path | None = None
    diff_path: Path | None = None
    message: str = ""


@dataclass
class BatchResult:
    mode: str
    plan_path: Path
    out_dir: Path
    items: list[BatchItemResult] = field(default_factory=list)


def run_generated_batch(
    plan_path: Path,
    *,
    apply: bool = False,
    generate_missing: bool = False,
    out_dir: Path | None = None,
) -> BatchResult:
    plan_path = plan_path.resolve()
    plan = _load_plan(plan_path)
    chapter_path = Path(plan.get("chapter_path") or plan.get("chapterPath") or "")
    if not chapter_path:
        raise ValueError("Batch plan must provide chapter_path.")
    if not chapter_path.is_absolute():
        chapter_path = config.REPO_ROOT / chapter_path

    chapter = plan.get("chapter") or chapter_path.name
    volume = plan.get("volume")
    items = plan.get("items") or []
    if not isinstance(items, list) or not items:
        raise ValueError("Batch plan must provide a nonempty items list.")

    plan_out_dir = plan.get("out_dir") or plan.get("outDir")
    target_dir = (
        out_dir
        if out_dir
        else Path(plan_out_dir)
        if plan_out_dir
        else config.REPO_ROOT / "scripts" / "tmp" / "generated-batches" / plan_path.stem
    )
    if not target_dir.is_absolute():
        target_dir = config.REPO_ROOT / target_dir
    generated_dir = target_dir / "generated"
    patch_dir = target_dir / "patches"
    target_dir.mkdir(parents=True, exist_ok=True)
    generated_dir.mkdir(parents=True, exist_ok=True)
    patch_dir.mkdir(parents=True, exist_ok=True)
    _write_command_list(
        plan=plan,
        plan_path=plan_path,
        target_dir=target_dir,
        generated_dir=generated_dir,
    )

    results = BatchResult(
        mode="APPLY" if apply else "DRY_RUN",
        plan_path=plan_path,
        out_dir=target_dir,
    )

    for item in items:
        result = _process_item(
            item,
            chapter_path=chapter_path,
            chapter=chapter,
            volume=volume,
            generated_dir=generated_dir,
            patch_dir=patch_dir,
            apply=apply,
            generate_missing=generate_missing,
        )
        results.items.append(result)
        write_report(format_batch_result(results), target_dir / "summary.md")
        write_report(_batch_result_json(results), target_dir / "summary.json")

    return results


def format_batch_result(result: BatchResult) -> str:
    lines = [
        "# Generated Batch Patch",
        f"- **Mode:** {result.mode}",
        f"- **Plan:** `{result.plan_path}`",
        f"- **Output:** `{result.out_dir}`",
        f"- **Items:** {len(result.items)}",
        "",
        "| Label | Status | Generated | Diff | Message |",
        "|-------|--------|-----------|------|---------|",
    ]
    for item in result.items:
        generated = f"`{item.generated_path}`" if item.generated_path else ""
        diff = f"`{item.diff_path}`" if item.diff_path else ""
        message = item.message.replace("|", "\\|").replace("\n", " ")
        lines.append(f"| `{item.label}` | {item.status} | {generated} | {diff} | {message} |")
    lines.append("")
    return "\n".join(lines)


def _process_item(
    item: dict[str, Any],
    *,
    chapter_path: Path,
    chapter: str,
    volume: str | None,
    generated_dir: Path,
    patch_dir: Path,
    apply: bool,
    generate_missing: bool,
) -> BatchItemResult:
    label = item.get("label")
    artifact_type = item.get("type")
    subject = item.get("subject")
    if not label:
        return BatchItemResult(label="<missing>", status="ERROR", message="Item is missing label.")
    if not artifact_type:
        return BatchItemResult(label=label, status="ERROR", message="Item is missing type.")

    generated_path = _generated_path(item, generated_dir, label)
    if not generated_path.exists():
        if not generate_missing:
            return BatchItemResult(
                label=label,
                status="MISSING_GENERATED",
                generated_path=generated_path,
                message="Generated file is missing; rerun with --generate-missing.",
            )
        if not subject:
            return BatchItemResult(
                label=label,
                status="ERROR",
                generated_path=generated_path,
                message="Cannot generate missing file because item has no subject.",
            )
        try:
            latex = generate_statement(
                artifact_type=artifact_type,
                content_description=subject,
                chapter_subject=chapter,
                chapter_registry=_load_chapter_registry(volume) if volume else None,
                label=label,
            )
        except Exception as exc:
            return BatchItemResult(
                label=label,
                status="GENERATION_ERROR",
                generated_path=generated_path,
                message=str(exc),
            )
        generated_path.parent.mkdir(parents=True, exist_ok=True)
        generated_path.write_text(latex, encoding="utf-8")

    validation = validate_generated_file(
        generated_path,
        artifact_type=artifact_type,
        expected_label=label,
    )
    if validation["result"] != "PASS" and generate_missing and subject:
        try:
            latex = _regenerate_after_validation_failure(
                artifact_type=artifact_type,
                subject=subject,
                chapter=chapter,
                volume=volume,
                label=label,
                validation=validation,
            )
        except Exception as exc:
            return BatchItemResult(
                label=label,
                status="GENERATION_ERROR",
                generated_path=generated_path,
                message=f"Retry generation failed: {exc}",
            )
        generated_path.write_text(latex, encoding="utf-8")
        validation = validate_generated_file(
            generated_path,
            artifact_type=artifact_type,
            expected_label=label,
        )
    if validation["result"] != "PASS":
        return BatchItemResult(
            label=label,
            status="VALIDATION_FAIL",
            generated_path=generated_path,
            message=f"Generated block failed validation with {validation['summary']['failures']} failure(s).",
        )

    patch_result = patch_generated(
        chapter_path=chapter_path,
        label=label,
        generated_path=generated_path,
        apply=apply,
        expected_type=artifact_type,
        out_dir=patch_dir,
    )
    safe_label = _safe(label)
    write_report(
        format_generated_patch_result(patch_result),
        patch_dir / f"{safe_label}.patch-result.md",
    )
    if patch_result.result == "FAIL":
        status = "PATCH_FAIL"
    elif patch_result.result == "APPLIED":
        status = "APPLIED"
    elif patch_result.result == "UNCHANGED":
        status = "UNCHANGED"
    else:
        status = "DRY_RUN"
    return BatchItemResult(
        label=label,
        status=status,
        generated_path=generated_path,
        diff_path=patch_result.diff_path,
        message=patch_result.message,
    )


def _regenerate_after_validation_failure(
    *,
    artifact_type: str,
    subject: str,
    chapter: str,
    volume: str | None,
    label: str,
    validation: dict[str, Any],
) -> str:
    issues = "; ".join(
        f"{finding.get('code')}: {finding.get('message')}"
        for finding in validation.get("findings", [])
    )
    repair_subject = (
        f"{subject}\n\n"
        "The previous generated block failed deterministic validation. "
        "Regenerate the entire block from scratch and fix every issue below. "
        "Use plain ASCII punctuation in prose. Use canonical predicate readings "
        "with \\operatorname{...}; do not use undefined predicate macros such as "
        "\\UpperBound, \\LowerBound, \\Supremum, or \\Infimum. Include all required "
        "definition support blocks, especially Failure modes and Failure mode decomposition.\n\n"
        f"Validation issues: {issues}"
    )
    return generate_statement(
        artifact_type=artifact_type,
        content_description=repair_subject,
        chapter_subject=chapter,
        chapter_registry=_load_chapter_registry(volume) if volume else None,
        label=label,
    )


def _load_plan(plan_path: Path) -> dict[str, Any]:
    text = plan_path.read_text(encoding="utf-8")
    if plan_path.suffix.lower() == ".json":
        return json.loads(text)
    return yaml.safe_load(text) or {}


def _generated_path(item: dict[str, Any], generated_dir: Path, label: str) -> Path:
    explicit = item.get("generated")
    if explicit:
        path = Path(explicit)
        return path if path.is_absolute() else config.REPO_ROOT / path
    return generated_dir / f"{_safe(label)}.tex"


def _safe(label: str) -> str:
    return label.replace(":", "-").replace("/", "-").replace("\\", "-")


def _load_chapter_registry(volume: str | None) -> list[dict]:
    if not volume:
        return []
    volume_yaml = config.REPO_ROOT / volume / "chapter.yaml"
    if not volume_yaml.exists():
        return []
    data = yaml.safe_load(volume_yaml.read_text(encoding="utf-8")) or {}
    return data.get("chapters", [])


def _batch_result_json(result: BatchResult) -> str:
    payload = {
        "mode": result.mode,
        "plan_path": str(result.plan_path),
        "out_dir": str(result.out_dir),
        "items": [
            {
                "label": item.label,
                "status": item.status,
                "generated_path": str(item.generated_path) if item.generated_path else None,
                "diff_path": str(item.diff_path) if item.diff_path else None,
                "message": item.message,
            }
            for item in result.items
        ],
    }
    return json.dumps(payload, indent=2, ensure_ascii=False)


def _write_command_list(
    *,
    plan: dict[str, Any],
    plan_path: Path,
    target_dir: Path,
    generated_dir: Path,
) -> None:
    chapter = plan.get("chapter") or ""
    volume = plan.get("volume") or ""
    lines = [
        "# Generated Batch Commands",
        "",
        "## Generate Missing Files One At A Time",
        "",
    ]
    for item in plan.get("items") or []:
        label = item.get("label")
        artifact_type = item.get("type")
        subject = item.get("subject")
        if not label or not artifact_type or not subject:
            continue
        out_path = _generated_path(item, generated_dir, label)
        command = (
            "python -m constitution.auditor --repoDir F:\\repos\\Learning-Real-Analysis "
            f"-ai codex generate statement --type {artifact_type} "
            f"--chapter {chapter} --volume {volume} --label {label} "
            f"--subject \"{_escape_powershell_double_quoted(subject)}\" "
            f"--out {out_path} --validate"
        )
        lines += [f"### `{label}`", "", "```powershell", command, "```", ""]

    lines += [
        "## Batch Dry Run",
        "",
        "```powershell",
        f"python -m constitution.auditor patch generated-batch {plan_path}",
        "```",
        "",
        "## Batch Apply",
        "",
        "```powershell",
        f"python -m constitution.auditor patch generated-batch {plan_path} --apply",
        "```",
        "",
        "## Batch Generate Missing And Dry Run",
        "",
        "```powershell",
        f"python -m constitution.auditor --repoDir F:\\repos\\Learning-Real-Analysis -ai codex patch generated-batch {plan_path} --generate-missing",
        "```",
        "",
    ]
    write_report("\n".join(lines), target_dir / "commands.md")


def _escape_powershell_double_quoted(text: str) -> str:
    return text.replace("`", "``").replace('"', '`"').replace("$", "`$")
