"""
report.py
Formats audit results for terminal output and timestamped markdown files.
All report writing goes through here.
"""

import json
from datetime import datetime
from pathlib import Path

from auditor import config


# ---------------------------------------------------------------------------
# Timestamp
# ---------------------------------------------------------------------------

def _timestamp() -> str:
    return datetime.now().strftime("%Y-%m-%d_%H-%M-%S")


def _human_timestamp() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


# ---------------------------------------------------------------------------
# Report path construction
# ---------------------------------------------------------------------------

def _safe_label(label: str) -> str:
    return label.replace(":", "-").replace("/", "-").replace("\\", "-")


def report_path(chapter: str, operation: str, label: str) -> Path:
    """
    Constructs the output path for a timestamped report file.
    label is sanitized (colons replaced with dashes) for filesystem safety.
    """
    safe_label = _safe_label(label)
    filename = f"{_timestamp()}_{operation}_{safe_label}.md"
    return config.REPORTS_DIR / chapter / filename


def stable_report_path(output_dir: Path, operation: str, label: str, filename_prefix: str = "") -> Path:
    """
    Constructs a deterministic report filename inside an explicit output dir.
    Used by resumable chapter runs so completed items can be skipped later.
    """
    safe_label = _safe_label(label)
    prefix = f"{filename_prefix}_" if filename_prefix else ""
    return output_dir / f"{prefix}{operation}_{safe_label}.md"


# ---------------------------------------------------------------------------
# Audit report → markdown
# ---------------------------------------------------------------------------

def _status_emoji(status: str) -> str:
    return {
        "PASS":                  "[PASS]",
        "FAIL":                  "[FAIL]",
        "NONCOMPLIANT":          "[WARN]",
        "CONDITIONAL_MET":       "[PASS]",
        "CONDITIONAL_UNMET":     "[-]",
        "CONDITIONAL_VIOLATION": "[FAIL]",
        "DEPENDENT_MET":         "[PASS]",
        "DEPENDENT_UNMET":       "[-]",
        "DEPENDENT_VIOLATION":   "[FAIL]",
        "FORBIDDEN_VIOLATION":   "[FORBIDDEN]",
        "STUB":                  "[STUB]",
    }.get(status, status)


def _overall_result(report: dict) -> str:
    s = report["summary"]
    hard_failures = (
        s.get("failed", 0)
        + s.get("noncompliant", 0)
        + s.get("conditional_violation", 0)
        + s.get("dependent_violation", 0)
        + s.get("forbidden_violation", 0)
    )
    flags = len(report.get("special_flags", []))
    if hard_failures == 0 and flags == 0:
        return "PASS"
    elif hard_failures == 0:
        return f"PASS WITH FLAGS ({flags} flags)"
    else:
        return f"FAIL ({hard_failures} violations, {flags} flags)"


def format_audit_report_markdown(
    report: dict,
    chapter: str,
    operation: str,
) -> str:
    """
    Converts an audit report dict to a markdown string.
    This is the canonical report format written to disk and printed to terminal.
    """
    label   = report.get("label", "unknown")
    result  = _overall_result(report)
    s       = report["summary"]

    lines: list[str] = []

    # --- Header ---
    lines += [
        "# Audit Report",
        f"- **Operation:** {operation}",
        f"- **Artifact:** {label}",
        f"- **Artifact Type:** {report.get('artifact_type', '?')}",
        f"- **Chapter:** {chapter}",
        f"- **Timestamp:** {_human_timestamp()}",
        f"- **Result:** {result}",
        "",
    ]

    # --- Summary table ---
    lines += [
        "## Summary",
        "",
        "| Status | Count |",
        "|--------|-------|",
        f"| [PASS] Pass | {s.get('passed', 0) + s.get('conditional_met', 0) + s.get('dependent_met', 0)} |",
        f"| [FAIL] Fail | {s.get('failed', 0)} |",
        f"| [WARN] Noncompliant | {s.get('noncompliant', 0)} |",
        f"| [FAIL] Conditional violation | {s.get('conditional_violation', 0)} |",
        f"| [FAIL] Dependent violation | {s.get('dependent_violation', 0)} |",
        f"| [FORBIDDEN] Forbidden violation | {s.get('forbidden_violation', 0)} |",
        f"| [-] Conditional unmet (correct) | {s.get('conditional_unmet', 0)} |",
        f"| [-] Dependent unmet (correct) | {s.get('dependent_unmet', 0)} |",
        f"| [STUB] Stub | {'Yes' if s.get('stub') else 'No'} |",
        "",
    ]

    # --- Violations ---
    violations = report.get("violations", [])
    if violations:
        lines += ["## Violations", ""]
        for v in violations:
            lines += [
                f"### {_status_emoji(v['status'])} `{v['block_id']}` - {v['status']}",
                "",
                v.get("finding", ""),
                "",
            ]
    else:
        lines += ["## Violations", "", "_None._", ""]

    # --- Special flags ---
    flags = report.get("special_flags", [])
    if flags:
        lines += ["## Special Flags", ""]
        for f in flags:
            lines += [
                f"### [FLAG] {f['flag_type']}",
                "",
                f.get("description", ""),
                "",
            ]
            if f.get("location"):
                lines += [f"**Location:** `{f['location']}`", ""]
            if f.get("suggested_form"):
                lines += [
                    "**Suggested canonical form:**",
                    "```yaml",
                    f["suggested_form"],
                    "```",
                    "",
                ]
    else:
        lines += ["## Special Flags", "", "_None._", ""]

    # --- Full check table ---
    lines += [
        "## Full Check Table",
        "",
        "| Block | Requirement | Status | Finding |",
        "|-------|-------------|--------|---------|",
    ]
    for check in report.get("checks", []):
        block_id    = check.get("block_id", "?")
        requirement = check.get("requirement", "?")
        status      = check.get("status", "?")
        finding     = check.get("finding", "").replace("\n", " ").replace("|", "\\|")
        emoji       = _status_emoji(status)
        lines.append(
            f"| `{block_id}` | {requirement} | {emoji} {status} | {finding} |"
        )

    lines.append("")

    # --- Trigger evaluations ---
    trigger_checks = [
        c for c in report.get("checks", [])
        if c.get("trigger_evaluation") or c.get("proof_usage_evaluation")
    ]
    if trigger_checks:
        lines += ["## Trigger and Proof-Usage Evaluations", ""]
        for c in trigger_checks:
            lines += [f"### `{c['block_id']}`", ""]
            if c.get("trigger_evaluation"):
                lines += [f"**Trigger:** {c['trigger_evaluation']}", ""]
            if c.get("proof_usage_evaluation"):
                lines += [f"**Proof usage:** {c['proof_usage_evaluation']}", ""]

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Symbol audit report (already markdown — just write it)
# ---------------------------------------------------------------------------

def format_symbol_audit_header(chapter: str) -> str:
    return (
        f"# Symbol Audit Report\n"
        f"- **Chapter:** {chapter}\n"
        f"- **Timestamp:** {_human_timestamp()}\n\n"
        f"---\n\n"
    )


# ---------------------------------------------------------------------------
# True-up report
# ---------------------------------------------------------------------------

def format_trueup_report_markdown(
    trueup,   # TrueUpReport
    chapter: str,
    scan_warnings: list[str],
) -> str:
    lines = [
        "# True-Up Report",
        f"- **Chapter:** {chapter}",
        f"- **Timestamp:** {_human_timestamp()}",
        f"- **Result:** {'CLEAN' if trueup.clean else 'DISCREPANCIES FOUND'}",
        "",
    ]

    if trueup.added:
        lines += ["## Added (in scan, not in chapter.yaml)", ""]
        for lbl in trueup.added:
            lines += [f"- `{lbl}`"]
        lines.append("")

    if trueup.removed:
        lines += ["## Removed (in chapter.yaml, not in scan)", ""]
        for lbl in trueup.removed:
            lines += [f"- `{lbl}`"]
        lines.append("")

    if trueup.changed:
        lines += ["## Changed (label present in both, fields differ)", ""]
        for lbl in trueup.changed:
            lines += [f"- `{lbl}`"]
        lines.append("")

    if trueup.clean:
        lines += ["## Result", "", "_chapter.yaml is consistent with scanned content._", ""]

    if scan_warnings:
        lines += ["## Scanner Warnings", ""]
        for w in scan_warnings:
            lines += [f"- {w}"]
        lines.append("")

    lines += [
        "---",
        "",
        "_To apply changes: run `python -m auditor scan chapter <path>` "
        "and review the proposed chapter.yaml before writing._",
    ]

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Write to disk
# ---------------------------------------------------------------------------

def write_report(content: str, path: Path) -> None:
    """
    Writes a report to disk. Creates parent directories if needed.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def save_audit_report(
    report: dict,
    chapter: str,
    operation: str,
    print_report: bool = True,
    output_dir: Path | None = None,
    filename_prefix: str = "",
) -> Path:
    """
    Formats and writes an audit report. Returns the path written.
    Also prints the report to terminal.
    """
    label   = report.get("label", "unknown")
    content = format_audit_report_markdown(report, chapter, operation)
    path    = (
        stable_report_path(output_dir, operation, label, filename_prefix)
        if output_dir
        else report_path(chapter, operation, label)
    )

    write_report(content, path)
    if print_report:
        print(content)
        print(f"\nReport written to: {path}")

    return path


def save_symbol_audit_report(
    markdown: str,
    chapter: str,
) -> Path:
    """
    Writes a symbol audit markdown report. Returns the path written.
    Also prints to terminal.
    """
    content = format_symbol_audit_header(chapter) + markdown
    path    = report_path(chapter, "audit-symbols", chapter)

    write_report(content, path)
    print(content)
    print(f"\nReport written to: {path}")

    return path


def save_trueup_report(
    trueup,
    chapter: str,
    scan_warnings: list[str],
) -> Path:
    content = format_trueup_report_markdown(trueup, chapter, scan_warnings)
    path    = report_path(chapter, "trueup", chapter)

    write_report(content, path)
    print(content)
    print(f"\nReport written to: {path}")

    return path
