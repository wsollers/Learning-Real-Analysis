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
    label: str | None = None,
    print_report: bool = True,
    output_dir: Path | None = None,
    filename_prefix: str = "",
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

    system = client.assemble_audit_system_prompt(base_prompt)

    user = (
        f"## Proof File to Audit\n\n"
        f"**File:** `{proof_path.name}`\n\n"
        f"```latex\n{tex}\n```"
    )

    report = client.call(system, user, expect_json=True, validate_report=False)

    # Ensure fixed metadata is set correctly before schema validation.
    report.setdefault("audit_type", "proof")
    report.setdefault("artifact_type", "proof")
    if label:
        report["label"] = label
    client.validate_audit_report(report)

    report["_report_path"] = str(
        save_audit_report(
            report,
            chapter,
            "audit-proof",
            print_report=print_report,
            output_dir=output_dir,
            filename_prefix=filename_prefix,
        )
    )

    return report


def audit_proof_from_yaml_entry(
    entry: dict,
    chapter_root: Path,
    chapter: str,
    print_report: bool = True,
    output_dir: Path | None = None,
    filename_prefix: str = "",
) -> dict:
    """
    Convenience wrapper that accepts a proof_files entry from chapter.yaml.
    """
    proof_path = chapter_root / entry["file"]
    return audit_proof(
        proof_path=proof_path,
        chapter=chapter,
        label=entry.get("label"),
        print_report=print_report,
        output_dir=output_dir,
        filename_prefix=filename_prefix,
    )
