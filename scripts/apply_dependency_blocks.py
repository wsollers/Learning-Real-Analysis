#!/usr/bin/env python3
"""
Apply missing Dependencies remarks from standardizer request sidecars.

This script is deliberately narrow:
- it consumes only generate_dependencies requests;
- it inserts only remark* blocks titled Dependencies;
- it never rewrites existing formal environments or logical remarks.

The generated dependency choices are deterministic.  Explicit local rules handle
known function-chapter relationships, and conservative text heuristics fill in
obvious formal references from the extracted label catalog.
"""

from __future__ import annotations

import argparse
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
TCOLORBOX_PATTERN = re.compile(
    r"\\begin\{tcolorbox\}(.*?)(?=\\end\{tcolorbox\})\\end\{tcolorbox\}",
    re.S,
)
LABEL_PATTERN = re.compile(r"\\label\{([^}]+)\}")
HYPERREF_PATTERN = re.compile(r"\\hyperref\[([^\]]+)\]")


DEPENDENCY_OVERRIDES: dict[str, list[str]] = {
    "def:deleted-epsilon-neighbourhood": ["def:epsilon-neighbourhood"],
    "def:cluster-point-R": ["def:deleted-epsilon-neighbourhood"],
    "thm:cluster-point-sequential": ["def:cluster-point-R"],
    "def:adherent-point-R": ["def:epsilon-neighbourhood"],
    "def:isolated-point-R": ["def:adherent-point-R", "def:cluster-point-R"],
    "def:interior-point-R": ["def:epsilon-neighbourhood"],
    "def:boundary-point-R": ["def:epsilon-neighbourhood"],
    "def:interior-R": ["def:interior-point-R"],
    "def:boundary-R": ["def:boundary-point-R"],
    "def:closure-R": ["def:adherent-point-R"],
    "lem:closure-elementary": ["def:closure-R"],
    "def:closed-set-R": ["def:closure-R"],
    "cor:closed-iff-seq-limits": ["def:closed-set-R", "thm:cluster-point-sequential"],
    "cor:interval-all-limit-points": ["def:cluster-point-R"],
    "thm:heine-borel": ["def:closed-set-R", "def:bounded-set-R"],
    "def:true-near": ["def:epsilon-neighbourhood"],
    "def:delta-close-function": ["def:eps-close-function"],
    "def:limit-function": ["def:cluster-point-R"],
    "def:limit-function-sequential": ["def:limit-function", "def:cluster-point-R"],
    "prop:limit-neighbourhood-equiv": [
        "def:limit-function",
        "def:epsilon-neighbourhood",
        "def:deleted-epsilon-neighbourhood",
    ],
    "thm:limit-unique": ["def:limit-function"],
    "prop:sequential-criterion-limits": ["def:limit-function", "def:limit-function-sequential"],
    "cor:limit-three-equiv": [
        "def:limit-function",
        "prop:limit-neighbourhood-equiv",
        "prop:sequential-criterion-limits",
    ],
    "thm:limit-implies-local-bounding": ["def:limit-function"],
    "thm:bounded-limits": ["def:limit-function"],
    "thm:squeeze-function-limits": ["def:limit-function", "thm:bounded-limits"],
    "thm:limits-are-local": ["def:limit-function"],
    "prop:limit-absolute-value": ["def:limit-function"],
    "thm:composition-of-limits": ["def:limit-function"],
    "thm:monotone-limit-theorem": [
        "def:function-monotone",
        "def:function-bounded",
        "def:left-hand-limit",
        "def:right-hand-limit",
    ],
    "prop:cauchy-criterion-limits": ["def:limit-function"],
    "def:right-hand-limit": ["def:limit-function", "def:cluster-point-R"],
    "def:left-hand-limit": ["def:limit-function", "def:cluster-point-R"],
    "thm:sequential-criterion-right-hand-limit": [
        "def:right-hand-limit",
        "def:limit-function-sequential",
    ],
    "cor:sequential-criterion-left-hand-limit": [
        "def:left-hand-limit",
        "def:limit-function-sequential",
    ],
    "thm:two-sided-limit-iff-matching-one-sided-limits": [
        "def:limit-function",
        "def:right-hand-limit",
        "def:left-hand-limit",
    ],
    "thm:limit-sum": ["def:limit-function", "def:pointwise-sum-of-functions"],
    "thm:limit-difference": ["def:limit-function", "def:pointwise-difference-of-functions"],
    "thm:limit-scalar-multiple": [
        "def:limit-function",
        "def:pointwise-scalar-multiple-of-a-function",
    ],
    "thm:limit-product": ["def:limit-function", "def:pointwise-product-of-functions"],
    "thm:limit-quotient": ["def:limit-function", "def:pointwise-quotient-of-functions"],
    "def:function-bounded": ["def:function-bounded-above", "def:function-bounded-below"],
    "def:function-monotone": ["def:function-increasing", "def:function-decreasing"],
}


@dataclass
class Remark:
    heading: str
    start: int
    end: int


@dataclass
class Cluster:
    env: str
    title: str
    label: str
    start: int
    end: int
    line: int
    remarks: list[Remark]

    @property
    def remark_headings(self) -> list[str]:
        return [remark.heading for remark in self.remarks]


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


def collect_attached_remarks(text: str, end: int) -> tuple[list[Remark], int]:
    remarks: list[Remark] = []
    pos = end
    while True:
        skipped = len(text[pos:]) - len(text[pos:].lstrip())
        probe = pos + skipped
        match = REMARK_PATTERN.match(text, probe)
        if not match:
            return remarks, pos
        remarks.append(Remark(match.group(1).strip(), match.start(), match.end()))
        pos = match.end()


def parse_clusters(text: str) -> list[Cluster]:
    clusters: list[Cluster] = []
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
        clusters.append(
            Cluster(
                env=match.group(1),
                title=(match.group(2) or "").strip(),
                label=label_match.group(1) if label_match else "",
                start=match.start(),
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
    tmp_root = tmp_dir if tmp_dir.is_absolute() else root / tmp_dir
    if not tmp_root.exists():
        return
    for path in sorted(tmp_root.rglob("*.requests.json")):
        try:
            doc = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            continue
        source_file = root / doc.get("file", "")
        if is_relative_to(source_file, target):
            yield path


def clean_text(text: str) -> str:
    text = re.sub(r"\\[A-Za-z]+\*?(?:\[[^\]]*\])?(?:\{([^{}]*)\})?", r"\1", text)
    text = re.sub(r"[^A-Za-z0-9: -]+", " ", text)
    return re.sub(r"\s+", " ", text).strip().lower()


def unique(items: Iterable[str]) -> list[str]:
    seen: set[str] = set()
    out: list[str] = []
    for item in items:
        if not item or item in seen:
            continue
        seen.add(item)
        out.append(item)
    return out


def infer_dependencies(request: dict[str, Any], label_catalog: dict[str, dict[str, Any]]) -> list[str]:
    label = request.get("label", "")
    source = request.get("source", "")
    source_clean = clean_text(source)
    candidates: list[str] = []

    if label in DEPENDENCY_OVERRIDES:
        candidates.extend(DEPENDENCY_OVERRIDES[label])
        for ref in HYPERREF_PATTERN.findall(source):
            if ref in label_catalog and ref != label and not ref.startswith("prf:"):
                candidates.append(ref)
        return [dep for dep in unique(candidates) if dep in label_catalog and dep != label]

    for ref in HYPERREF_PATTERN.findall(source):
        if ref in label_catalog and ref != label and not ref.startswith("prf:"):
            candidates.append(ref)

    for item_label, item in label_catalog.items():
        if item_label == label or item_label.startswith("prf:"):
            continue
        title = clean_text(item.get("title", ""))
        if title and len(title) >= 9 and title in source_clean:
            candidates.append(item_label)

    if "\\lim" in source or "lim_" in source_clean:
        if label != "def:limit-function":
            candidates.append("def:limit-function")
    if "cluster point" in source_clean and label != "def:cluster-point-R":
        candidates.append("def:cluster-point-R")
    if "neighbourhood" in source_clean or "neighborhood" in source_clean:
        candidates.append("def:epsilon-neighbourhood")
    if "deleted" in source_clean and "neighbourhood" in source_clean:
        candidates.append("def:deleted-epsilon-neighbourhood")
    if (
        "bounded" in source_clean
        and label != "def:function-bounded"
        and not label.startswith("def:function-bounded-")
        and label != "def:bounded-set-R"
    ):
        candidates.append("def:function-bounded")
    if "monotone" in source_clean and label != "def:function-monotone":
        candidates.append("def:function-monotone")
    if "increasing" in source_clean and label != "def:function-increasing":
        candidates.append("def:function-increasing")
    if "decreasing" in source_clean and label != "def:function-decreasing":
        candidates.append("def:function-decreasing")

    return [dep for dep in unique(candidates) if dep in label_catalog and dep != label]


def dependency_block(deps: list[str], label_catalog: dict[str, dict[str, Any]]) -> str:
    if not deps:
        return "\n\n\\begin{remark*}[Dependencies]\nNo local dependencies.\n\\end{remark*}"
    lines = ["", "", "\\begin{remark*}[Dependencies]\\hfill", "\\begin{itemize}"]
    for dep in deps:
        title = label_catalog[dep].get("title") or dep
        lines.append(f"  \\item \\hyperref[{dep}]{{{title}}}")
    lines.extend(["\\end{itemize}", "\\end{remark*}"])
    return "\n".join(lines)


def apply_file(
    source_path: Path,
    requests: list[dict[str, Any]],
    label_catalog: dict[str, dict[str, Any]],
    replace_existing: bool,
) -> dict[str, Any]:
    text = source_path.read_text(encoding="utf-8", errors="replace")
    clusters = parse_clusters(text)
    by_key = {(cluster.line, cluster.label): cluster for cluster in clusters}
    by_label = {cluster.label: cluster for cluster in clusters if cluster.label}
    edits: list[tuple[int, int, str, dict[str, Any]]] = []
    report_items: list[dict[str, Any]] = []
    for request in requests:
        if request.get("prompt_key") != "generate_dependencies":
            continue
        cluster = by_label.get(request.get("label", "")) or by_key.get((int(request.get("line")), request.get("label", "")))
        if not cluster:
            report_items.append({"request_id": request.get("id"), "status": "skipped", "reason": "cluster not found"})
            continue
        existing = next((remark for remark in cluster.remarks if remark.heading == "Dependencies"), None)
        if existing and not replace_existing:
            report_items.append({"request_id": request.get("id"), "status": "skipped", "reason": "already present"})
            continue
        deps = infer_dependencies(request, label_catalog)
        block = dependency_block(deps, label_catalog)
        if existing:
            edits.append((existing.start, existing.end, block, request))
        else:
            edits.append((cluster.end, cluster.end, block, request))
        report_items.append(
            {
                "request_id": request.get("id"),
                "status": "inserted",
                "label": request.get("label"),
                "dependencies": deps,
            }
        )

    if edits:
        output = text
        for start, end, block, _request in sorted(edits, key=lambda item: item[0], reverse=True):
            output = output[:start] + block + output[end:]
        source_path.write_text(output, encoding="utf-8")

    return {
        "file": str(source_path),
        "inserted": sum(1 for item in report_items if item["status"] == "inserted"),
        "items": report_items,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Insert missing Dependencies remark blocks from standardizer requests.")
    parser.add_argument("target", help="Chapter/file directory whose request sidecars should be applied.")
    parser.add_argument("--tmp-dir", default=str(DEFAULT_TMP_DIR), help="Directory containing standardizer sidecars.")
    parser.add_argument("--report", default="scripts/tmp/dependency-apply-report.json", help="JSON report path.")
    parser.add_argument("--replace-existing", action="store_true", help="Replace existing Dependencies remarks.")
    args = parser.parse_args()

    target = Path(args.target)
    if not target.is_absolute():
        target = (Path.cwd() / target).resolve()
    root = find_repo_root(target if target.exists() else Path.cwd())
    tmp_dir = Path(args.tmp_dir)

    grouped: dict[Path, list[dict[str, Any]]] = {}
    label_catalog: dict[str, dict[str, Any]] = {}
    for requests_path in request_files(target, root, tmp_dir):
        doc = json.loads(requests_path.read_text(encoding="utf-8"))
        for item in doc.get("formal_label_catalog", []):
            if item.get("label"):
                label_catalog[item["label"]] = item
        source_file = root / doc.get("file", "")
        for request in doc.get("requests", []):
            if request.get("prompt_key") == "generate_dependencies":
                grouped.setdefault(source_file, []).append(request)

    file_reports = []
    for source_file, requests in sorted(grouped.items()):
        file_reports.append(apply_file(source_file, requests, label_catalog, args.replace_existing))

    report = {
        "schema_version": 1,
        "target": str(target),
        "files_seen": len(grouped),
        "formal_label_catalog_count": len(label_catalog),
        "inserted_total": sum(item["inserted"] for item in file_reports),
        "files": file_reports,
    }
    report_path = Path(args.report)
    if not report_path.is_absolute():
        report_path = root / report_path
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
