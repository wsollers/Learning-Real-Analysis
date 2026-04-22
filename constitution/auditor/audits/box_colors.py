"""
audits/box_colors.py
Deterministic audit for tcolorbox color usage.

Checks that colback/colframe values use color names defined in common/colors.tex
or explicitly allowed xcolor expressions such as gray!6.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from auditor import config
from auditor.report import _human_timestamp, report_path, write_report


_DEFINECOLOR = re.compile(r"\\definecolor\{([^{}]+)\}")
_TCOLORBOX_OPEN = re.compile(r"\\begin\{tcolorbox\}\s*(?:\[([^\]]*)\])?", re.IGNORECASE | re.DOTALL)
_COLOR_OPT = re.compile(r"\b(colback|colframe)\s*=\s*([^,\]\n]+)")
_LINE_COMMENT = re.compile(r"(?<!\\)%.*$")


def _strip_comments(text: str) -> str:
    return "\n".join(_LINE_COMMENT.sub("", line) for line in text.splitlines())


def load_defined_colors() -> set[str]:
    colors_path = config.REPO_ROOT / "common" / "colors.tex"
    if not colors_path.exists():
        raise FileNotFoundError(f"common/colors.tex not found at {colors_path}")
    text = _strip_comments(colors_path.read_text(encoding="utf-8"))
    return set(_DEFINECOLOR.findall(text))


def _line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def _is_allowed_xcolor_expression(value: str, defined_colors: set[str]) -> bool:
    value = value.strip().strip("{}")
    if value in defined_colors:
        return True
    if re.fullmatch(r"[A-Za-z][A-Za-z0-9]*(?:!\d+)+(?:![A-Za-z][A-Za-z0-9]*)?", value):
        base = value.split("!")[0]
        # Built-in xcolor names are acceptable for neutral structural boxes.
        return base in defined_colors or base in {"black", "white", "gray", "red", "green", "blue", "yellow"}
    return False


def _scan_file(tex_path: Path, root: Path, defined_colors: set[str]) -> list[dict[str, Any]]:
    raw_text = tex_path.read_text(encoding="utf-8")
    text = _strip_comments(raw_text)
    rel = str(tex_path.relative_to(root))
    findings: list[dict[str, Any]] = []

    for match in _TCOLORBOX_OPEN.finditer(text):
        options = match.group(1) or ""
        line = _line_number(text, match.start())
        color_options = dict(_COLOR_OPT.findall(options))

        if not color_options:
            findings.append({
                "status": "WARNING",
                "type": "missing_color_options",
                "file": rel,
                "line": line,
                "message": "tcolorbox has no explicit colback/colframe options.",
            })
            continue

        for key in ("colback", "colframe"):
            value = (color_options.get(key) or "").strip().strip("{}")
            if not value:
                findings.append({
                    "status": "WARNING",
                    "type": "missing_color_option",
                    "file": rel,
                    "line": line,
                    "option": key,
                    "message": f"tcolorbox is missing {key}.",
                })
                continue
            if not _is_allowed_xcolor_expression(value, defined_colors):
                findings.append({
                    "status": "FAIL",
                    "type": "undefined_box_color",
                    "file": rel,
                    "line": line,
                    "option": key,
                    "value": value,
                    "message": f"{key} uses '{value}', which is not defined in common/colors.tex.",
                })
    return findings


def audit_box_colors(path: Path) -> dict[str, Any]:
    target = path.resolve()
    root = config.REPO_ROOT
    if target.is_file():
        tex_files = [target]
        report_name = target.stem
    else:
        tex_files = sorted(target.rglob("*.tex"))
        report_name = target.name

    defined_colors = load_defined_colors()
    findings: list[dict[str, Any]] = []
    for tex_path in tex_files:
        findings.extend(_scan_file(tex_path, root, defined_colors))

    report = {
        "target": str(target),
        "timestamp": _human_timestamp(),
        "defined_colors": sorted(defined_colors),
        "summary": {
            "files_scanned": len(tex_files),
            "findings": len(findings),
            "failures": sum(1 for finding in findings if finding["status"] == "FAIL"),
            "warnings": sum(1 for finding in findings if finding["status"] == "WARNING"),
        },
        "findings": findings,
    }

    chapter = report_name if target.is_dir() else target.parent.name
    md_path = report_path(chapter, "box-colors", report_name)
    json_path = md_path.with_suffix(".json")
    write_report(format_box_color_report(report), md_path)
    import json
    write_report(json.dumps(report, indent=2, ensure_ascii=False), json_path)

    print(format_box_color_report(report))
    print(f"\nBox color audit markdown written to: {md_path}")
    print(f"Box color audit JSON written to: {json_path}")
    return report


def format_box_color_report(report: dict[str, Any]) -> str:
    summary = report["summary"]
    lines = [
        "# Box Color Audit",
        f"- **Target:** `{report['target']}`",
        f"- **Timestamp:** {report['timestamp']}",
        f"- **Files scanned:** {summary['files_scanned']}",
        f"- **Failures:** {summary['failures']}",
        f"- **Warnings:** {summary['warnings']}",
        "",
    ]

    findings = report.get("findings", [])
    if not findings:
        lines += ["_No box color findings._", ""]
        return "\n".join(lines)

    for finding in findings:
        lines += [
            f"## {finding['status']}: {finding['type']}",
            "",
            finding["message"],
            "",
            f"- **File:** `{finding['file']}`",
            f"- **Line:** {finding['line']}",
        ]
        if finding.get("option"):
            lines.append(f"- **Option:** `{finding['option']}`")
        if finding.get("value"):
            lines.append(f"- **Value:** `{finding['value']}`")
        lines.append("")
    return "\n".join(lines)
