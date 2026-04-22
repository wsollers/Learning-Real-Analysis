"""
scanner.py
Scans a chapter directory for LaTeX theorem-like environments and proof files.
Used to bootstrap chapter.yaml and to true-up an existing chapter.yaml.

All file writes require explicit caller approval — this module only returns
data structures, never writes to disk.
"""

import re
from dataclasses import dataclass, field, asdict
from pathlib import Path

import yaml

from auditor.config import ENV_TO_TYPE, THEOREM_LIKE_ENVS


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

@dataclass
class EnvironmentEntry:
    label: str
    type: str                   # def | thm | lem | prop | cor | ax
    file: str                   # relative path from chapter root
    display_title: str
    proof_file: str | None      # relative path, or null


@dataclass
class ProofEntry:
    label: str                  # prf:X
    file: str                   # relative path from chapter root
    theorem_label: str          # thm:X / def:X this proof belongs to


@dataclass
class ScanResult:
    subject: str
    chapter_root: Path
    environments: list[EnvironmentEntry] = field(default_factory=list)
    proof_files: list[ProofEntry] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Regex patterns
# ---------------------------------------------------------------------------

# Matches: \begin{definition}[Display Title] or \begin{theorem}[Name]
# Also matches environments with no title argument.
_ENV_OPEN = re.compile(
    r"\\begin\{(" + "|".join(THEOREM_LIKE_ENVS) + r")\}"
    r"(?:\[([^\]]*)\])?",
    re.IGNORECASE,
)

# Matches: \label{def:upper-bound} or \label{thm:cauchy-criterion}
_LABEL = re.compile(r"\\label\{([a-z]+:[a-z0-9\-]+)\}")

# Matches: \label{prf:...} in proof files
_PROOF_LABEL = re.compile(r"\\label\{(prf:[a-z0-9\-]+)\}")

# Matches: \hyperref[thm:...]{Return to Theorem} — extracts theorem_label
_RETURN_LINK = re.compile(r"\\hyperref\[([a-z]+:[a-z0-9\-]+)\]\{Return")


# ---------------------------------------------------------------------------
# Notes scanner
# ---------------------------------------------------------------------------

def _scan_notes_file(
    tex_path: Path,
    chapter_root: Path,
) -> tuple[list[EnvironmentEntry], list[str]]:
    """
    Scans a single notes .tex file for theorem-like environments.
    Returns (entries, warnings).
    """
    entries: list[EnvironmentEntry] = []
    warnings: list[str] = []
    rel_path = str(tex_path.relative_to(chapter_root))

    try:
        text = tex_path.read_text(encoding="utf-8")
    except Exception as e:
        warnings.append(f"Could not read {rel_path}: {e}")
        return entries, warnings

    lines = text.split("\n")

    i = 0
    while i < len(lines):
        line = lines[i]
        env_match = _ENV_OPEN.search(line)

        if env_match:
            env_name    = env_match.group(1).lower()
            env_title   = (env_match.group(2) or "").strip()
            artifact_type = ENV_TO_TYPE.get(env_name, env_name)

            # Search the next 10 lines for the label
            label = None
            search_limit = min(i + 10, len(lines))
            for j in range(i, search_limit):
                lm = _LABEL.search(lines[j])
                if lm:
                    label = lm.group(1)
                    break

            if label is None:
                warnings.append(
                    f"{rel_path} line {i+1}: "
                    f"\\begin{{{env_name}}} has no \\label within 10 lines — skipped."
                )
                i += 1
                continue

            # Verify label prefix matches artifact type
            expected_prefix = {
                "def": "def", "thm": "thm", "lem": "lem",
                "prop": "prop", "cor": "cor", "ax": "ax",
            }.get(artifact_type, artifact_type)

            actual_prefix = label.split(":")[0]
            if actual_prefix != expected_prefix:
                warnings.append(
                    f"{rel_path} line {i+1}: "
                    f"Label prefix mismatch — expected {expected_prefix}:, "
                    f"got {actual_prefix}: in {label}"
                )

            entries.append(EnvironmentEntry(
                label=label,
                type=artifact_type,
                file=rel_path,
                display_title=env_title or label,
                proof_file=None,  # resolved later
            ))

        i += 1

    return entries, warnings


def _scan_proof_file(
    tex_path: Path,
    chapter_root: Path,
) -> tuple[ProofEntry | None, list[str]]:
    """
    Scans a single proof .tex file.
    Returns (ProofEntry or None, warnings).
    """
    warnings: list[str] = []
    rel_path = str(tex_path.relative_to(chapter_root))

    try:
        text = tex_path.read_text(encoding="utf-8")
    except Exception as e:
        warnings.append(f"Could not read {rel_path}: {e}")
        return None, warnings

    # Find proof label
    proof_label_match = _PROOF_LABEL.search(text)
    if not proof_label_match:
        warnings.append(f"{rel_path}: No \\label{{prf:...}} found — skipped.")
        return None, warnings

    proof_label = proof_label_match.group(1)

    # Find return link to identify the theorem this proof belongs to
    return_match = _RETURN_LINK.search(text)
    if not return_match:
        warnings.append(
            f"{rel_path}: No return hyperref link found. "
            f"Cannot determine theorem_label — using prf label root."
        )
        # Infer theorem label from proof label
        root = proof_label.replace("prf:", "")
        theorem_label = f"thm:{root}"
    else:
        theorem_label = return_match.group(1)

    return ProofEntry(
        label=proof_label,
        file=rel_path,
        theorem_label=theorem_label,
    ), warnings


def _resolve_proof_links(
    environments: list[EnvironmentEntry],
    proof_files: list[ProofEntry],
) -> None:
    """
    Mutates environments in place to populate proof_file fields
    by matching theorem labels to proof theorem_labels.
    """
    proof_map = {pf.theorem_label: pf.file for pf in proof_files}
    for env in environments:
        env.proof_file = proof_map.get(env.label)


# ---------------------------------------------------------------------------
# Chapter scanner
# ---------------------------------------------------------------------------

def scan_chapter(chapter_path: Path) -> ScanResult:
    """
    Scans a chapter directory and returns a ScanResult.
    Does not write any files.

    Walks:
    - notes/**/*.tex  for theorem-like environments
    - proofs/notes/*.tex  for proof files
    """
    chapter_root = chapter_path.resolve()
    subject = chapter_root.name
    result = ScanResult(subject=subject, chapter_root=chapter_root)

    # --- Scan notes ---
    notes_dir = chapter_root / "notes"
    if notes_dir.exists():
        for tex_file in sorted(notes_dir.rglob("*.tex")):
            # Skip index files — they only contain \input chains
            if tex_file.name == "index.tex":
                continue
            entries, warnings = _scan_notes_file(tex_file, chapter_root)
            result.environments.extend(entries)
            result.warnings.extend(warnings)
    else:
        result.warnings.append(f"notes/ directory not found in {chapter_root}")

    # --- Scan proof files ---
    proofs_notes_dir = chapter_root / "proofs" / "notes"
    if proofs_notes_dir.exists():
        for tex_file in sorted(proofs_notes_dir.glob("*.tex")):
            if tex_file.name == "index.tex":
                continue
            entry, warnings = _scan_proof_file(tex_file, chapter_root)
            result.warnings.extend(warnings)
            if entry:
                result.proof_files.append(entry)
    else:
        result.warnings.append(
            f"proofs/notes/ directory not found in {chapter_root}"
        )

    # Resolve proof_file links on environment entries
    _resolve_proof_links(result.environments, result.proof_files)

    return result


# ---------------------------------------------------------------------------
# chapter.yaml serialization
# ---------------------------------------------------------------------------

def scan_result_to_yaml(
    result: ScanResult,
    existing_yaml: dict | None = None,
) -> str:
    """
    Converts a ScanResult to a chapter.yaml string.

    If existing_yaml is provided, preserves top-level metadata fields
    (subject/display title/volume/status/dependencies/path) from the existing file,
    only replacing environments and proof_files sections.
    """
    doc: dict = {}

    if existing_yaml:
        # Preserve metadata
        doc["subject"] = existing_yaml.get("subject") or existing_yaml.get("chapter") or result.subject
        for key in ("display_title", "volume", "status", "dependencies", "path"):
            if key in existing_yaml:
                doc[key] = existing_yaml[key]
        doc.setdefault("display_title", "")
        doc.setdefault("volume", "")
        doc.setdefault("status", "in-progress")
        doc.setdefault("dependencies", {"prior": "", "next": ""})
    else:
        doc["subject"] = result.subject
        doc["display_title"] = ""   # caller fills in
        doc["volume"] = ""          # caller fills in
        doc["status"] = "in-progress"
        doc["dependencies"] = {"prior": "", "next": ""}

    doc["environments"] = [asdict(e) for e in result.environments]
    doc["proof_files"]  = [asdict(p) for p in result.proof_files]

    return yaml.dump(doc, default_flow_style=False, allow_unicode=True, sort_keys=False)


# ---------------------------------------------------------------------------
# True-up diff
# ---------------------------------------------------------------------------

@dataclass
class TrueUpReport:
    added: list[str]       # labels in scan but not in yaml
    removed: list[str]     # labels in yaml but not in scan
    changed: list[str]     # labels present in both but with differing fields
    clean: bool


def trueup_diff(
    scan_result: ScanResult,
    existing_yaml: dict,
) -> TrueUpReport:
    """
    Diffs a fresh scan against an existing chapter.yaml.
    Returns a TrueUpReport describing discrepancies.
    Does not write any files.
    """
    existing_envs = {
        e["label"]: e for e in existing_yaml.get("environments", [])
    }
    scanned_envs = {
        e.label: asdict(e) for e in scan_result.environments
    }

    added   = [lbl for lbl in scanned_envs if lbl not in existing_envs]
    removed = [lbl for lbl in existing_envs if lbl not in scanned_envs]

    changed = []
    for lbl in scanned_envs:
        if lbl in existing_envs:
            s = scanned_envs[lbl]
            e = existing_envs[lbl]
            # Compare all fields except proof_file (may be intentionally null in yaml)
            if any(s[k] != e.get(k) for k in ("type", "file", "display_title")):
                changed.append(lbl)

    return TrueUpReport(
        added=added,
        removed=removed,
        changed=changed,
        clean=(not added and not removed and not changed),
    )
