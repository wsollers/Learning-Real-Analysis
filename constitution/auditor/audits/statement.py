"""
audits/statement.py
Audits a single theorem-like environment and its following remark* blocks.
"""

import re
from pathlib import Path

import yaml

from auditor import client, loader
from auditor.config import ENV_TO_TYPE, THEOREM_LIKE_ENVS
from auditor.report import save_audit_report


# ---------------------------------------------------------------------------
# Environment extraction
# ---------------------------------------------------------------------------

# Matches \begin{envname} up to (and including) the matching \end{envname},
# then captures all immediately following remark* blocks.
_REMARK_BLOCK = re.compile(
    r"\\begin\{remark\*\}.*?\\end\{remark\*\}",
    re.DOTALL,
)


def _extract_environment_and_remarks(
    tex: str,
    label: str,
) -> str | None:
    """
    Extracts the environment block identified by `label` and all remark*
    blocks that immediately follow it (before the next environment or section).

    Returns the extracted LaTeX string, or None if the label is not found.
    """
    # Find the label position
    label_pattern = re.compile(re.escape(f"\\label{{{label}}}"))
    label_match = label_pattern.search(tex)
    if not label_match:
        return None

    # Walk back to find the \begin{...} that contains this label
    prefix = tex[:label_match.start()]
    begin_pattern = re.compile(
        r"\\begin\{(" + "|".join(THEOREM_LIKE_ENVS) + r")\}",
        re.IGNORECASE,
    )
    begins = list(begin_pattern.finditer(prefix))
    if not begins:
        return None

    last_begin = begins[-1]
    env_name = last_begin.group(1).lower()
    end_pattern = re.compile(re.escape(f"\\end{{{env_name}}}"))

    # Find matching \end
    env_start = last_begin.start()
    end_match = end_pattern.search(tex, label_match.start())
    if not end_match:
        return None

    env_block = tex[env_start : end_match.end()]

    # Collect immediately following remark* blocks
    rest = tex[end_match.end():]
    remarks = []

    # Walk through rest line by line; collect contiguous remark* blocks
    pos = 0
    # Skip whitespace and comments
    while pos < len(rest):
        stripped = rest[pos:].lstrip()
        if stripped.startswith("\\begin{remark*}"):
            rm = _REMARK_BLOCK.match(stripped)
            if rm:
                remarks.append(rm.group(0))
                pos += len(rest) - len(stripped) + rm.end()
                continue
        # Stop at first non-remark, non-whitespace content
        if stripped and not stripped.startswith("%"):
            break
        # Advance past whitespace/comments
        next_line = rest.find("\n", pos)
        if next_line == -1:
            break
        pos = next_line + 1

    combined = env_block
    if remarks:
        combined += "\n\n" + "\n\n".join(remarks)

    return combined


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def audit_statement(
    tex_path: Path,
    label: str,
    artifact_type: str,
    chapter: str,
) -> dict:
    """
    Audits a single statement environment identified by label.

    Args:
        tex_path:       Path to the .tex file containing the environment.
        label:          The full LaTeX label (e.g. "def:upper-bound").
        artifact_type:  One of: def, thm, lem, prop, cor, ax.
        chapter:        Chapter subject name (for report filing).

    Returns:
        The audit report dict (conforming to audit-report.json schema).
    """
    tex = tex_path.read_text(encoding="utf-8")

    block = _extract_environment_and_remarks(tex, label)
    if block is None:
        raise ValueError(
            f"Label '{label}' not found in {tex_path}. "
            f"Verify the label exists and the file is correct."
        )

    # Build prompt components
    base_prompt    = loader.prompt("audit_statement")
    registry       = loader.block_registry()
    matrix_row     = loader.matrix_row(artifact_type)

    registry_yaml  = yaml.dump(
        {"blocks": list(registry.values())},
        default_flow_style=False,
        allow_unicode=True,
    )
    matrix_yaml    = yaml.dump(
        matrix_row,
        default_flow_style=False,
        allow_unicode=True,
    )

    system = client.assemble_audit_system_prompt(
        base_prompt,
        block_registry_yaml=registry_yaml,
        artifact_matrix_row=matrix_yaml,
        artifact_type=artifact_type,
    )

    user = (
        f"## LaTeX Block to Audit\n\n"
        f"```latex\n{block}\n```\n\n"
        f"Artifact type: {artifact_type}\n"
        f"Label: {label}"
    )

    report = client.call(system, user, expect_json=True)

    # Ensure label is set correctly in report
    report["label"] = label
    report["artifact_type"] = artifact_type

    save_audit_report(report, chapter, "audit-statement")

    return report


def audit_statement_from_yaml_entry(
    entry: dict,
    chapter_root: Path,
    chapter: str,
) -> dict:
    """
    Convenience wrapper that accepts an environments entry from chapter.yaml.
    """
    tex_path = chapter_root / entry["file"]
    return audit_statement(
        tex_path=tex_path,
        label=entry["label"],
        artifact_type=entry["type"],
        chapter=chapter,
    )
