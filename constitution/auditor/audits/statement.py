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

    block_start = env_start
    block_end = end_match.end()

    # If the formal environment is wrapped in a display box, audit the wrapper
    # too. Otherwise the model never sees the required box and the following
    # support remarks are hidden behind the wrapper's closing line.
    box_start, box_end = _expand_to_enclosing_display_box(tex, block_start, block_end)
    block_start, block_end = box_start, box_end
    env_block = tex[block_start:block_end]

    # Collect immediately following remark* blocks
    rest = tex[block_end:]
    remarks = []

    # Walk through rest; collect contiguous remark* blocks, allowing blank
    # lines and ordinary comment separators between support remarks.
    pos = 0
    while pos < len(rest):
        pos = _skip_whitespace_and_comments(rest, pos)
        rm = _REMARK_BLOCK.match(rest, pos)
        if rm:
            remarks.append(rm.group(0))
            pos = rm.end()
            continue
        if pos < len(rest):
            break

    combined = env_block
    if remarks:
        combined += "\n\n" + "\n\n".join(remarks)

    return combined


def _skip_whitespace_and_comments(text: str, pos: int) -> int:
    while pos < len(text):
        while pos < len(text) and text[pos].isspace():
            pos += 1
        if pos < len(text) and text[pos] == "%":
            newline = text.find("\n", pos)
            if newline == -1:
                return len(text)
            pos = newline + 1
            continue
        return pos
    return pos


def _expand_to_enclosing_display_box(tex: str, start: int, end: int) -> tuple[int, int]:
    """
    Expands an environment span to an immediately enclosing display box, if
    one exists. This intentionally mirrors the generated patcher span logic.
    """
    candidates = [
        (r"\\begin\{tcolorbox\}(?:\[[^\]]*\])?", r"\\end\{tcolorbox\}"),
        (r"\\begin\{bpnew\}\{[^}]*\}\{[^}]*\}", r"\\end\{bpnew\}"),
    ]

    best: tuple[int, int] | None = None
    for open_re, close_re in candidates:
        opens = list(re.finditer(open_re, tex[:start], re.DOTALL))
        if not opens:
            continue

        closes_before = list(re.finditer(close_re, tex[:start]))
        if closes_before and closes_before[-1].start() > opens[-1].start():
            continue

        open_match = opens[-1]
        close_after = re.search(close_re, tex[end:])
        if not close_after:
            continue

        candidate = (open_match.start(), end + close_after.end())
        if best is None or candidate[0] > best[0]:
            best = candidate

    return best if best is not None else (start, end)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def audit_statement(
    tex_path: Path,
    label: str,
    artifact_type: str,
    chapter: str,
    print_report: bool = True,
    output_dir: Path | None = None,
    filename_prefix: str = "",
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
    registry.pop("toolkit_box", None)
    matrix_row.pop("toolkit_box", None)

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

    report = client.call(system, user, expect_json=True, validate_report=False)

    # Ensure fixed metadata is set correctly before schema validation.
    report.setdefault("audit_type", "statement")
    report["label"] = label
    report["artifact_type"] = artifact_type
    client.validate_audit_report(report)

    report["_report_path"] = str(
        save_audit_report(
            report,
            chapter,
            "audit-statement",
            print_report=print_report,
            output_dir=output_dir,
            filename_prefix=filename_prefix,
        )
    )

    return report


def audit_statement_from_yaml_entry(
    entry: dict,
    chapter_root: Path,
    chapter: str,
    print_report: bool = True,
    output_dir: Path | None = None,
    filename_prefix: str = "",
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
        print_report=print_report,
        output_dir=output_dir,
        filename_prefix=filename_prefix,
    )
