"""
audits/chapter.py
Orchestrates a full chapter audit:
  1. Reads chapter.yaml for the manifest.
  2. Audits every environment listed.
  3. Audits every proof file listed.
  4. Writes a chapter-level summary report.

Does not run the symbol audit (that is a separate explicit command).
"""

from pathlib import Path

import yaml

from auditor.audits.statement import audit_statement_from_yaml_entry
from auditor.audits.proof import audit_proof_from_yaml_entry
from auditor.report import (
    save_audit_report,
    report_path,
    write_report,
    format_audit_report_markdown,
    _human_timestamp,
    _overall_result,
)
from auditor.config import REPORTS_DIR


# ---------------------------------------------------------------------------
# Chapter summary report
# ---------------------------------------------------------------------------

def _chapter_summary_markdown(
    chapter: str,
    env_reports: list[dict],
    proof_reports: list[dict],
) -> str:
    from datetime import datetime

    lines = [
        "# Chapter Audit Summary",
        f"- **Chapter:** {chapter}",
        f"- **Timestamp:** {_human_timestamp()}",
        "",
    ]

    total_violations = 0
    total_flags = 0

    # Environment results table
    lines += ["## Environment Audits", "", "| Label | Type | Result |", "|-------|------|--------|"]
    for r in env_reports:
        label  = r.get("label", "?")
        atype  = r.get("artifact_type", "?")
        result = _overall_result(r)
        s = r.get("summary", {})
        hard = (
            s.get("failed", 0)
            + s.get("noncompliant", 0)
            + s.get("conditional_violation", 0)
            + s.get("dependent_violation", 0)
            + s.get("forbidden_violation", 0)
        )
        total_violations += hard
        total_flags += len(r.get("special_flags", []))
        lines.append(f"| `{label}` | {atype} | {result} |")

    lines.append("")

    # Proof results table
    lines += ["## Proof File Audits", "", "| Label | Result |", "|-------|--------|"]
    for r in proof_reports:
        label  = r.get("label", "?")
        result = _overall_result(r)
        s = r.get("summary", {})
        hard = (
            s.get("failed", 0)
            + s.get("noncompliant", 0)
            + s.get("conditional_violation", 0)
            + s.get("dependent_violation", 0)
            + s.get("forbidden_violation", 0)
        )
        total_violations += hard
        total_flags += len(r.get("special_flags", []))
        lines.append(f"| `{label}` | {result} |")

    lines.append("")

    # Overall
    if total_violations == 0 and total_flags == 0:
        overall = "✅ CHAPTER CLEAN"
    elif total_violations == 0:
        overall = f"✅ PASS WITH FLAGS — {total_flags} flags across chapter"
    else:
        overall = f"❌ CHAPTER FAIL — {total_violations} violations, {total_flags} flags"

    lines += [
        "## Overall",
        "",
        overall,
        "",
        "_Individual report files contain full check tables and violation details._",
    ]

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def audit_chapter(chapter_path: Path) -> None:
    """
    Runs a full structural audit of all environments and proof files
    listed in chapter.yaml.

    Writes individual reports for each item and a chapter summary report.
    Prints summary to terminal.
    """
    chapter_root = chapter_path.resolve()
    chapter      = chapter_root.name

    # Load chapter.yaml
    yaml_path = chapter_root / "chapter.yaml"
    if not yaml_path.exists():
        raise FileNotFoundError(
            f"chapter.yaml not found at {yaml_path}. "
            f"Run `python -m auditor scan chapter {chapter_path}` first."
        )

    with open(yaml_path, encoding="utf-8") as f:
        chapter_yaml = yaml.safe_load(f)

    environments = chapter_yaml.get("environments", [])
    proof_files  = chapter_yaml.get("proof_files", [])

    if not environments and not proof_files:
        print(
            f"chapter.yaml for '{chapter}' has no environments or proof_files listed.\n"
            f"Run `python -m auditor scan chapter {chapter_path}` to populate it."
        )
        return

    env_reports: list[dict] = []
    proof_reports: list[dict] = []

    # --- Audit environments ---
    print(f"\nAuditing {len(environments)} environment(s) in '{chapter}'...")
    for entry in environments:
        label = entry.get("label", "?")
        print(f"  → {label}")
        try:
            report = audit_statement_from_yaml_entry(entry, chapter_root, chapter)
            env_reports.append(report)
        except Exception as e:
            print(f"    ERROR: {e}")
            env_reports.append({
                "label": label,
                "artifact_type": entry.get("type", "?"),
                "audit_type": "statement",
                "summary": {"failed": 1},
                "checks": [],
                "violations": [{"block_id": "ERROR", "status": "FAIL", "finding": str(e)}],
                "special_flags": [],
            })

    # --- Audit proof files ---
    print(f"\nAuditing {len(proof_files)} proof file(s) in '{chapter}'...")
    for entry in proof_files:
        label = entry.get("label", "?")
        print(f"  → {label}")
        try:
            report = audit_proof_from_yaml_entry(entry, chapter_root, chapter)
            proof_reports.append(report)
        except Exception as e:
            print(f"    ERROR: {e}")
            proof_reports.append({
                "label": label,
                "artifact_type": "proof",
                "audit_type": "proof",
                "summary": {"failed": 1},
                "checks": [],
                "violations": [{"block_id": "ERROR", "status": "FAIL", "finding": str(e)}],
                "special_flags": [],
            })

    # --- Chapter summary ---
    summary_md = _chapter_summary_markdown(chapter, env_reports, proof_reports)
    summary_path = report_path(chapter, "audit-chapter", chapter)
    write_report(summary_md, summary_path)

    print("\n" + summary_md)
    print(f"\nChapter summary written to: {summary_path}")
