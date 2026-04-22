"""
safe_auto.py
Conservative deterministic remediation for audit findings.

This patcher intentionally avoids mathematical content generation. It only
applies changes that can be derived from existing source structure:
  - wrap genuinely unboxed theorem-like environments in the standard box
  - normalize proof restatement environments to theorem*
  - fix obvious proof-link/proof-label drift when the manifest identifies the
    intended theorem label
  - add a missing in-statement proof link only when chapter.yaml already names
    the proof file
"""

from __future__ import annotations

import json
import re
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml

from auditor import config


THEOREM_ENVS = ("definition", "theorem", "lemma", "proposition", "corollary", "axiom")
RESTATEMENT_BLOCKS = {
    "layer_theorem_restatement",
    "theorem_restatement",
    "proof_layer_restated_theorem",
    "restatement_theoremstar",
    "proof_layer_restated_theorem",
}

BOX_COLORS = {
    "def": ("defbox", "defborder", "Definition"),
    "thm": ("thmbox", "thmborder", "Theorem"),
    "lem": ("propbox", "propborder", "Lemma"),
    "prop": ("propbox", "propborder", "Proposition"),
    "cor": ("propbox", "propborder", "Corollary"),
    "ax": ("axiombox", "axiomborder", "Axiom"),
}


@dataclass
class PatchEvent:
    label: str
    block_id: str
    action: str
    path: str | None = None
    detail: str = ""


@dataclass
class PatchResult:
    applied: list[PatchEvent] = field(default_factory=list)
    skipped: list[PatchEvent] = field(default_factory=list)
    manual: list[PatchEvent] = field(default_factory=list)


def safe_autopatch(report_dir: Path, apply: bool = False) -> PatchResult:
    report_dir = report_dir.resolve()
    run_state = _load_run_state(report_dir)
    chapter_root = (Path(run_state["repo_root"]) / run_state["chapter_path"]).resolve()
    chapter_yaml_path = chapter_root / "chapter.yaml"
    chapter_yaml = yaml.safe_load(chapter_yaml_path.read_text(encoding="utf-8")) or {}
    environments = chapter_yaml.get("environments", [])
    proof_files = chapter_yaml.get("proof_files", [])

    env_by_label = {entry["label"]: entry for entry in environments}
    proof_by_label = {entry["label"]: entry for entry in proof_files}
    proof_by_theorem = {
        entry.get("theorem_label"): entry
        for entry in proof_files
        if entry.get("theorem_label")
    }

    result = PatchResult()
    texts: dict[Path, str] = {}

    for report_path in _audit_json_paths(report_dir):
        report = json.loads(report_path.read_text(encoding="utf-8"))
        label = report.get("label", "?")
        audit_type = report.get("audit_type")
        for violation in report.get("violations", []):
            block_id = violation.get("block_id", "?")
            finding = violation.get("finding", "")
            if audit_type == "statement":
                _handle_statement_violation(
                    result,
                    texts,
                    chapter_root,
                    chapter_yaml_path,
                    env_by_label,
                    proof_by_theorem,
                    label,
                    block_id,
                    finding,
                )
            elif audit_type == "proof":
                _handle_proof_violation(
                    result,
                    texts,
                    chapter_root,
                    chapter_yaml_path,
                    proof_by_label,
                    label,
                    block_id,
                    finding,
                )

    out_json = report_dir / "safe-autopatch-plan.json"
    out_md = report_dir / "safe-autopatch-plan.md"
    _write_plan(result, out_json, out_md, apply=apply)

    if apply:
        for path, text in texts.items():
            path.write_text(text, encoding="utf-8")

    return result


def _load_run_state(report_dir: Path) -> dict[str, Any]:
    run_json = report_dir / "run.json"
    if not run_json.exists():
        raise FileNotFoundError(f"Expected run.json in report directory: {report_dir}")
    return json.loads(run_json.read_text(encoding="utf-8"))


def _audit_json_paths(report_dir: Path) -> list[Path]:
    return sorted(report_dir.glob("statement/*.json")) + sorted(report_dir.glob("proof/*.json"))


def _get_text(texts: dict[Path, str], path: Path) -> str:
    if path not in texts:
        texts[path] = path.read_text(encoding="utf-8")
    return texts[path]


def _set_text(texts: dict[Path, str], path: Path, text: str) -> None:
    texts[path] = text


def _entry_path(chapter_root: Path, entry: dict[str, Any]) -> Path:
    return chapter_root / entry["file"]


def _label_root(label: str) -> str:
    return label.split(":", 1)[1] if ":" in label else label


def _expected_proof_label(statement_label: str) -> str:
    return f"prf:{_label_root(statement_label)}"


def _find_env_block(tex: str, label: str) -> tuple[int, int, str, str] | None:
    label_match = re.search(r"\\label\{" + re.escape(label) + r"\}", tex)
    if not label_match:
        return None
    begin_re = re.compile(
        r"\\begin\{(" + "|".join(THEOREM_ENVS) + r")\}(\[[^\]]*\])?",
        re.DOTALL,
    )
    begins = [m for m in begin_re.finditer(tex[: label_match.start()])]
    if not begins:
        return None
    begin = begins[-1]
    env = begin.group(1)
    end_re = re.compile(r"\\end\{" + re.escape(env) + r"\}")
    end = end_re.search(tex, label_match.end())
    if not end:
        return None
    return begin.start(), end.end(), env, begin.group(2) or ""


def _is_inside_tcolorbox(tex: str, start: int, end: int) -> bool:
    begin_positions = [m.start() for m in re.finditer(r"\\begin\{tcolorbox\}", tex)]
    end_positions = [m.end() for m in re.finditer(r"\\end\{tcolorbox\}", tex)]
    last_begin = max([p for p in begin_positions if p < start], default=-1)
    last_end = max([p for p in end_positions if p < start], default=-1)
    if last_begin <= last_end:
        return False
    closing = min([p for p in end_positions if p > end], default=-1)
    return closing != -1


def _extract_title(optional_title: str, entry: dict[str, Any]) -> str:
    if optional_title:
        return optional_title.strip()[1:-1].strip()
    display = str(entry.get("display_title") or "").strip()
    if display and ":" not in display:
        return display
    return entry.get("label", "Untitled")


def _box_wrapper(artifact_type: str, title: str) -> tuple[str, str]:
    colback, colframe, noun = BOX_COLORS.get(artifact_type, BOX_COLORS["prop"])
    title_text = f"{noun} ({title})"
    header = (
        f"\\begin{{tcolorbox}}[colback={colback}, colframe={colframe}, arc=2pt,\n"
        f"  left=6pt, right=6pt, top=4pt, bottom=4pt,\n"
        "  title={\\small\\textbf{" + title_text + "}},\n"
        f"  fonttitle=\\small\\bfseries]\n"
    )
    return header, "\\end{tcolorbox}\n"


def _handle_statement_violation(
    result: PatchResult,
    texts: dict[Path, str],
    chapter_root: Path,
    chapter_yaml_path: Path,
    env_by_label: dict[str, dict[str, Any]],
    proof_by_theorem: dict[str, dict[str, Any]],
    label: str,
    block_id: str,
    finding: str,
) -> None:
    entry = env_by_label.get(label)
    if not entry:
        result.manual.append(PatchEvent(label, block_id, "manual", detail="No chapter.yaml environment entry found."))
        return

    path = _entry_path(chapter_root, entry)

    if block_id == "box":
        tex = _get_text(texts, path)
        block = _find_env_block(tex, label)
        if not block:
            result.manual.append(PatchEvent(label, block_id, "manual", str(path), "Could not locate environment block."))
            return
        start, end, _env, optional_title = block
        if _is_inside_tcolorbox(tex, start, end):
            result.skipped.append(PatchEvent(label, block_id, "already_boxed", str(path), "Label is already enclosed by a tcolorbox."))
            return
        title = _extract_title(optional_title, entry)
        before, after = _box_wrapper(entry.get("type", "prop"), title)
        tex = tex[:start] + before + tex[start:end].rstrip() + "\n" + after + tex[end:]
        _set_text(texts, path, tex)
        result.applied.append(PatchEvent(label, block_id, "wrap_environment_in_tcolorbox", str(path), title))
        return

    if block_id == "proof_link":
        proof_entry = proof_by_theorem.get(label)
        if not proof_entry:
            result.manual.append(PatchEvent(label, block_id, "manual", str(path), "No proof file is listed in chapter.yaml."))
            return
        tex = _get_text(texts, path)
        expected = _expected_proof_label(label)
        block = _find_env_block(tex, label)
        if not block:
            result.manual.append(PatchEvent(label, block_id, "manual", str(path), "Could not locate environment block."))
            return
        start, end, _env, _optional_title = block
        env_text = tex[start:end]
        if re.search(r"\\hyperref\[" + re.escape(expected) + r"\]", env_text):
            result.skipped.append(PatchEvent(label, block_id, "proof_link_present", str(path), expected))
            return
        old_link = re.search(r"\\hyperref\[(prf:[^\]]+)\]\{\\textit\{Go to proof\.\}\}", env_text)
        if old_link:
            old = old_link.group(1)
            env_text = env_text.replace(old, expected, 1)
            tex = tex[:start] + env_text + tex[end:]
            _set_text(texts, path, tex)
            _sync_proof_label(result, texts, chapter_root, chapter_yaml_path, proof_entry, old, expected)
            result.applied.append(PatchEvent(label, block_id, "fix_proof_hyperref_target", str(path), f"{old} -> {expected}"))
            return
        insertion = (
            "\n\\smallskip\n\\noindent\n"
            f"\\hyperref[{expected}]{{\\textit{{Go to proof.}}}}\n"
        )
        end_pattern = r"\n\\end\{" + re.escape(_env) + r"\}"
        env_text = re.sub(
            end_pattern,
            lambda _m: insertion + f"\\end{{{_env}}}",
            env_text,
            count=1,
        )
        tex = tex[:start] + env_text + tex[end:]
        _set_text(texts, path, tex)
        result.applied.append(PatchEvent(label, block_id, "insert_missing_proof_link", str(path), expected))
        return

    result.manual.append(PatchEvent(label, block_id, "requires_content_or_judgment", str(path), finding))


def _sync_proof_label(
    result: PatchResult,
    texts: dict[Path, str],
    chapter_root: Path,
    chapter_yaml_path: Path,
    proof_entry: dict[str, Any],
    old_label: str,
    expected_label: str,
) -> None:
    proof_path = _entry_path(chapter_root, proof_entry)
    tex = _get_text(texts, proof_path)
    if f"\\label{{{old_label}}}" in tex:
        tex = tex.replace(f"\\label{{{old_label}}}", f"\\label{{{expected_label}}}", 1)
        _set_text(texts, proof_path, tex)
        result.applied.append(PatchEvent(expected_label, "layer3_proof_label", "sync_proof_file_label", str(proof_path), f"{old_label} -> {expected_label}"))

    yaml_text = _get_text(texts, chapter_yaml_path)
    old_line = f"- label: {old_label}"
    new_line = f"- label: {expected_label}"
    if old_line in yaml_text:
        yaml_text = yaml_text.replace(old_line, new_line, 1)
        _set_text(texts, chapter_yaml_path, yaml_text)
        result.applied.append(PatchEvent(expected_label, "chapter_yaml", "sync_manifest_proof_label", str(chapter_yaml_path), f"{old_label} -> {expected_label}"))


def _handle_proof_violation(
    result: PatchResult,
    texts: dict[Path, str],
    chapter_root: Path,
    chapter_yaml_path: Path,
    proof_by_label: dict[str, dict[str, Any]],
    label: str,
    block_id: str,
    finding: str,
) -> None:
    entry = proof_by_label.get(label)
    if not entry:
        result.manual.append(PatchEvent(label, block_id, "manual", detail="No chapter.yaml proof entry found."))
        return
    path = _entry_path(chapter_root, entry)

    if block_id in RESTATEMENT_BLOCKS:
        tex = _get_text(texts, path)
        first_proof = tex.find(r"\begin{proof}")
        search_area = tex if first_proof == -1 else tex[:first_proof]
        m = re.search(r"\\begin\{(" + "|".join(THEOREM_ENVS) + r")\*\}(\[[^\]]*\])?", search_area)
        if not m:
            result.manual.append(PatchEvent(label, block_id, "manual", str(path), "Could not find proof restatement environment."))
            return
        env = m.group(1)
        if env == "theorem":
            result.skipped.append(PatchEvent(label, block_id, "already_theorem_star", str(path), "Restatement already uses theorem*."))
            return
        tex = tex[:m.start()] + tex[m.start():m.end()].replace(f"{{{env}*}}", "{theorem*}", 1) + tex[m.end():]
        end_pat = re.compile(r"\\end\{" + re.escape(env) + r"\*\}")
        end_match = end_pat.search(tex, m.end())
        if not end_match:
            result.manual.append(PatchEvent(label, block_id, "manual", str(path), "Could not find matching restatement end."))
            return
        tex = tex[:end_match.start()] + r"\end{theorem*}" + tex[end_match.end():]
        _set_text(texts, path, tex)
        result.applied.append(PatchEvent(label, block_id, "normalize_restatement_to_theorem_star", str(path), f"{env}* -> theorem*"))
        return

    if block_id == "layer3_proof_label":
        theorem_label = entry.get("theorem_label")
        if not theorem_label:
            result.manual.append(PatchEvent(label, block_id, "manual", str(path), "No theorem_label in chapter.yaml."))
            return
        expected = _expected_proof_label(theorem_label)
        if expected == label:
            result.skipped.append(PatchEvent(label, block_id, "label_already_expected", str(path), expected))
            return
        _sync_proof_label(result, texts, chapter_root, chapter_yaml_path, entry, label, expected)
        return

    result.manual.append(PatchEvent(label, block_id, "requires_content_or_judgment", str(path), finding))


def _write_plan(result: PatchResult, out_json: Path, out_md: Path, apply: bool) -> None:
    payload = {
        "applied": [event.__dict__ for event in result.applied],
        "skipped": [event.__dict__ for event in result.skipped],
        "manual": [event.__dict__ for event in result.manual],
    }
    out_json.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    lines = [
        "# Safe Autopatch Plan",
        "",
        f"- **Mode:** {'APPLY' if apply else 'DRY RUN'}",
        f"- **Applied:** {len(result.applied)}",
        f"- **Skipped:** {len(result.skipped)}",
        f"- **Manual / not safe:** {len(result.manual)}",
        "",
    ]
    for title, events in [
        ("Applied", result.applied),
        ("Skipped", result.skipped),
        ("Manual / Not Safe", result.manual),
    ]:
        lines += [f"## {title}", ""]
        if not events:
            lines += ["_None._", ""]
            continue
        counts = Counter(event.action for event in events)
        lines += ["### Counts", ""]
        for action, count in counts.most_common():
            lines.append(f"- `{action}`: {count}")
        lines.append("")
        lines += ["### Items", ""]
        for event in events:
            where = f" ({event.path})" if event.path else ""
            detail = f" — {event.detail}" if event.detail else ""
            lines.append(f"- `{event.label}` `{event.block_id}`: {event.action}{where}{detail}")
        lines.append("")
    out_md.write_text("\n".join(lines), encoding="utf-8")
