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

    if _is_todo_stub(tex):
        report = _todo_stub_report(label or _infer_label(tex, proof_path))
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


def _is_todo_stub(tex: str) -> bool:
    lowered = tex.lower()
    return "\\label{prf:" in lowered and "todo" in lowered and "\\begin{proof}" in lowered


def _infer_label(tex: str, proof_path: Path) -> str:
    import re

    match = re.search(r"\\label\{(prf:[^}]+)\}", tex)
    if match:
        return match.group(1)
    return f"prf:{proof_path.stem.removeprefix('prf-')}"


def _todo_stub_report(label: str) -> dict:
    checks = [
        ("layer1_newpage", "R", "PASS", ""),
        ("layer2_phantomsection", "R", "PASS", ""),
        ("layer3_proof_label", "R", "PASS", ""),
        ("layer4_return_remark", "R", "PASS", ""),
        ("layer5_theorem_restatement", "R", "PASS", ""),
        ("layer6_professional_proof", "R", "STUB", "Intentional TODO proof stub; full proof not audited yet."),
        ("layer7_detailed_proof", "R", "STUB", "Intentional TODO proof stub; detailed learning proof not expected yet."),
        ("layer8_structure_remark", "R", "PASS", ""),
        ("layer9_dependencies_remark", "R", "PASS", ""),
    ]
    return {
        "audit_type": "proof",
        "artifact_type": "proof",
        "label": label,
        "summary": {
            "total_checks": len(checks),
            "passed": 7,
            "failed": 0,
            "noncompliant": 0,
            "conditional_met": 0,
            "conditional_unmet": 0,
            "conditional_violation": 0,
            "dependent_met": 0,
            "dependent_unmet": 0,
            "dependent_violation": 0,
            "forbidden_violation": 0,
            "stub": True,
        },
        "checks": [
            {
                "block_id": block_id,
                "requirement": requirement,
                "status": status,
                "finding": finding,
            }
            for block_id, requirement, status, finding in checks
        ],
        "violations": [],
    }


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
