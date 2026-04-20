#!/usr/bin/env python3
"""
Consume standardization request sidecars and assemble approval drafts.

This is the second stage after standardize_notes_plan.py.  It deliberately
does not call an AI service.  Instead it:

1. reads *.requests.json files;
2. creates *.generated.json stubs for narrow generation tasks;
3. validates any filled generated outputs;
4. writes *.assembly.json reports;
5. writes *.assembled.tmp.tex only when replacement LaTeX validates.

Generated sidecars are the handoff point for an AI or a human editor.  The
assembler is intentionally strict so source files are never replaced by
unchecked generated text.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
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
DEFAULT_TMP_DIR = Path("scripts/tmp/standardizer")

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

FORBIDDEN_PREDICATE_HEADINGS = (
    "Standard quantified statement",
    "Negated quantified statement",
    "Contrapositive quantified statement",
)

LOGICAL_BLOCK_ORDER = [
    "Standard quantified statement",
    "Definition predicate reading",
    "Negated quantified statement",
    "Negation predicate reading",
    "Failure modes",
    "Failure mode decomposition",
    "Contrapositive quantified statement",
    "Interpretation",
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
TCOLORBOX_PATTERN = re.compile(
    r"\\begin\{tcolorbox\}(.*?)(?=\\end\{tcolorbox\})\\end\{tcolorbox\}",
    re.S,
)
OPERATORNAME_PATTERN = re.compile(r"\\operatorname\{([A-Za-z][A-Za-z0-9]*)\}")


@dataclass
class RemarkBlock:
    heading: str
    body: str
    start: int
    end: int
    line: int


@dataclass
class FormalCluster:
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


def line_of(text: str, pos: int) -> int:
    return text.count("\n", 0, pos) + 1


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data: dict[str, Any]) -> None:
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")


def load_predicate_names(root: Path) -> set[str]:
    path = root / "predicates.yaml"
    if not path.exists():
        return set()
    text = path.read_text(encoding="utf-8", errors="replace")
    return set(re.findall(r"^\s*name:\s*([A-Za-z][A-Za-z0-9]*)\s*$", text, re.M))


def collect_attached_remarks(text: str, end: int) -> tuple[list[RemarkBlock], int]:
    remarks: list[RemarkBlock] = []
    pos = end
    while True:
        skipped = len(text[pos:]) - len(text[pos:].lstrip())
        probe = pos + skipped
        match = REMARK_PATTERN.match(text, probe)
        if not match:
            return remarks, pos
        remarks.append(
            RemarkBlock(
                heading=match.group(1).strip(),
                body=match.group(2),
                start=match.start(),
                end=match.end(),
                line=line_of(text, match.start()),
            )
        )
        pos = match.end()


def parse_formal_clusters(text: str) -> list[FormalCluster]:
    clusters: list[FormalCluster] = []
    tcolorboxes = [(match.start(), match.end()) for match in TCOLORBOX_PATTERN.finditer(text)]
    for match in FORMAL_PATTERN.finditer(text):
        cluster_start = match.start()
        probe_end = match.end()
        containing_boxes = [
            (box_start, box_end) for box_start, box_end in tcolorboxes if box_start <= match.start() and match.end() <= box_end
        ]
        if containing_boxes:
            box_start, box_end = min(containing_boxes, key=lambda item: item[1] - item[0])
            cluster_start = box_start
            probe_end = box_end
        label_match = LABEL_PATTERN.search(match.group(3))
        remarks, cluster_end = collect_attached_remarks(text, probe_end)
        clusters.append(
            FormalCluster(
                env=match.group(1),
                title=(match.group(2) or "").strip(),
                body=match.group(3),
                label=label_match.group(1) if label_match else "",
                start=cluster_start,
                end=cluster_end,
                line=line_of(text, match.start()),
                remarks=remarks,
            )
        )
    return clusters


def is_relative_to(path: Path, base: Path) -> bool:
    try:
        path.resolve().relative_to(base.resolve())
        return True
    except ValueError:
        return False


def request_files(target: Path, root: Path, tmp_dir: Path) -> Iterable[Path]:
    if target.is_file():
        if target.name.endswith(".requests.json"):
            yield target
        return
    found: set[Path] = set()
    for path in sorted(target.rglob("*.requests.json")):
        found.add(path.resolve())
        yield path
    tmp_root = tmp_dir if tmp_dir.is_absolute() else root / tmp_dir
    if not tmp_root.exists():
        return
    for path in sorted(tmp_root.rglob("*.requests.json")):
        if path.resolve() in found:
            continue
        try:
            doc = load_json(path)
        except json.JSONDecodeError:
            continue
        source_file = root / doc.get("file", "")
        if is_relative_to(source_file, target):
            yield path


def sidecar_base(path: Path) -> str:
    name = path.name
    if name.endswith(".requests.json"):
        return name[: -len(".requests.json")]
    return path.stem


def generated_path_for(requests_path: Path) -> Path:
    return requests_path.with_name(sidecar_base(requests_path) + ".generated.json")


def assembly_report_path_for(requests_path: Path) -> Path:
    return requests_path.with_name(sidecar_base(requests_path) + ".assembly.json")


def assembled_tex_path_for(requests_path: Path) -> Path:
    return requests_path.with_name(sidecar_base(requests_path) + ".assembled.tmp.tex")


def prompt_contract(requests_doc: dict[str, Any], request: dict[str, Any]) -> dict[str, Any]:
    return requests_doc.get("prompt_library", {}).get(request.get("prompt_key"), {})


def make_generated_stub(requests_path: Path, requests_doc: dict[str, Any], root: Path) -> dict[str, Any]:
    entries = []
    for request in requests_doc.get("requests", []):
        contract = prompt_contract(requests_doc, request)
        entries.append(
            {
                "request_id": request["id"],
                "action": request["action"],
                "prompt_key": request.get("prompt_key"),
                "rule_ids": request.get("rule_ids", []),
                "status": "pending",
                "input_sha256": sha256_text(request.get("source", "")),
                "prompt_contract": contract,
                "input": {
                    "file": request.get("file"),
                    "line": request.get("line"),
                    "env": request.get("env"),
                    "title": request.get("title"),
                    "label": request.get("label"),
                    "source": request.get("source"),
                    "signals": request.get("signals", []),
                    "missing_heading": request.get("missing_heading"),
                    "remark_heading": request.get("remark_heading"),
                    "existing_remark_headings": request.get("existing_remark_headings", []),
                },
                "output": None,
                "validation": {
                    "status": "pending",
                    "errors": [],
                    "warnings": [],
                },
            }
        )
    return {
        "schema_version": 1,
        "source_requests": str(requests_path.relative_to(root)).replace("\\", "/"),
        "source_file": requests_doc.get("file"),
        "instructions": [
            "Fill each output with a narrow generated result matching prompt_contract.",
            "Use status='ready_for_validation' when an output should be checked.",
            "For split_environment, output.replacement_latex must contain the complete replacement block stack.",
            "Do not edit the original .tex file; run this consumer again to validate and assemble.",
        ],
        "entries": entries,
    }


def sync_generated_doc(
    generated_doc: dict[str, Any],
    requests_path: Path,
    requests_doc: dict[str, Any],
    root: Path,
) -> dict[str, Any]:
    by_id = {entry.get("request_id"): entry for entry in generated_doc.get("entries", [])}
    stub = make_generated_stub(requests_path, requests_doc, root)
    synced_entries = []
    for stub_entry in stub["entries"]:
        existing = by_id.get(stub_entry["request_id"])
        if existing:
            for key in ("action", "prompt_key", "rule_ids", "input_sha256", "prompt_contract", "input"):
                existing[key] = stub_entry[key]
            existing.setdefault("status", "pending")
            existing.setdefault("output", None)
            existing.setdefault("validation", {"status": "pending", "errors": [], "warnings": []})
            synced_entries.append(existing)
        else:
            synced_entries.append(stub_entry)
    stub["entries"] = synced_entries
    return stub


def latex_from_output(output: Any) -> str:
    if isinstance(output, str):
        return output
    if isinstance(output, dict):
        for key in ("replacement_latex", "latex", "formal_environment", "remark_latex"):
            value = output.get(key)
            if isinstance(value, str):
                return value
    return ""


def replacement_latex_from_output(output: Any) -> str:
    if isinstance(output, dict) and isinstance(output.get("replacement_latex"), str):
        return output["replacement_latex"]
    return ""


def operator_names(text: str) -> set[str]:
    return set(OPERATORNAME_PATTERN.findall(text))


def validate_formal_environment(text: str, predicate_names: set[str]) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    matches = list(FORMAL_PATTERN.finditer(text))
    if not matches:
        errors.append("No formal environment was found in generated LaTeX.")
        return errors, warnings
    for match in matches:
        env = match.group(1)
        body = match.group(3)
        labels = LABEL_PATTERN.findall(body)
        if len(labels) != 1:
            errors.append(f"{env} environment must contain exactly one label; found {len(labels)}.")
        elif not labels[0].startswith(LABEL_PREFIX.get(env, "")):
            errors.append(f"{env} label {labels[0]!r} does not use prefix {LABEL_PREFIX.get(env, '')!r}.")
        canonical_hits = sorted(operator_names(body).intersection(predicate_names))
        if canonical_hits:
            errors.append(
                "Formal environment body leaks canonical predicate names: " + ", ".join(canonical_hits)
            )
        if re.search(r"\\begin\{(?:itemize|enumerate)\}", body):
            errors.append("Formal environment body contains itemize/enumerate; split into atomic environments.")
        if len(re.findall(r"\\item\b", body)) >= 2:
            errors.append("Formal environment body contains multiple list items.")
        if len(re.findall(r":=", body)) >= 2:
            warnings.append("Generated formal body still contains multiple := assignments; verify atomicity.")
    return errors, warnings


def validate_remark_roles(text: str, predicate_names: set[str]) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    for match in REMARK_PATTERN.finditer(text):
        heading = match.group(1).strip()
        body = match.group(2)
        canonical_hits = sorted(operator_names(body).intersection(predicate_names))
        if any(role in heading for role in FORBIDDEN_PREDICATE_HEADINGS) and canonical_hits:
            errors.append(f"{heading} leaks canonical predicate names: {', '.join(canonical_hits)}.")
        if "predicate reading" in heading.lower():
            unknown = sorted(operator_names(body).difference(predicate_names))
            if unknown:
                errors.append(f"{heading} uses noncanonical predicate/operator names: {', '.join(unknown)}.")
        if heading == "Interpretation" and canonical_hits:
            warnings.append("Interpretation contains canonical predicate notation; verify this is explanatory.")
        if heading == "Failure modes" and re.match(r"\s*\\begin\{itemize\}", body):
            warnings.append("Failure modes begins with itemize; DESIGN.md usually expects \\hfill after heading.")
    clusters = parse_formal_clusters(text)
    if clusters:
        for cluster in clusters:
            headings = [remark.heading for remark in cluster.remarks]
            ordered_indices = [LOGICAL_BLOCK_ORDER.index(h) for h in headings if h in LOGICAL_BLOCK_ORDER]
            if ordered_indices != sorted(ordered_indices):
                label = cluster.label or f"line {cluster.line}"
                errors.append(f"Logical remark blocks are not in DESIGN.md order for {label}.")
    else:
        headings = [match.group(1).strip() for match in REMARK_PATTERN.finditer(text)]
        ordered_indices = [LOGICAL_BLOCK_ORDER.index(h) for h in headings if h in LOGICAL_BLOCK_ORDER]
        if ordered_indices != sorted(ordered_indices):
            errors.append("Logical remark blocks are not in DESIGN.md order.")
    return errors, warnings


def validate_entry(entry: dict[str, Any], predicate_names: set[str]) -> dict[str, Any]:
    errors: list[str] = []
    warnings: list[str] = []
    status = entry.get("status", "pending")
    output = entry.get("output")
    if status == "pending" or output in (None, ""):
        return {"status": "pending", "errors": [], "warnings": []}
    if status not in {"ready_for_validation", "validated"}:
        errors.append("status must be 'pending', 'ready_for_validation', or 'validated'.")
    if isinstance(output, dict) and output.get("omit") is True:
        if not output.get("reason"):
            errors.append("omit output requires a reason.")
        return {"status": "invalid" if errors else "validated", "errors": errors, "warnings": warnings}
    if isinstance(output, dict) and output.get("no_predicate") is True:
        if not output.get("reason"):
            errors.append("no_predicate output requires a reason.")
        return {"status": "invalid" if errors else "validated", "errors": errors, "warnings": warnings}
    latex = latex_from_output(output)
    if not latex.strip():
        errors.append("No LaTeX content found in output.")
        return {"status": "invalid", "errors": errors, "warnings": warnings}

    action = entry.get("action")
    if isinstance(output, dict) and isinstance(output.get("replacement_latex"), str):
        replacement = replacement_latex_from_output(output)
        if not replacement.strip():
            errors.append("output.replacement_latex is empty.")
        else:
            formal_errors, formal_warnings = validate_formal_environment(replacement, predicate_names)
            remark_errors, remark_warnings = validate_remark_roles(replacement, predicate_names)
            errors.extend(formal_errors)
            errors.extend(remark_errors)
            warnings.extend(formal_warnings)
            warnings.extend(remark_warnings)
    elif action == "split_environment":
        errors.append("split_environment output requires output.replacement_latex for assembly.")
    elif action in {"rewrite_formal_body"}:
        formal_errors, formal_warnings = validate_formal_environment(latex, predicate_names)
        errors.extend(formal_errors)
        warnings.extend(formal_warnings)
    else:
        remark_errors, remark_warnings = validate_remark_roles(latex, predicate_names)
        errors.extend(remark_errors)
        warnings.extend(remark_warnings)
        if not REMARK_PATTERN.search(latex):
            warnings.append("Generated output does not contain a remark* block; assembly may require wrapping.")

    return {"status": "invalid" if errors else "validated", "errors": errors, "warnings": warnings}


def validate_generated_doc(generated_doc: dict[str, Any], predicate_names: set[str]) -> dict[str, Any]:
    counts = {"pending": 0, "validated": 0, "invalid": 0, "superseded": 0}
    for entry in generated_doc.get("entries", []):
        validation = validate_entry(entry, predicate_names)
        entry["validation"] = validation
        if validation["status"] == "validated":
            entry["status"] = "validated"
    replacement_lines = {
        int(entry.get("input", {}).get("line"))
        for entry in generated_doc.get("entries", [])
        if entry.get("validation", {}).get("status") == "validated"
        and replacement_latex_from_output(entry.get("output"))
        and entry.get("input", {}).get("line") is not None
    }
    replacement_labels = {
        entry.get("input", {}).get("label")
        for entry in generated_doc.get("entries", [])
        if entry.get("validation", {}).get("status") == "validated"
        and replacement_latex_from_output(entry.get("output"))
        and entry.get("input", {}).get("label")
    }
    for entry in generated_doc.get("entries", []):
        line = entry.get("input", {}).get("line")
        label = entry.get("input", {}).get("label")
        validation = entry.get("validation", {})
        if (
            (
                (line is not None and int(line) in replacement_lines)
                or (label and label in replacement_labels)
            )
            and validation.get("status") == "pending"
            and not replacement_latex_from_output(entry.get("output"))
        ):
            entry["status"] = "superseded"
            entry["validation"] = {
                "status": "superseded",
                "errors": [],
                "warnings": ["Request covered by a validated replacement_latex for the same source cluster."],
            }
    for entry in generated_doc.get("entries", []):
        status = entry.get("validation", {}).get("status", "pending")
        counts[status] = counts.get(status, 0) + 1
    return counts


def group_replacements(generated_doc: dict[str, Any]) -> dict[int, dict[str, Any]]:
    replacements: dict[int, dict[str, Any]] = {}
    for entry in generated_doc.get("entries", []):
        line = entry.get("input", {}).get("line")
        output = entry.get("output")
        replacement = replacement_latex_from_output(output)
        if line is None or not replacement:
            continue
        replacements[int(line)] = {
            "request_id": entry.get("request_id"),
            "replacement_latex": replacement.rstrip() + "\n",
            "validated": entry.get("validation", {}).get("status") == "validated",
        }
    return replacements


def assemble_text(source_text: str, replacements_by_line: dict[int, dict[str, Any]]) -> tuple[str, list[str]]:
    blockers: list[str] = []
    clusters = parse_formal_clusters(source_text)
    output: list[str] = []
    cursor = 0
    for cluster in clusters:
        replacement = replacements_by_line.get(cluster.line)
        if not replacement:
            continue
        if not replacement["validated"]:
            blockers.append(f"{replacement['request_id']} has replacement_latex but did not validate.")
            continue
        output.append(source_text[cursor:cluster.start])
        output.append(replacement["replacement_latex"])
        cursor = cluster.end
    output.append(source_text[cursor:])
    return "".join(output), blockers


def process_requests_file(
    requests_path: Path,
    root: Path,
    predicate_names: set[str],
    init_stubs: bool,
    overwrite_stubs: bool,
    assemble: bool,
) -> dict[str, Any]:
    requests_doc = load_json(requests_path)
    generated_path = generated_path_for(requests_path)
    if generated_path.exists() and not overwrite_stubs:
        generated_doc = load_json(generated_path)
        generated_doc = sync_generated_doc(generated_doc, requests_path, requests_doc, root)
    elif init_stubs or overwrite_stubs:
        generated_doc = make_generated_stub(requests_path, requests_doc, root)
    else:
        generated_doc = make_generated_stub(requests_path, requests_doc, root)

    counts = validate_generated_doc(generated_doc, predicate_names)
    write_json(generated_path, generated_doc)

    source_file = root / requests_doc["file"]
    source_text = source_file.read_text(encoding="utf-8", errors="replace")
    replacements = group_replacements(generated_doc)
    blockers: list[str] = []
    assembled_path = assembled_tex_path_for(requests_path)
    wrote_assembled = False
    if assemble:
        request_count = len(requests_doc.get("requests", []))
        if request_count == 0:
            report = {
                "schema_version": 1,
                "source_requests": str(requests_path.relative_to(root)).replace("\\", "/"),
                "source_file": requests_doc["file"],
                "generated_file": str(generated_path.relative_to(root)).replace("\\", "/"),
                "assembled_file": str(assembled_path.relative_to(root)).replace("\\", "/"),
                "request_count": request_count,
                "validation_counts": counts,
                "replacement_count": 0,
                "assembly_requested": assemble,
                "assembled_written": False,
                "blockers": [],
                "notes": ["No requests were present, so no assembled tmp file was needed."],
            }
            write_json(assembly_report_path_for(requests_path), report)
            return report
        pending = [
            entry["request_id"]
            for entry in generated_doc.get("entries", [])
            if entry.get("validation", {}).get("status") not in {"validated", "superseded"}
        ]
        if pending:
            blockers.extend(f"{request_id} is not validated." for request_id in pending)
        if not replacements:
            blockers.append("No validated replacement_latex blocks are available.")
        assembled_text, assembly_blockers = assemble_text(source_text, replacements)
        blockers.extend(assembly_blockers)
        if not blockers:
            assembled_path.write_text(assembled_text, encoding="utf-8")
            wrote_assembled = True

    report = {
        "schema_version": 1,
        "source_requests": str(requests_path.relative_to(root)).replace("\\", "/"),
        "source_file": requests_doc["file"],
        "generated_file": str(generated_path.relative_to(root)).replace("\\", "/"),
        "assembled_file": str(assembled_path.relative_to(root)).replace("\\", "/"),
        "request_count": len(requests_doc.get("requests", [])),
        "validation_counts": counts,
        "replacement_count": len(replacements),
        "assembly_requested": assemble,
        "assembled_written": wrote_assembled,
        "blockers": blockers,
    }
    write_json(assembly_report_path_for(requests_path), report)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create generation stubs, validate filled outputs, and assemble approval tmp files."
    )
    parser.add_argument("target", help="A .requests.json file or directory containing request sidecars.")
    parser.add_argument("--init-stubs", action="store_true", help="Create or sync .generated.json stubs.")
    parser.add_argument("--overwrite-stubs", action="store_true", help="Recreate generated stubs from requests.")
    parser.add_argument("--assemble", action="store_true", help="Write .assembled.tmp.tex when all outputs validate.")
    parser.add_argument(
        "--tmp-dir",
        default=str(DEFAULT_TMP_DIR),
        help="Directory containing standardizer sidecars. Defaults to scripts/tmp/standardizer.",
    )
    parser.add_argument("--summary-json", help="Optional run summary JSON path.")
    args = parser.parse_args()

    target = Path(args.target)
    if not target.is_absolute():
        target = (Path.cwd() / target).resolve()
    root = find_repo_root(target if target.exists() else Path.cwd())
    predicate_names = load_predicate_names(root)
    tmp_dir = Path(args.tmp_dir)
    files = list(request_files(target, root, tmp_dir))
    if not files:
        raise SystemExit(f"No .requests.json files found under {target}")

    reports = [
        process_requests_file(
            path,
            root,
            predicate_names,
            init_stubs=args.init_stubs,
            overwrite_stubs=args.overwrite_stubs,
            assemble=args.assemble,
        )
        for path in files
    ]
    summary = {
        "target": str(target),
        "repo_root": str(root),
        "files_seen": len(files),
        "files_blocked": sum(1 for report in reports if report["blockers"]),
        "assembled_written": sum(1 for report in reports if report["assembled_written"]),
        "total_requests": sum(report["request_count"] for report in reports),
        "validation_counts": {
            status: sum(report["validation_counts"].get(status, 0) for report in reports)
            for status in ("pending", "validated", "invalid", "superseded")
        },
        "files": reports,
    }
    if args.summary_json:
        summary_path = Path(args.summary_json)
        if not summary_path.is_absolute():
            summary_path = root / summary_path
        write_json(summary_path, summary)
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 1 if summary["files_blocked"] and args.assemble else 0


if __name__ == "__main__":
    raise SystemExit(main())
