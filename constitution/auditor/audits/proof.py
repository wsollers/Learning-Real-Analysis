"""
audits/proof.py
Audits a single proof .tex file against the 9-layer proof architecture.
"""

from pathlib import Path

from auditor import client, loader
from auditor.report import save_audit_report


def audit_proof(
    proof_path: Path,
    chapter: str,
) -> dict:
    """
    Audits a proof .tex file.

    Args:
        proof_path: Path to the proof .tex file.
        chapter:    Chapter subject name (for report filing).

    Returns:
        The audit report dict.
    """
    tex = proof_path.read_text(encoding="utf-8")

    base_prompt = loader.prompt("audit_proof")

    system = base_prompt  # proof audit prompt is self-contained

    user = (
        f"## Proof File to Audit\n\n"
        f"**File:** `{proof_path.name}`\n\n"
        f"```latex\n{tex}\n```"
    )

    report = client.call(system, user, expect_json=True)

    # Ensure audit_type is set
    report.setdefault("audit_type", "proof")
    report.setdefault("artifact_type", "proof")

    save_audit_report(report, chapter, "audit-proof")

    return report


def audit_proof_from_yaml_entry(
    entry: dict,
    chapter_root: Path,
    chapter: str,
) -> dict:
    """
    Convenience wrapper that accepts a proof_files entry from chapter.yaml.
    """
    proof_path = chapter_root / entry["file"]
    return audit_proof(proof_path=proof_path, chapter=chapter)
