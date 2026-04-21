"""
audits/symbols.py
Audits a chapter for predicate, notation, and relation consistency
against the canonical source files.

Returns markdown (not JSON) — this audit has a different output contract.
"""

from pathlib import Path

from auditor import client, loader
from auditor.report import save_symbol_audit_report


def _collect_chapter_tex(chapter_path: Path) -> str:
    """
    Concatenates all non-index .tex files in the chapter's notes/ and proofs/
    directories into a single string for analysis.
    """
    chapter_root = chapter_path.resolve()
    parts: list[str] = []

    for subdir in ("notes", "proofs/notes", "proofs/exercises"):
        d = chapter_root / subdir
        if not d.exists():
            continue
        for tex_file in sorted(d.rglob("*.tex")):
            if tex_file.name == "index.tex":
                continue
            rel = tex_file.relative_to(chapter_root)
            try:
                content = tex_file.read_text(encoding="utf-8")
                parts.append(f"%% === FILE: {rel} ===\n{content}")
            except Exception as e:
                parts.append(f"%% WARNING: Could not read {rel}: {e}")

    return "\n\n".join(parts)


def audit_symbols(
    chapter_path: Path,
) -> str:
    """
    Audits a chapter's use of predicates, notation, and relations.

    Returns:
        A markdown string containing the audit report.
        The report is also written to disk and printed to terminal.
    """
    chapter = chapter_path.resolve().name

    chapter_tex    = _collect_chapter_tex(chapter_path)
    base_prompt    = loader.prompt("audit_symbols")
    predicates_yaml = loader.canonical_source("predicates")
    notation_yaml   = loader.canonical_source("notation")
    relations_yaml  = loader.canonical_source("relations")

    system = client.assemble_symbol_audit_system_prompt(
        base_prompt,
        predicates_yaml=predicates_yaml,
        notation_yaml=notation_yaml,
        relations_yaml=relations_yaml,
    )

    user = (
        f"## Chapter: {chapter}\n\n"
        f"## Chapter Content\n\n"
        f"```latex\n{chapter_tex}\n```"
    )

    # Symbol audit returns markdown, not JSON
    markdown = client.call(system, user, expect_json=False)

    save_symbol_audit_report(markdown, chapter)

    return markdown
