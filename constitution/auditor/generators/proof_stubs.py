"""
generators/proof_stubs.py
Chapter-wide proof stub generation.

This module deliberately delegates proof-file formatting to generators.proof
so the single-proof and batch-stub paths share label conventions.
"""

from __future__ import annotations

from dataclasses import dataclass, asdict
from pathlib import Path
import re
from typing import Any

import yaml

from auditor import config
from auditor.generators.proof import (
    generate_proof_stub_latex,
    label_root,
    proof_label_for,
)


PROVABLE_TYPES = {"thm", "lem", "prop", "cor"}
THEOREM_ENVS = {
    "thm": "theorem",
    "lem": "lemma",
    "prop": "proposition",
    "cor": "corollary",
}


@dataclass
class ProofStubItem:
    label: str
    proof_label: str
    status: str
    theorem_name: str
    output_file: str
    message: str


def generate_chapter_proof_stubs(
    chapter_path: Path,
    *,
    write: bool = False,
    overwrite: bool = False,
    update_chapter_yaml: bool = False,
) -> dict[str, Any]:
    chapter_root = _resolve_chapter_path(chapter_path)
    yaml_path = chapter_root / "chapter.yaml"
    if not yaml_path.exists():
        raise FileNotFoundError(f"chapter.yaml not found: {yaml_path}")

    data = yaml.safe_load(yaml_path.read_text(encoding="utf-8")) or {}
    environments = data.get("environments", [])
    proof_files = data.get("proof_files", [])
    existing_proofs = _scan_existing_proofs(chapter_root)
    proof_entries_by_label = {str(p.get("label")): p for p in proof_files}
    changed_yaml = False
    items: list[ProofStubItem] = []

    for entry in environments:
        artifact_type = str(entry.get("type") or "")
        if artifact_type not in PROVABLE_TYPES:
            continue

        label = str(entry.get("label") or "")
        if not label:
            continue

        proof_label = proof_label_for(label)
        listed_rel = _normalize_rel_path(entry.get("proof_file"))
        listed_path = chapter_root / listed_rel if listed_rel else None

        if proof_label in existing_proofs:
            proof_path = existing_proofs[proof_label]
            rel = _rel_to_chapter(chapter_root, proof_path)
            if not listed_rel and update_chapter_yaml:
                entry["proof_file"] = rel
                changed_yaml = True
            if proof_label not in proof_entries_by_label and update_chapter_yaml:
                proof_files.append({"label": proof_label, "file": rel, "theorem_label": label})
                proof_entries_by_label[proof_label] = proof_files[-1]
                changed_yaml = True
            items.append(
                ProofStubItem(
                    label=label,
                    proof_label=proof_label,
                    status="EXISTS",
                    theorem_name=_title_for_entry(entry),
                    output_file=rel,
                    message="Proof label already exists; no stub generated.",
                )
            )
            continue

        if listed_path and listed_path.exists():
            items.append(
                ProofStubItem(
                    label=label,
                    proof_label=proof_label,
                    status="EXISTS_PATH_LABEL_NOT_FOUND",
                    theorem_name=_title_for_entry(entry),
                    output_file=_rel_to_chapter(chapter_root, listed_path),
                    message="Proof file path exists but expected proof label was not found; left unchanged.",
                )
            )
            continue

        output_rel = listed_rel or _default_proof_rel_path(label, artifact_type)
        output_path = chapter_root / output_rel
        if output_path.exists() and not overwrite:
            items.append(
                ProofStubItem(
                    label=label,
                    proof_label=proof_label,
                    status="SKIP_EXISTS_NO_OVERWRITE",
                    theorem_name=_title_for_entry(entry),
                    output_file=output_rel,
                    message="Output file already exists; use --overwrite to replace it.",
                )
            )
            continue

        statement = _extract_statement(chapter_root, entry)
        theorem_name = _title_for_entry(entry, statement.optional_title)
        content = generate_proof_stub_latex(
            theorem_label=label,
            theorem_name=theorem_name,
            theorem_statement=statement.body,
        )

        if write:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(content, encoding="utf-8")
            status = "WRITTEN"
        else:
            status = "DRY_RUN"

        if update_chapter_yaml:
            entry["proof_file"] = output_rel
            if proof_label not in proof_entries_by_label:
                proof_files.append({"label": proof_label, "file": output_rel, "theorem_label": label})
                proof_entries_by_label[proof_label] = proof_files[-1]
            changed_yaml = True

        items.append(
            ProofStubItem(
                label=label,
                proof_label=proof_label,
                status=status,
                theorem_name=theorem_name,
                output_file=output_rel,
                message="Generated proof stub." if write else "Would generate proof stub.",
            )
        )

    if update_chapter_yaml:
        data["proof_files"] = proof_files
        if write and changed_yaml:
            yaml_path.write_text(yaml.safe_dump(data, sort_keys=False, allow_unicode=False), encoding="utf-8")

    return {
        "chapter_path": str(chapter_root),
        "write": write,
        "overwrite": overwrite,
        "update_chapter_yaml": update_chapter_yaml,
        "items": [asdict(item) for item in items],
    }


@dataclass
class StatementBlock:
    optional_title: str
    body: str


def _resolve_chapter_path(path: Path) -> Path:
    if path.is_absolute():
        return path.resolve()
    return (config.REPO_ROOT / path).resolve()


def _normalize_rel_path(value: Any) -> str:
    if not value:
        return ""
    return str(value).replace("\\", "/")


def _rel_to_chapter(chapter_root: Path, path: Path) -> str:
    return path.resolve().relative_to(chapter_root.resolve()).as_posix()


def _default_proof_rel_path(label: str, artifact_type: str) -> str:
    return f"proofs/notes/prf-{artifact_type}-{label_root(label)}.tex"


def _scan_existing_proofs(chapter_root: Path) -> dict[str, Path]:
    proof_labels: dict[str, Path] = {}
    proof_root = chapter_root / "proofs"
    if not proof_root.exists():
        return proof_labels
    for path in proof_root.rglob("*.tex"):
        text = path.read_text(encoding="utf-8", errors="ignore")
        for label in re.findall(r"\\label\{(prf:[^}]+)\}", text):
            proof_labels[label] = path
    return proof_labels


def _title_for_entry(entry: dict[str, Any], optional_title: str = "") -> str:
    if optional_title.strip():
        return optional_title.strip()
    display = str(entry.get("display_title") or "").strip()
    if display and ":" not in display:
        return display
    return label_root(str(entry.get("label") or "")).replace("-", " ").title()


def _extract_statement(chapter_root: Path, entry: dict[str, Any]) -> StatementBlock:
    rel = _normalize_rel_path(entry.get("file"))
    if not rel:
        raise ValueError(f"No source file listed for {entry.get('label')}")
    source_path = chapter_root / rel
    text = source_path.read_text(encoding="utf-8")
    label = str(entry.get("label"))
    artifact_type = str(entry.get("type"))
    env = THEOREM_ENVS.get(artifact_type)
    if not env:
        raise ValueError(f"Unsupported proof-stub environment type for {label}: {artifact_type}")

    label_match = re.search(r"\\label\{" + re.escape(label) + r"\}", text)
    if not label_match:
        raise ValueError(f"Label not found in source: {label}")

    begin_re = re.compile(r"\\begin\{" + re.escape(env) + r"\}(\[[^\]]*\])?", re.DOTALL)
    begins = [m for m in begin_re.finditer(text[: label_match.start()])]
    if not begins:
        raise ValueError(f"Could not find \\begin{{{env}}} for {label}")
    begin = begins[-1]
    end_re = re.compile(r"\\end\{" + re.escape(env) + r"\}")
    end = end_re.search(text, label_match.end())
    if not end:
        raise ValueError(f"Could not find \\end{{{env}}} for {label}")

    optional_title = (begin.group(1) or "").strip()
    if optional_title.startswith("[") and optional_title.endswith("]"):
        optional_title = optional_title[1:-1].strip()

    body_start = begin.end()
    body = text[body_start:end.start()]
    body = re.sub(r"^\s*\\label\{[^}]+\}\s*", "", body, flags=re.MULTILINE)
    body = re.sub(
        r"\n\s*\\smallskip\s*\n\s*\\noindent\s*\n\s*\\hyperref\[prf:[^\]]+\]\{\\textit\{Go to proof\.\}\}\s*",
        "\n",
        body,
        flags=re.MULTILINE,
    )
    body = re.sub(r"\n{3,}", "\n\n", body).strip()
    return StatementBlock(optional_title=optional_title, body=body)
