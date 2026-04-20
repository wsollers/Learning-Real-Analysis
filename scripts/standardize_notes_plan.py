#!/usr/bin/env python3
"""
Plan house-style standardization work for analysis notes.

This script is intentionally conservative:
- it does not call an AI service;
- it does not replace source .tex files;
- with --write, it creates sidecar files only:
    <file>.old.tex
    <file>.tmp.tex
    <file>.audit.json
    <file>.requests.json

The script owns deterministic checks.  AI can later consume the generated
requests one narrow task at a time.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


FORMAL_ENVS = (
    "definition",
    "theorem",
    "lemma",
    "proposition",
    "corollary",
    "axiom",
    "assumption",
    "consequence",
)

LABEL_PREFIX = {
    "definition": "def:",
    "theorem": "thm:",
    "lemma": "lem:",
    "proposition": "prop:",
    "corollary": "cor:",
    "axiom": "ax:",
    "assumption": "ass:",
    "consequence": "cons:",
}

REQUIRED_BLOCK_ORDER = [
    "Standard quantified statement",
    "Definition predicate reading",
    "Negated quantified statement",
    "Negation predicate reading",
    "Failure modes",
    "Failure mode decomposition",
    "Contrapositive quantified statement",
    "Interpretation",
    "Dependencies",
]

FORMAL_PATTERN = re.compile(
    r"\\begin\{("
    + "|".join(FORMAL_ENVS)
    + r")\*?\}(?:\[([^\]]*)\])?(.*?)(?=\\end\{\1\*?\})\\end\{\1\*?\}",
    re.S,
)

REMARK_PATTERN = re.compile(
    r"\s*\\begin\{remark\*\}\[([^\]]+)\](.*?)(?=\\end\{remark\*\})\\end\{remark\*\}",
    re.S,
)

LABEL_PATTERN = re.compile(r"\\label\{([^}]+)\}")
HYPERREF_PATTERN = re.compile(r"\\hyperref\[([^\]]+)\]")
TCOLORBOX_PATTERN = re.compile(
    r"\\begin\{tcolorbox\}(.*?)(?=\\end\{tcolorbox\})\\end\{tcolorbox\}",
    re.S,
)

DEFAULT_WORKFLOWS = ["standardization_planning", "definition_generation", "theorem_generation", "audit"]
DEFAULT_TMP_DIR = Path("scripts/tmp/standardizer")

CHECKED_RULE_IDS = {
    "notation.layer_gated_roles",
    "atomicity.global",
    "atomicity.detect_bundled_content",
    "labels.required",
    "labels.prefix",
    "notation.no_predicate_leakage",
    "logical_blocks.standard_quantified_statement",
    "logical_blocks.dependencies_block",
    "prose.interpretation_required",
    "boxes.single_bare_environment",
    "extraction.sidecar_workflow",
}

VIOLATION_RULE_IDS = {
    "Atomicity / Bundled Content": ["atomicity.global", "atomicity.detect_bundled_content"],
    "No Predicate Leakage": ["notation.no_predicate_leakage", "notation.layer_gated_roles"],
    "No Predicate Leakage in Standard quantified statement": ["notation.no_predicate_leakage", "notation.layer_gated_roles"],
    "No Predicate Leakage in Negated quantified statement": ["notation.no_predicate_leakage", "notation.layer_gated_roles"],
    "No Predicate Leakage in Contrapositive quantified statement": ["notation.no_predicate_leakage", "notation.layer_gated_roles"],
    "Required logical blocks": ["logical_blocks.standard_quantified_statement", "prose.interpretation_required"],
    "Required dependencies block": ["logical_blocks.dependencies_block"],
    "Dependencies block - proof label": ["logical_blocks.dependencies_block"],
    "Dependencies block - missing links": ["logical_blocks.dependencies_block"],
    "Label discipline - missing label": ["labels.required"],
    "Label discipline - prefix": ["labels.prefix"],
    "Box-environment separation": ["boxes.single_bare_environment"],
}


@dataclass
class RemarkBlock:
    heading: str
    body: str
    start: int
    end: int
    line: int


@dataclass
class FormalBlock:
    env: str
    title: str
    body: str
    label: str
    start: int
    end: int
    line: int
    remarks: list[RemarkBlock]


def find_repo_root(start: Path) -> Path:
    current = start.resolve()
    if current.is_file():
        current = current.parent
    for path in [current, *current.parents]:
        if (path / "DESIGN.md").exists():
            return path
    raise SystemExit("Could not find repository root containing DESIGN.md")


def parse_yaml_list(value: str) -> list[str]:
    value = value.strip()
    if not value.startswith("[") or not value.endswith("]"):
        return []
    inner = value[1:-1].strip()
    if not inner:
        return []
    return [part.strip().strip("'\"") for part in inner.split(",") if part.strip()]


def folded_yaml_field(block: str, field: str) -> str:
    match = re.search(rf"^\s*{re.escape(field)}:\s*(.*)$", block, re.M)
    if not match:
        return ""
    value = match.group(1).strip()
    if value not in {">", "|"}:
        return value.strip("'\"")
    tail = block[match.end() :].splitlines()
    lines: list[str] = []
    for line in tail:
        if re.match(r"^\s{2}[A-Za-z_][A-Za-z0-9_]*:\s*", line):
            break
        if line.startswith("    "):
            lines.append(line[4:])
        elif not line.strip():
            lines.append("")
        else:
            break
    return "\n".join(lines).strip()


def load_predicate_catalog(root: Path) -> list[dict[str, Any]]:
    path = root / "predicates.yaml"
    if not path.exists():
        return []
    text = path.read_text(encoding="utf-8", errors="replace")
    catalog: list[dict[str, Any]] = []
    for match in re.finditer(r"(?m)^-\s+id:\s*([^\n]+)\n", text):
        start = match.start()
        next_match = re.search(r"(?m)^-\s+id:\s*", text[match.end() :])
        end = match.end() + next_match.start() if next_match else len(text)
        block = text[start:end]
        name = folded_yaml_field(block, "name")
        if not name:
            continue
        catalog.append(
            {
                "id": match.group(1).strip(),
                "name": name,
                "reading_aliases": parse_yaml_list(folded_yaml_field(block, "reading_aliases")),
                "category": folded_yaml_field(block, "category"),
                "arity": folded_yaml_field(block, "arity"),
                "arguments": parse_yaml_list(folded_yaml_field(block, "arguments")),
                "optional_arguments": parse_yaml_list(folded_yaml_field(block, "optional_arguments")),
                "signature": folded_yaml_field(block, "signature"),
                "formal": folded_yaml_field(block, "formal"),
                "reading_template": folded_yaml_field(block, "reading_template"),
                "note": folded_yaml_field(block, "note"),
            }
        )
    return sorted(catalog, key=lambda item: (item.get("category", ""), item["name"]))


def load_predicate_names(root: Path) -> list[str]:
    names: set[str] = set()
    for item in load_predicate_catalog(root):
        names.add(item["name"])
        names.update(item.get("reading_aliases", []))
    return sorted(names, key=len, reverse=True)


def load_rule_registry(root: Path) -> dict[str, Any]:
    path = root / "rules" / "design-rules.json"
    if not path.exists():
        return {"rules": []}
    return json.loads(path.read_text(encoding="utf-8"))


def select_rules(registry: dict[str, Any], workflows: list[str], modules: list[str]) -> list[dict[str, Any]]:
    workflow_set = set(workflows)
    module_set = set(modules)
    selected = []
    for rule in registry.get("rules", []):
        if module_set and rule.get("module") not in module_set:
            continue
        if workflow_set and not workflow_set.intersection(rule.get("applies_to", [])):
            continue
        selected.append(rule)
    return selected


def initial_rule_coverage(selected_rules: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    coverage: dict[str, dict[str, Any]] = {}
    for rule in selected_rules:
        rule_id = rule["id"]
        coverage[rule_id] = {
            "id": rule_id,
            "module": rule.get("module"),
            "status": "checked" if rule_id in CHECKED_RULE_IDS else "not_checked",
            "violations": 0,
            "source": rule.get("source"),
            "summary": rule.get("summary"),
        }
    return coverage


def rule_ids_for_violation(rule_name: str) -> list[str]:
    if rule_name in VIOLATION_RULE_IDS:
        return VIOLATION_RULE_IDS[rule_name]
    for prefix, ids in VIOLATION_RULE_IDS.items():
        if rule_name.startswith(prefix):
            return ids
    return []


def rule_ids_for_missing_heading(heading: str) -> list[str]:
    if "Standard quantified statement" in heading:
        return ["logical_blocks.standard_quantified_statement"]
    if "Interpretation" in heading:
        return ["prose.interpretation_required"]
    if "Dependencies" in heading:
        return ["logical_blocks.dependencies_block"]
    if "Negation predicate reading" in heading:
        return ["logical_blocks.predicate_reading_header", "variables.closed_predicate"]
    if "Failure modes" in heading or "Failure mode decomposition" in heading:
        return ["logical_blocks.failure_mode_decomposition"]
    return ["logical_blocks.required_order"]


def attach_rule_ids(violations: list[dict[str, Any]]) -> None:
    for violation in violations:
        violation["rule_ids"] = rule_ids_for_violation(violation["rule"])


def update_rule_coverage(coverage: dict[str, dict[str, Any]], violations: list[dict[str, Any]]) -> None:
    for violation in violations:
        for rule_id in violation.get("rule_ids", []):
            if rule_id in coverage:
                coverage[rule_id]["violations"] += 1


def summarize_coverage(coverage: dict[str, dict[str, Any]]) -> dict[str, Any]:
    status_counts = Counter(entry["status"] for entry in coverage.values())
    checked_with_violations = sum(
        1 for entry in coverage.values() if entry["status"] == "checked" and entry["violations"]
    )
    return {
        "selected_rules": len(coverage),
        "by_status": dict(status_counts),
        "checked_rules_with_violations": checked_with_violations,
        "rules": list(coverage.values()),
    }


def line_of(text: str, pos: int) -> int:
    return text.count("\n", 0, pos) + 1


def snippet(text: str, limit: int = 240) -> str:
    return re.sub(r"\s+", " ", text.strip())[:limit]


def iter_tex_files(target: Path) -> Iterable[Path]:
    if target.is_file() and target.suffix == ".tex":
        yield target
        return
    if target.is_dir():
        for path in sorted(target.rglob("*.tex")):
            if path.name.endswith((".old.tex", ".tmp.tex")):
                continue
            yield path


def collect_attached_remarks(text: str, end: int) -> tuple[list[RemarkBlock], int]:
    remarks: list[RemarkBlock] = []
    pos = end
    while True:
        between = text[pos:]
        stripped = between.lstrip()
        skipped = len(between) - len(stripped)
        probe = pos + skipped
        match = REMARK_PATTERN.match(text, probe)
        if not match:
            return remarks, pos
        heading = match.group(1).strip()
        body = match.group(2)
        remarks.append(
            RemarkBlock(
                heading=heading,
                body=body,
                start=match.start(),
                end=match.end(),
                line=line_of(text, match.start()),
            )
        )
        pos = match.end()


def parse_formal_blocks(text: str) -> list[FormalBlock]:
    blocks: list[FormalBlock] = []
    tcolorboxes = [(match.start(), match.end()) for match in TCOLORBOX_PATTERN.finditer(text)]
    for match in FORMAL_PATTERN.finditer(text):
        probe_end = match.end()
        containing_boxes = [
            box_end for box_start, box_end in tcolorboxes if box_start <= match.start() and match.end() <= box_end
        ]
        if containing_boxes:
            probe_end = min(containing_boxes)
        label_match = LABEL_PATTERN.search(match.group(3))
        remarks, cluster_end = collect_attached_remarks(text, probe_end)
        blocks.append(
            FormalBlock(
                env=match.group(1),
                title=(match.group(2) or "").strip(),
                body=match.group(3),
                label=label_match.group(1) if label_match else "",
                start=match.start(),
                end=cluster_end,
                line=line_of(text, match.start()),
                remarks=remarks,
            )
        )
    return blocks


def build_formal_label_catalog(files: list[Path], root: Path) -> list[dict[str, Any]]:
    catalog: list[dict[str, Any]] = []
    seen: set[str] = set()
    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        for block in parse_formal_blocks(text):
            if not block.label or block.label in seen:
                continue
            seen.add(block.label)
            catalog.append(
                {
                    "label": block.label,
                    "env": block.env,
                    "title": block.title,
                    "file": str(path.relative_to(root)).replace("\\", "/"),
                    "line": block.line,
                }
            )
    return sorted(catalog, key=lambda item: (item["file"], item["line"], item["label"]))


def predicate_regex(predicate_names: list[str]) -> re.Pattern[str]:
    if not predicate_names:
        return re.compile(r"a^")
    return re.compile(
        r"\\operatorname\{(" + "|".join(map(re.escape, predicate_names)) + r")\}\s*(?:\(|\b)"
    )


def title_suggests_multiple_concepts(title: str) -> bool:
    if not title:
        return False
    allowed_phrases = {
        "If and Only If",
        "Necessary and Sufficient",
    }
    if any(phrase in title for phrase in allowed_phrases):
        return False
    return any(token in title for token in (";", " and ", ",", "/"))


def split_signals(block: FormalBlock) -> list[str]:
    signals: list[str] = []
    item_count = len(re.findall(r"\\item\b", block.body))
    if item_count >= 2:
        signals.append(f"formal body contains {item_count} list items")
    if re.search(r"\\begin\{(?:itemize|enumerate)\}", block.body):
        signals.append("formal body contains itemize/enumerate")
    if title_suggests_multiple_concepts(block.title):
        signals.append("title appears to name multiple concepts")
    if re.search(r"&.*&", block.body):
        signals.append("display may contain multiple aligned definitions")
    if len(re.findall(r":=", block.body)) >= 2:
        signals.append("formal body contains multiple definition assignments")
    if len(re.findall(r"\\textbf\{[^}]+\}", block.body)) >= 2:
        signals.append("formal body contains multiple bold concept heads")
    return signals


def expected_remark_headings(block: FormalBlock, has_predicate: bool, has_negation: bool) -> list[str]:
    headings = ["Standard quantified statement"]
    if has_predicate:
        headings.append("Definition predicate reading")
    if has_negation:
        headings.extend(
            [
                "Negated quantified statement",
                "Negation predicate reading",
                "Failure modes",
                "Failure mode decomposition",
            ]
        )
    if block.env in {"theorem", "lemma", "proposition", "corollary"}:
        headings.append("Contrapositive quantified statement")
    headings.append("Interpretation")
    return headings


def default_prompt_library() -> dict[str, Any]:
    return {
        "split_environment": (
            "You are standardizing Learning Real Analysis notes under DESIGN.md. "
            "Split the source formal environment into atomic formal environments. "
            "Return JSON only with atomic_items. Each atomic item must include env, "
            "title, label, formal_body, and a short rationale. Do not generate remark "
            "blocks in this step."
        ),
        "generate_definition": (
            "Generate one atomic {env} environment in house notation. Use standard "
            "mathematical notation only. Do not use canonical predicate names or "
            "\\operatorname{{...}} in the environment body. Include exactly one label."
        ),
        "generate_standard_quantified_statement": (
            "Generate the Standard quantified statement remark for the given atomic "
            "formal environment. Use standard mathematical notation only. Do not use "
            "canonical predicate names or \\operatorname{{...}}."
        ),
        "generate_definition_predicate_reading": (
            "Generate the Definition predicate reading remark for the given atomic "
            "formal environment. Use only canonical predicates or registered "
            "reading_aliases from the provided predicates.yaml catalog. When helpful, "
            "start with a short helper-predicate dictionary and then give the main "
            "predicate equivalence in an aligned display. If no canonical predicate "
            "applies, return an explicit no_predicate result."
        ),
        "generate_negated_quantified_statement": (
            "Generate the Negated quantified statement remark. Use standard "
            "mathematical notation only. Do not use canonical predicate names or "
            "\\operatorname{{...}}."
        ),
        "generate_negation_predicate_reading": (
            "Generate the Negation predicate reading remark using only canonical "
            "predicates or registered reading_aliases from the provided predicates.yaml "
            "catalog. Prefer readable helper predicates for nested quantifier "
            "structure. If no canonical predicate applies, return an explicit "
            "no_predicate result."
        ),
        "generate_failure_modes": (
            "Generate the Failure modes remark in prose/list form. Explain the "
            "mathematically distinct ways the negation can occur."
        ),
        "generate_failure_mode_decomposition": (
            "Generate the Failure mode decomposition remark. Canonical predicates "
            "are allowed only if this block is explicitly predicate-analysis oriented."
        ),
        "generate_interpretation": (
            "Generate the Interpretation remark in prose. Do not introduce new formal "
            "content and do not use predicate-language notation unless discussing "
            "notation explicitly."
        ),
        "generate_dependencies": (
            "Generate the mandatory Dependencies remark for the given atomic formal "
            "environment. Use explicit \\hyperref[...] labels from the provided "
            "formal-item label catalog, or write exactly 'No local dependencies.' "
            "when the item is foundational in the current extraction scope. Do not "
            "reference proof labels."
        ),
    }


def prompt_library() -> dict[str, Any]:
    prompt_path = Path(__file__).with_name("standardization_prompts.json")
    if prompt_path.exists():
        try:
            return json.loads(prompt_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            return default_prompt_library()
    return default_prompt_library()


def audit_file(
    path: Path,
    root: Path,
    predicate_catalog: list[dict[str, Any]],
    formal_label_catalog: list[dict[str, Any]] | None = None,
    selected_rules: list[dict[str, Any]] | None = None,
) -> dict:
    text = path.read_text(encoding="utf-8", errors="replace")
    predicate_names = sorted(
        {
            name
            for item in predicate_catalog
            for name in [item["name"], *item.get("reading_aliases", [])]
        },
        key=len,
        reverse=True,
    )
    pred_re = predicate_regex(predicate_names)
    label_catalog = formal_label_catalog or []
    formal_blocks = parse_formal_blocks(text)
    violations: list[dict] = []
    requests: list[dict] = []

    def add_violation(block: FormalBlock | None, rule: str, severity: str, message: str, evidence: str):
        violations.append(
            {
                "rule": rule,
                "severity": severity,
                "line": block.line if block else None,
                "env": block.env if block else None,
                "title": block.title if block else None,
                "label": block.label if block else None,
                "message": message,
                "evidence": snippet(evidence),
            }
        )

    for i, block in enumerate(formal_blocks, start=1):
        block_id = f"{path.stem}:env:{i}"
        signals = split_signals(block)
        predicate_hits = sorted(set(pred_re.findall(block.body)))
        remark_headings = [remark.heading for remark in block.remarks]

        if not block.label:
            add_violation(
                block,
                "Label discipline - missing label",
                "ERROR",
                "Formal environment has no label.",
                block.body,
            )
        elif not block.label.startswith(LABEL_PREFIX.get(block.env, "")):
            add_violation(
                block,
                "Label discipline - prefix",
                "WARNING",
                f"Label should start with {LABEL_PREFIX.get(block.env, '')}.",
                block.label,
            )

        if signals:
            add_violation(
                block,
                "Atomicity / Bundled Content",
                "ERROR",
                "Formal environment appears to contain more than one independently extractable item.",
                "; ".join(signals),
            )
            requests.append(
                {
                    "id": f"{block_id}:split",
                    "action": "split_environment",
                    "file": str(path.relative_to(root)).replace("\\", "/"),
                    "line": block.line,
                    "env": block.env,
                    "title": block.title,
                    "label": block.label,
                    "signals": signals,
                    "source": block.body.strip(),
                    "required_output_contract": {
                        "atomic": True,
                        "one_label_per_environment": True,
                        "formal_body_uses_standard_notation_only": True,
                        "remarks_generated_separately": True,
                    },
                    "rule_ids": ["atomicity.global", "atomicity.detect_bundled_content"],
                    "post_split_generation_plan": {
                        "predicate_catalog_available": True,
                        "for_each_atomic_item": [
                            "generate_definition",
                            "generate_standard_quantified_statement",
                            "generate_definition_predicate_reading_if_canonical_predicate_exists",
                            "generate_negated_quantified_statement_when_mathematically_illuminating",
                            "generate_negation_predicate_reading_if_negation_was_generated",
                            "generate_failure_modes_when_applicable",
                            "generate_failure_mode_decomposition_if_failure_modes_were_generated",
                            "generate_contrapositive_quantified_statement_when_mathematically_illuminating",
                            "generate_interpretation",
                            "generate_dependencies",
                        ],
                        "validation_gate_after_each_step": [
                            "formal_body_has_no_predicate_leakage",
                            "standard_negated_and_contrapositive_blocks_have_no_predicate_leakage",
                            "predicate_blocks_use_only_predicates_yaml",
                            "interpretation_is_prose_only",
                            "dependencies_use_existing_formal_labels_or_explicit_no_local_dependencies",
                            "logical_blocks_are_in_design_order",
                        ],
                    },
                    "prompt_key": "split_environment",
                }
            )

        if predicate_hits:
            add_violation(
                block,
                "No Predicate Leakage",
                "ERROR",
                "Canonical predicate names appear in a formal environment body.",
                ", ".join(predicate_hits),
            )
            requests.append(
                {
                    "id": f"{block_id}:rewrite_formal_body",
                    "action": "rewrite_formal_body",
                    "file": str(path.relative_to(root)).replace("\\", "/"),
                    "line": block.line,
                    "env": block.env,
                    "title": block.title,
                    "label": block.label,
                    "predicate_hits": predicate_hits,
                    "source": block.body.strip(),
                    "rule_ids": ["notation.no_predicate_leakage", "notation.layer_gated_roles"],
                    "prompt_key": "generate_definition",
                }
            )

        missing = []
        if not any("Standard quantified statement" in heading for heading in remark_headings[:2]):
            missing.append("Standard quantified statement")
        if not any("Interpretation" in heading for heading in remark_headings):
            missing.append("Interpretation")
        if not any("Dependencies" == heading for heading in remark_headings):
            missing.append("Dependencies")
        has_negation = any("Negated quantified statement" == heading for heading in remark_headings)
        if has_negation:
            if not any("Negation predicate reading" == heading for heading in remark_headings):
                missing.append("Negation predicate reading")
            if not any(heading in {"Failure mode", "Failure modes"} for heading in remark_headings):
                missing.append("Failure modes")
            if not any("Failure mode decomposition" == heading for heading in remark_headings):
                missing.append("Failure mode decomposition")
        if missing:
            add_violation(
                block,
                "Required dependencies block" if missing == ["Dependencies"] else "Required logical blocks",
                "WARNING",
                "Required logical block stack is incomplete or not immediately attached.",
                ", ".join(missing),
            )
            for heading in missing:
                key = "generate_" + heading.lower().replace(" ", "_")
                requests.append(
                    {
                        "id": f"{block_id}:block:{heading}",
                        "action": "generate_missing_block",
                        "file": str(path.relative_to(root)).replace("\\", "/"),
                        "line": block.line,
                        "env": block.env,
                        "title": block.title,
                        "label": block.label,
                        "missing_heading": heading,
                        "source": block.body.strip(),
                        "existing_remark_headings": remark_headings,
                        "formal_label_catalog": label_catalog,
                        "predicate_catalog": predicate_catalog,
                        "rule_ids": rule_ids_for_missing_heading(heading),
                        "prompt_key": key,
                    }
                )

        for remark in block.remarks:
            if remark.heading == "Dependencies":
                refs = HYPERREF_PATTERN.findall(remark.body)
                proof_refs = [ref for ref in refs if ref.startswith("prf:")]
                if proof_refs:
                    add_violation(
                        block,
                        "Dependencies block - proof label",
                        "ERROR",
                        "Dependencies remarks must reference formal mathematical items, not proof labels.",
                        ", ".join(proof_refs),
                    )
                if not refs and "No local dependencies." not in remark.body:
                    add_violation(
                        block,
                        "Dependencies block - missing links",
                        "WARNING",
                        "Dependencies remark should contain explicit hyperref links or the exact text 'No local dependencies.'.",
                        remark.body,
                    )
            remark_predicate_hits = sorted(set(pred_re.findall(remark.body)))
            forbidden_role = any(
                role in remark.heading
                for role in (
                    "Standard quantified statement",
                    "Negated quantified statement",
                    "Contrapositive quantified statement",
                )
            )
            if forbidden_role and remark_predicate_hits:
                add_violation(
                    block,
                    f"No Predicate Leakage in {remark.heading}",
                    "ERROR",
                    "Predicate names appear in a logical block that requires standard notation.",
                    ", ".join(remark_predicate_hits),
                )
                requests.append(
                    {
                        "id": f"{block_id}:rewrite_remark:{remark.line}",
                        "action": "rewrite_logical_block",
                        "file": str(path.relative_to(root)).replace("\\", "/"),
                        "line": remark.line,
                        "env": block.env,
                        "title": block.title,
                        "label": block.label,
                        "remark_heading": remark.heading,
                        "predicate_hits": remark_predicate_hits,
                        "source": remark.body.strip(),
                        "predicate_catalog": predicate_catalog
                        if "predicate reading" in remark.heading.lower()
                        else [],
                        "rule_ids": ["notation.no_predicate_leakage", "notation.layer_gated_roles"],
                        "prompt_key": "generate_" + remark.heading.lower().replace(" ", "_"),
                    }
                )

    box_violations = audit_tcolorboxes(text, path, root)
    violations.extend(box_violations)
    attach_rule_ids(violations)
    if selected_rules is not None:
        selected_rule_ids = {rule["id"] for rule in selected_rules}
        violations = [
            violation
            for violation in violations
            if selected_rule_ids.intersection(violation.get("rule_ids", []))
        ]
        requests = [
            request
            for request in requests
            if selected_rule_ids.intersection(request.get("rule_ids", []))
        ]
    coverage = initial_rule_coverage(selected_rules or [])
    update_rule_coverage(coverage, violations)

    audit = {
        "file": str(path.relative_to(root)).replace("\\", "/"),
        "formal_environment_count": len(formal_blocks),
        "violation_count": len(violations),
        "request_count": len(requests),
        "rule_coverage": summarize_coverage(coverage) if coverage else None,
        "violations": violations,
    }
    return {
        "audit": audit,
        "requests": {
            "file": str(path.relative_to(root)).replace("\\", "/"),
            "prompt_library": prompt_library(),
            "formal_label_catalog": label_catalog,
            "predicate_catalog": predicate_catalog,
            "requests": requests,
        },
        "tmp_text": build_tmp_text(text, formal_blocks, requests),
    }


def audit_tcolorboxes(text: str, path: Path, root: Path) -> list[dict]:
    violations: list[dict] = []
    for match in TCOLORBOX_PATTERN.finditer(text):
        body = match.group(1)
        title_match = re.search(r"title=\{([^}]*)\}", body[:250])
        title = title_match.group(1) if title_match else ""
        if "Toolkit" in title:
            continue
        if is_breadcrumb_or_navigation_box(body, title):
            continue
        begins = re.findall(r"\\begin\{([A-Za-z*]+)\}", body)
        formal_begins = [name.rstrip("*") for name in begins if name.rstrip("*") in FORMAL_ENVS]
        if len(formal_begins) != 1:
            violations.append(
                {
                    "rule": "Box-environment separation",
                    "severity": "ERROR",
                    "line": line_of(text, match.start()),
                    "env": None,
                    "title": title,
                    "label": None,
                    "message": "Non-toolkit tcolorbox should wrap exactly one bare formal environment.",
                    "evidence": snippet(body),
                }
            )
    return violations


def is_breadcrumb_or_navigation_box(body: str, title: str) -> bool:
    """Breadcrumb/navigation boxes are intentionally out of extraction scope."""
    haystack = f"{title}\n{body}"
    if "\\begin{center}" in body and "\\to" in body:
        return True
    lowered = haystack.lower()
    return any(token in lowered for token in ("breadcrumb", "where you are in the journey"))


def build_tmp_text(text: str, blocks: list[FormalBlock], requests: list[dict]) -> str:
    by_line: dict[int, list[dict]] = {}
    for request in requests:
        by_line.setdefault(int(request["line"]), []).append(request)
    output: list[str] = []
    cursor = 0
    for block in blocks:
        output.append(text[cursor:block.end])
        related = by_line.get(block.line, [])
        if related:
            output.append("\n")
            output.append("% LRA-STANDARDIZER: generated work requests for the preceding environment.\n")
            for request in related:
                output.append(f"% LRA-STANDARDIZER REQUEST: {request['id']} action={request['action']}\n")
            output.append("% LRA-STANDARDIZER: see the sidecar .requests.json file before replacing this block.\n")
        cursor = block.end
    output.append(text[cursor:])
    return "".join(output)


def sidecar_dir_for(path: Path, root: Path, tmp_dir: Path) -> Path:
    tmp_root = tmp_dir if tmp_dir.is_absolute() else root / tmp_dir
    return tmp_root / path.relative_to(root).parent


def write_sidecars(path: Path, result: dict, overwrite_old: bool, root: Path, tmp_dir: Path) -> None:
    sidecar_dir = sidecar_dir_for(path, root, tmp_dir)
    sidecar_dir.mkdir(parents=True, exist_ok=True)
    old_path = sidecar_dir / (path.stem + ".old.tex")
    tmp_path = sidecar_dir / (path.stem + ".tmp.tex")
    audit_path = sidecar_dir / (path.stem + ".audit.json")
    requests_path = sidecar_dir / (path.stem + ".requests.json")

    if overwrite_old or not old_path.exists():
        shutil.copyfile(path, old_path)
    tmp_path.write_text(result["tmp_text"], encoding="utf-8")
    audit_path.write_text(json.dumps(result["audit"], indent=2, ensure_ascii=False), encoding="utf-8")
    requests_path.write_text(json.dumps(result["requests"], indent=2, ensure_ascii=False), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create deterministic standardization audits and AI request plans for LRA .tex notes."
    )
    parser.add_argument("target", help="A .tex file, chapter directory, volume directory, or analysis directory.")
    parser.add_argument("--write", action="store_true", help="Write .old.tex, .tmp.tex, .audit.json, and .requests.json sidecars.")
    parser.add_argument("--overwrite-old", action="store_true", help="Overwrite existing .old.tex sidecars.")
    parser.add_argument(
        "--tmp-dir",
        default=str(DEFAULT_TMP_DIR),
        help="Directory for generated sidecars. Defaults to scripts/tmp/standardizer.",
    )
    parser.add_argument("--summary-json", help="Optional path for a run-level summary JSON.")
    parser.add_argument(
        "--workflow",
        action="append",
        default=[],
        help="Workflow tag for rule selection. Defaults to extraction-standardization workflows.",
    )
    parser.add_argument("--module", action="append", default=[], help="Restrict selected registry rules by module.")
    args = parser.parse_args()

    target = Path(args.target)
    if not target.is_absolute():
        target = (Path.cwd() / target).resolve()
    root = find_repo_root(target if target.exists() else Path.cwd())
    tmp_dir = Path(args.tmp_dir)
    predicate_catalog = load_predicate_catalog(root)
    workflows = args.workflow or DEFAULT_WORKFLOWS
    registry = load_rule_registry(root)
    selected_rules = select_rules(registry, workflows, args.module)
    files = list(iter_tex_files(target))
    if not files:
        raise SystemExit(f"No .tex files found under {target}")
    formal_label_catalog = build_formal_label_catalog(files, root)

    summary = {
        "target": str(target),
        "repo_root": str(root),
        "files_seen": len(files),
        "files_with_requests": 0,
        "total_violations": 0,
        "total_requests": 0,
        "workflows": workflows,
        "modules": args.module,
        "formal_label_catalog_count": len(formal_label_catalog),
        "predicate_catalog_count": len(predicate_catalog),
        "rule_coverage": summarize_coverage(initial_rule_coverage(selected_rules)),
        "files": [],
    }

    for path in files:
        result = audit_file(path, root, predicate_catalog, formal_label_catalog, selected_rules)
        audit = result["audit"]
        if audit["request_count"]:
            summary["files_with_requests"] += 1
        summary["total_violations"] += audit["violation_count"]
        summary["total_requests"] += audit["request_count"]
        if audit.get("rule_coverage"):
            update_rule_coverage(
                {entry["id"]: entry for entry in summary["rule_coverage"]["rules"]},
                audit["violations"],
            )
            summary["rule_coverage"] = summarize_coverage(
                {entry["id"]: entry for entry in summary["rule_coverage"]["rules"]}
            )
        summary["files"].append(
            {
                "file": audit["file"],
                "violations": audit["violation_count"],
                "requests": audit["request_count"],
            }
        )
        if args.write:
            write_sidecars(path, result, args.overwrite_old, root, tmp_dir)

    if args.summary_json:
        summary_path = Path(args.summary_json)
        summary_path.write_text(json.dumps(summary, indent=2, ensure_ascii=False), encoding="utf-8")

    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
