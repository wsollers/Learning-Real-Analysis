"""
audits/toolkits.py
Section-level Toolkit box inventory, planning, and deterministic auditing.

Toolkit boxes are not per-statement artifacts. This module keeps them out of
single-environment audits and treats them as section/file planning artifacts.
"""

from __future__ import annotations

import json
import re
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

import yaml

from auditor import client, config, loader
from auditor.report import _human_timestamp, report_path, write_report


_SECTION = re.compile(r"\\(section|subsection|subsubsection)\*?\{([^{}]*)\}")
_ENV_OPEN = re.compile(
    r"\\begin\{(definition|theorem|lemma|proposition|corollary|axiom)\}"
    r"(?:\[([^\]]*)\])?",
    re.IGNORECASE,
)
_LABEL = re.compile(r"\\label\{([a-z]+:[a-z0-9\-]+)\}")
_TCOLORBOX_OPEN = re.compile(r"\\begin\{tcolorbox\}(?:\[([^\]]*)\])?", re.IGNORECASE)
_TCOLORBOX_END = re.compile(r"\\end\{tcolorbox\}", re.IGNORECASE)


@dataclass
class FormalItem:
    label: str
    type: str
    title: str
    line: int


@dataclass
class ToolkitBox:
    line: int
    title: str
    labels_mentioned: list[str]
    excerpt: str


@dataclass
class SectionInventory:
    file: str
    section: str
    start_line: int
    end_line: int
    formal_items: list[FormalItem] = field(default_factory=list)
    existing_toolkit_boxes: list[ToolkitBox] = field(default_factory=list)


def _line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def _section_ranges(text: str) -> list[tuple[str, int, int, int]]:
    matches = list(_SECTION.finditer(text))
    if not matches:
        return [("File", 0, len(text), 1)]

    ranges: list[tuple[str, int, int, int]] = []
    for index, match in enumerate(matches):
        title = match.group(2).strip() or "Untitled"
        start = match.start()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        ranges.append((title, start, end, _line_number(text, start)))
    return ranges


def _find_formal_items(section_text: str, section_start: int, full_text: str) -> list[FormalItem]:
    items: list[FormalItem] = []
    for match in _ENV_OPEN.finditer(section_text):
        env_name = match.group(1).lower()
        title = (match.group(2) or "").strip()
        after = section_text[match.start() : match.start() + 1200]
        label_match = _LABEL.search(after)
        if not label_match:
            continue
        label = label_match.group(1)
        artifact_type = {
            "definition": "def",
            "theorem": "thm",
            "lemma": "lem",
            "proposition": "prop",
            "corollary": "cor",
            "axiom": "ax",
        }.get(env_name, env_name)
        items.append(
            FormalItem(
                label=label,
                type=artifact_type,
                title=title or label,
                line=_line_number(full_text, section_start + match.start()),
            )
        )
    return items


def _find_toolkit_boxes(section_text: str, section_start: int, full_text: str) -> list[ToolkitBox]:
    boxes: list[ToolkitBox] = []
    for match in _TCOLORBOX_OPEN.finditer(section_text):
        end_match = _TCOLORBOX_END.search(section_text, match.end())
        if not end_match:
            continue
        box_text = section_text[match.start() : end_match.end()]
        options = match.group(1) or ""
        if "toolkit" not in (options + box_text).lower():
            continue
        title_match = re.search(r"title\s*=\s*\{([^{}]*)\}", options)
        title = title_match.group(1).strip() if title_match else "Toolkit"
        labels = sorted(set(_LABEL.findall(box_text) + re.findall(r"\\hyperref\[([a-z]+:[a-z0-9\-]+)\]", box_text)))
        excerpt = " ".join(box_text.split())[:300]
        boxes.append(
            ToolkitBox(
                line=_line_number(full_text, section_start + match.start()),
                title=title,
                labels_mentioned=labels,
                excerpt=excerpt,
            )
        )
    return boxes


def inventory_chapter_toolkits(chapter_path: Path) -> list[SectionInventory]:
    chapter_root = chapter_path.resolve()
    yaml_path = chapter_root / "chapter.yaml"
    if not yaml_path.exists():
        raise FileNotFoundError(f"chapter.yaml not found at {yaml_path}")

    chapter_yaml = yaml.safe_load(yaml_path.read_text(encoding="utf-8")) or {}
    files = sorted({entry["file"] for entry in chapter_yaml.get("environments", [])})

    inventories: list[SectionInventory] = []
    for rel_file in files:
        tex_path = chapter_root / rel_file
        if not tex_path.exists():
            continue
        text = tex_path.read_text(encoding="utf-8")
        for section_title, start, end, line in _section_ranges(text):
            section_text = text[start:end]
            formal_items = _find_formal_items(section_text, start, text)
            toolkit_boxes = _find_toolkit_boxes(section_text, start, text)
            if not formal_items and not toolkit_boxes:
                continue
            inventories.append(
                SectionInventory(
                    file=rel_file,
                    section=section_title,
                    start_line=line,
                    end_line=_line_number(text, end),
                    formal_items=formal_items,
                    existing_toolkit_boxes=toolkit_boxes,
                )
            )
    return inventories


def _inventory_payload(chapter: str, inventories: list[SectionInventory]) -> dict[str, Any]:
    return {
        "chapter": chapter,
        "sections": [
            {
                "file": section.file,
                "section": section.section,
                "start_line": section.start_line,
                "end_line": section.end_line,
                "formal_items": [asdict(item) for item in section.formal_items],
                "existing_toolkit_boxes": [asdict(box) for box in section.existing_toolkit_boxes],
            }
            for section in inventories
        ],
    }


def _write_json_report(payload: dict[str, Any], chapter: str, operation: str) -> Path:
    path = report_path(chapter, operation, chapter).with_suffix(".json")
    write_report(json.dumps(payload, indent=2, ensure_ascii=False), path)
    return path


def _write_markdown_report(content: str, chapter: str, operation: str) -> Path:
    path = report_path(chapter, operation, chapter)
    write_report(content, path)
    return path


def plan_toolkits(chapter_path: Path) -> dict[str, Any]:
    chapter_root = chapter_path.resolve()
    chapter = chapter_root.name
    inventories = inventory_chapter_toolkits(chapter_root)
    payload = _inventory_payload(chapter, inventories)

    system = loader.prompt("plan_toolkits")
    user = (
        "## Chapter Toolkit Inventory\n\n"
        "```json\n"
        f"{json.dumps(payload, indent=2, ensure_ascii=False)}\n"
        "```"
    )
    plan = client.call(system, user, expect_json=True, validate_report=False)
    plan.setdefault("chapter", chapter)

    json_path = _write_json_report(plan, chapter, "toolkit-plan")
    md_path = _write_markdown_report(format_toolkit_plan_markdown(plan), chapter, "toolkit-plan")
    print(f"Toolkit plan JSON written to: {json_path}")
    print(f"Toolkit plan markdown written to: {md_path}")
    return plan


def format_toolkit_plan_markdown(plan: dict[str, Any]) -> str:
    lines = [
        "# Toolkit Plan",
        f"- **Chapter:** {plan.get('chapter', '?')}",
        f"- **Timestamp:** {_human_timestamp()}",
        "",
    ]
    toolkits = plan.get("toolkits", [])
    if not toolkits:
        lines += ["_No toolkit recommendations returned._", ""]
    for toolkit in toolkits:
        lines += [
            f"## {toolkit.get('title', toolkit.get('toolkit_id', 'Toolkit'))}",
            "",
            f"- **ID:** `{toolkit.get('toolkit_id', '')}`",
            f"- **Status:** {toolkit.get('status', '')}",
            f"- **File:** `{toolkit.get('file', '')}`",
            f"- **Section:** {toolkit.get('section', '')}",
            f"- **Placement:** before `{toolkit.get('placement_before', '')}`",
            f"- **Purpose:** {toolkit.get('purpose', '')}",
            "",
            "### Covers",
            "",
        ]
        for label in toolkit.get("covers", []):
            lines.append(f"- `{label}`")
        lines.append("")
    if plan.get("notes"):
        lines += ["## Notes", ""]
        for note in plan["notes"]:
            lines.append(f"- {note}")
        lines.append("")
    return "\n".join(lines)


def _latest_plan_path(chapter: str) -> Path | None:
    reports_dir = config.REPORTS_DIR / chapter
    plans = sorted(reports_dir.glob("*_toolkit-plan_*.json"), key=lambda path: path.stat().st_mtime, reverse=True)
    return plans[0] if plans else None


def audit_toolkits(chapter_path: Path, plan_path: Path | None = None) -> dict[str, Any]:
    chapter_root = chapter_path.resolve()
    chapter = chapter_root.name
    inventories = inventory_chapter_toolkits(chapter_root)
    inventory = _inventory_payload(chapter, inventories)

    plan: dict[str, Any] | None = None
    if plan_path:
        plan = json.loads(plan_path.read_text(encoding="utf-8"))
    else:
        latest = _latest_plan_path(chapter)
        if latest:
            plan = json.loads(latest.read_text(encoding="utf-8"))

    findings: list[dict[str, Any]] = []
    section_map = {(section.file, section.section): section for section in inventories}

    if plan:
        for toolkit in plan.get("toolkits", []):
            file = toolkit.get("file", "")
            section_name = toolkit.get("section", "")
            placement_before = toolkit.get("placement_before", "")
            section = section_map.get((file, section_name))
            if not section:
                findings.append({
                    "status": "FAIL",
                    "type": "missing_section",
                    "toolkit_id": toolkit.get("toolkit_id", ""),
                    "message": f"Planned toolkit section not found: {file} / {section_name}",
                })
                continue

            first_item = next((item for item in section.formal_items if item.label == placement_before), None)
            before_line = first_item.line if first_item else None
            prior_boxes = [
                box for box in section.existing_toolkit_boxes
                if before_line is None or box.line < before_line
            ]
            if toolkit.get("status") == "existing" and not prior_boxes:
                findings.append({
                    "status": "FAIL",
                    "type": "missing_existing_toolkit",
                    "toolkit_id": toolkit.get("toolkit_id", ""),
                    "message": f"Plan says existing, but no toolkit-like box appears before {placement_before}.",
                })
            elif toolkit.get("status") == "missing":
                findings.append({
                    "status": "MISSING",
                    "type": "planned_toolkit_missing",
                    "toolkit_id": toolkit.get("toolkit_id", ""),
                    "file": file,
                    "section": section_name,
                    "placement_before": placement_before,
                    "covers": toolkit.get("covers", []),
                    "message": "Toolkit is planned but not yet present in source.",
                })
    else:
        for section in inventories:
            if section.formal_items and not section.existing_toolkit_boxes:
                findings.append({
                    "status": "MISSING",
                    "type": "section_without_toolkit",
                    "file": section.file,
                    "section": section.section,
                    "first_formal_label": section.formal_items[0].label,
                    "message": "Section contains formal items but no toolkit-like box.",
                })

    report = {
        "chapter": chapter,
        "timestamp": _human_timestamp(),
        "plan_path": str(plan_path) if plan_path else "",
        "summary": {
            "sections_scanned": len(inventories),
            "findings": len(findings),
            "planned_toolkits": len(plan.get("toolkits", [])) if plan else 0,
        },
        "findings": findings,
        "inventory": inventory,
    }

    json_path = _write_json_report(report, chapter, "toolkit-audit")
    md_path = _write_markdown_report(format_toolkit_audit_markdown(report), chapter, "toolkit-audit")
    print(format_toolkit_audit_markdown(report))
    print(f"\nToolkit audit JSON written to: {json_path}")
    print(f"Toolkit audit markdown written to: {md_path}")
    return report


def format_toolkit_audit_markdown(report: dict[str, Any]) -> str:
    lines = [
        "# Toolkit Audit",
        f"- **Chapter:** {report.get('chapter', '?')}",
        f"- **Timestamp:** {report.get('timestamp', '')}",
        f"- **Sections scanned:** {report.get('summary', {}).get('sections_scanned', 0)}",
        f"- **Findings:** {report.get('summary', {}).get('findings', 0)}",
        "",
    ]
    findings = report.get("findings", [])
    if not findings:
        lines += ["_No toolkit findings._", ""]
        return "\n".join(lines)
    for finding in findings:
        lines += [
            f"## {finding.get('status', '')}: {finding.get('type', '')}",
            "",
            finding.get("message", ""),
            "",
        ]
        for key in ("toolkit_id", "file", "section", "placement_before", "first_formal_label"):
            if finding.get(key):
                lines.append(f"- **{key}:** `{finding[key]}`")
        if finding.get("covers"):
            lines.append("- **covers:** " + ", ".join(f"`{label}`" for label in finding["covers"]))
        lines.append("")
    return "\n".join(lines)
