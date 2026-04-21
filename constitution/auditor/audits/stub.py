"""
audits/stub.py
Audits a chapter or volume stub for structural compliance.
"""

from pathlib import Path

import yaml

from auditor import client, loader
from auditor.report import save_audit_report


def _collect_stub_inputs(chapter_path: Path) -> dict:
    """
    Collects the directory listing and index.tex contents for a stub audit.
    """
    chapter_root = chapter_path.resolve()

    # Directory listing (relative paths only)
    all_files = sorted(
        str(p.relative_to(chapter_root))
        for p in chapter_root.rglob("*")
        if p.is_file()
    )

    # index.tex contents
    index_path = chapter_root / "index.tex"
    index_tex = (
        index_path.read_text(encoding="utf-8")
        if index_path.exists()
        else "# index.tex NOT FOUND"
    )

    # chapter.yaml contents
    chapter_yaml_path = chapter_root / "chapter.yaml"
    chapter_yaml_text = (
        chapter_yaml_path.read_text(encoding="utf-8")
        if chapter_yaml_path.exists()
        else "# chapter.yaml NOT FOUND"
    )

    return {
        "subject": chapter_root.name,
        "files": all_files,
        "index_tex": index_tex,
        "chapter_yaml": chapter_yaml_text,
    }


def audit_stub_chapter(
    chapter_path: Path,
    chapter_registry: list[dict],
) -> dict:
    """
    Audits a chapter stub.

    Args:
        chapter_path:      Path to the chapter directory.
        chapter_registry:  The volume's chapter registry as a list of dicts
                           [{"subject": ..., "display_title": ...}, ...].

    Returns:
        The audit report dict.
    """
    inputs = _collect_stub_inputs(chapter_path)
    chapter = inputs["subject"]

    base_prompt   = loader.prompt("audit_stub")
    file_schema   = loader.file_schema()

    file_schema_yaml = yaml.dump(
        file_schema,
        default_flow_style=False,
        allow_unicode=True,
    )
    registry_yaml = yaml.dump(
        chapter_registry,
        default_flow_style=False,
        allow_unicode=True,
    )

    system = (
        f"{base_prompt}\n\n"
        f"## File Schema\n\n```yaml\n{file_schema_yaml}\n```\n\n"
        f"## Chapter Registry\n\n```yaml\n{registry_yaml}\n```"
    )

    user = (
        f"## Stub Type\n\nchapter\n\n"
        f"## Chapter Subject\n\n{chapter}\n\n"
        f"## Directory Listing\n\n"
        + "\n".join(f"- {f}" for f in inputs["files"])
        + f"\n\n## index.tex\n\n```latex\n{inputs['index_tex']}\n```\n\n"
        f"## chapter.yaml\n\n```yaml\n{inputs['chapter_yaml']}\n```"
    )

    report = client.call(system, user, expect_json=True)

    report.setdefault("audit_type", "stub_chapter")
    report.setdefault("artifact_type", "stub_chapter")
    report.setdefault("label", chapter)

    save_audit_report(report, chapter, "audit-stub")

    return report
