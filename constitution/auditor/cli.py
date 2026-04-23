"""
cli.py
Command-line interface for the auditor.
All commands are dispatched from here.
"""

import argparse
import sys
import yaml
from pathlib import Path

from auditor import client
from auditor import config
from auditor.config import AI_PROVIDER, validate_config
from auditor.ai_provider import VALID_AI_PROVIDERS, normalize_provider


# ---------------------------------------------------------------------------
# Command implementations
# ---------------------------------------------------------------------------

def cmd_audit_statement(args: argparse.Namespace) -> None:
    from auditor.audits.statement import audit_statement
    audit_statement(
        tex_path=Path(args.file),
        label=args.label,
        artifact_type=args.type,
        chapter=args.chapter or Path(args.file).parent.parent.name,
    )


def cmd_audit_proof(args: argparse.Namespace) -> None:
    from auditor.audits.proof import audit_proof
    audit_proof(
        proof_path=Path(args.file),
        chapter=args.chapter or Path(args.file).parent.parent.parent.name,
    )


def cmd_audit_stub(args: argparse.Namespace) -> None:
    from auditor.audits.stub import audit_stub_chapter

    chapter_path = Path(args.path)
    registry = _load_chapter_registry(chapter_path)

    audit_stub_chapter(
        chapter_path=chapter_path,
        chapter_registry=registry,
    )


def cmd_audit_symbols(args: argparse.Namespace) -> None:
    from auditor.audits.symbols import audit_symbols
    audit_symbols(chapter_path=Path(args.path))


def cmd_audit_chapter(args: argparse.Namespace) -> None:
    from auditor.audits.chapter import audit_chapter
    audit_chapter(
        chapter_path=Path(args.path),
        resume=Path(args.resume) if args.resume else None,
    )


def cmd_audit_toolkits(args: argparse.Namespace) -> None:
    from auditor.audits.toolkits import audit_toolkits
    audit_toolkits(
        chapter_path=Path(args.path),
        plan_path=Path(args.plan) if args.plan else None,
    )


def cmd_audit_box_colors(args: argparse.Namespace) -> None:
    from auditor.audits.box_colors import audit_box_colors
    audit_box_colors(path=Path(args.path))


def cmd_plan_toolkits(args: argparse.Namespace) -> None:
    from auditor.audits.toolkits import plan_toolkits
    plan_toolkits(chapter_path=Path(args.path))


def cmd_plan_proofs_to_do(args: argparse.Namespace) -> None:
    from auditor.plans.proofs_to_do import (
        DEFAULT_TYPES,
        find_proofs_to_do,
        write_proofs_to_do_markdown,
    )

    repo_root = config.REPO_ROOT
    types = set(args.types or DEFAULT_TYPES)
    todos = find_proofs_to_do(
        repo_root,
        types=types,
        include_existing_todo=not args.ignore_existing_todo,
    )
    output_path = Path(args.out) if args.out else repo_root / "proofs-to-do.md"
    written = write_proofs_to_do_markdown(todos, repo_root, output_path)
    print(f"Proofs to do: {len(todos)}")
    print(f"Written: {written}")


def cmd_patch_safe(args: argparse.Namespace) -> None:
    from auditor.patchers.safe_auto import safe_autopatch
    result = safe_autopatch(
        report_dir=Path(args.report_dir),
        apply=args.apply,
    )
    mode = "applied" if args.apply else "planned"
    print(
        f"Safe autopatch {mode}: "
        f"{len(result.applied)} applied, "
        f"{len(result.skipped)} skipped, "
        f"{len(result.manual)} manual/not-safe."
    )
    print(f"Plan written under: {Path(args.report_dir).resolve()}")


def cmd_patch_generated(args: argparse.Namespace) -> None:
    from auditor.patchers.generated import (
        format_generated_patch_result,
        patch_generated,
    )

    result = patch_generated(
        chapter_path=Path(args.chapter_path),
        label=args.label,
        generated_path=Path(args.generated),
        apply=args.apply,
        expected_type=args.type,
        out_dir=Path(args.out_dir) if args.out_dir else None,
    )
    print(format_generated_patch_result(result))
    if result.result == "FAIL":
        sys.exit(2)


def cmd_patch_generated_batch(args: argparse.Namespace) -> None:
    from auditor.patchers.generated_batch import (
        format_batch_result,
        run_generated_batch,
    )

    result = run_generated_batch(
        plan_path=Path(args.plan),
        apply=args.apply,
        generate_missing=args.generate_missing,
        out_dir=Path(args.out_dir) if args.out_dir else None,
    )
    print(format_batch_result(result))
    failed_statuses = {
        "ERROR",
        "GENERATION_ERROR",
        "MISSING_GENERATED",
        "VALIDATION_FAIL",
        "PATCH_FAIL",
    }
    if any(item.status in failed_statuses for item in result.items):
        sys.exit(2)


def cmd_patch_comment_blocks(args: argparse.Namespace) -> None:
    from auditor.patchers.comment_blocks import add_theorem_comment_blocks

    result = add_theorem_comment_blocks(
        path=Path(args.path),
        repo_root=config.REPO_ROOT,
        apply=args.apply,
        include_exercises=args.include_exercises,
    )
    mode = "applied" if args.apply else "planned"
    print(
        f"Comment blocks {mode}: "
        f"{result.blocks_added} blocks across {result.files_changed} files "
        f"({result.files_seen} files scanned)."
    )
    for changed_file in result.changed_files:
        print(f"  {changed_file}")
    if not args.apply and result.blocks_added:
        print("Run again with --apply to write these headers.")


def cmd_index_labels(args: argparse.Namespace) -> None:
    from auditor.indexes.labels import build_label_index, write_label_index
    index = build_label_index(Path(args.path))
    json_path, md_path = write_label_index(
        index,
        output_dir=Path(args.out_dir) if args.out_dir else None,
    )
    print(f"Formal math labels indexed: {index['count']}")
    print(f"Issues: {len(index['issues'])}")
    if index["issues"]:
        counts = {}
        for issue in index["issues"]:
            counts[issue["type"]] = counts.get(issue["type"], 0) + 1
        for issue_type, count in sorted(counts.items()):
            print(f"  {issue_type}: {count}")
    print(f"JSON index written to: {json_path}")
    print(f"Markdown index written to: {md_path}")


def cmd_scan_chapter(args: argparse.Namespace) -> None:
    from auditor.scanner import scan_chapter, scan_result_to_yaml

    chapter_path = Path(args.path)
    result = scan_chapter(chapter_path)
    yaml_path = chapter_path / "chapter.yaml"
    existing_yaml = None
    if yaml_path.exists():
        with open(yaml_path, encoding="utf-8") as f:
            existing_yaml = yaml.safe_load(f) or {}

    if result.warnings:
        print("\nScanner warnings:")
        for w in result.warnings:
            print(f"  WARNING: {w}")

    yaml_str = scan_result_to_yaml(result, existing_yaml=existing_yaml)

    print(f"\nScanned {len(result.environments)} environment(s) "
          f"and {len(result.proof_files)} proof file(s).\n")
    print("Proposed chapter.yaml (environments + proof_files sections):\n")
    print(yaml_str)

    if yaml_path.exists():
        print(
            f"\nchapter.yaml already exists at {yaml_path}.\n"
            f"To update it, run:\n"
            f"  python -m auditor trueup chapter {args.path}\n"
            f"Or pass --write to overwrite (destructive)."
        )
        if getattr(args, "write", False):
            yaml_path.write_text(yaml_str, encoding="utf-8")
            print(f"Written to {yaml_path}")
    else:
        if getattr(args, "write", False):
            yaml_path.write_text(yaml_str, encoding="utf-8")
            print(f"Written to {yaml_path}")
        else:
            print(
                f"\nTo write this to disk:\n"
                f"  python -m auditor scan chapter {args.path} --write"
            )


def cmd_trueup_chapter(args: argparse.Namespace) -> None:
    from auditor.scanner import scan_chapter, trueup_diff, scan_result_to_yaml
    from auditor.report import save_trueup_report

    chapter_path = Path(args.path)
    yaml_path    = chapter_path / "chapter.yaml"

    if not yaml_path.exists():
        print(f"No chapter.yaml at {yaml_path}. Run scan first.")
        sys.exit(1)

    with open(yaml_path, encoding="utf-8") as f:
        existing = yaml.safe_load(f)

    result  = scan_chapter(chapter_path)
    trueup  = trueup_diff(result, existing)
    chapter = chapter_path.resolve().name

    save_trueup_report(trueup, chapter, result.warnings)

    if not trueup.clean:
        print(
            "\nTo apply changes, run:\n"
            f"  python -m auditor scan chapter {args.path} --write"
        )


def cmd_generate_statement(args: argparse.Namespace) -> None:
    from auditor.generators.statement import generate_statement
    from auditor.validators.generated_block import (
        format_generated_validation_report,
        validate_generated_block,
    )
    registry = _load_chapter_registry_from_volume(args.volume) if args.volume else None

    latex = generate_statement(
        artifact_type=args.type,
        content_description=args.subject,
        chapter_subject=args.chapter,
        chapter_registry=registry,
        label=args.label,
    )
    _print_and_optionally_write(latex, args)
    if args.validate:
        report = validate_generated_block(
            latex,
            artifact_type=args.type,
            expected_label=args.label,
        )
        print()
        print(format_generated_validation_report(report))
        if report["result"] != "PASS":
            sys.exit(2)


def cmd_validate_generated(args: argparse.Namespace) -> None:
    from auditor.report import write_report
    from auditor.validators.generated_block import (
        format_generated_validation_report,
        validate_generated_file,
    )

    report = validate_generated_file(
        Path(args.file),
        artifact_type=args.type,
        expected_label=args.label,
    )
    markdown = format_generated_validation_report(report)
    print(markdown)

    if args.out:
        out_path = Path(args.out)
        write_report(markdown, out_path)
        print(f"\nValidation report written to: {out_path}")

    if report["result"] != "PASS":
        sys.exit(2)


def cmd_validate_ontology(args: argparse.Namespace) -> None:
    from auditor.report import write_report
    from auditor.validators.ontology import (
        format_ontology_validation_markdown,
        validate_ontology,
    )

    ontology_dir = Path(args.path) if args.path else config.REPO_ROOT / "ontology"
    result = validate_ontology(ontology_dir)
    markdown = format_ontology_validation_markdown(result)
    print(markdown)

    if args.out:
        out_path = Path(args.out)
        write_report(markdown, out_path)
        print(f"\nOntology validation report written to: {out_path}")

    if not result.clean:
        sys.exit(2)


def cmd_generate_proof(args: argparse.Namespace) -> None:
    from auditor.generators.proof import generate_proof

    statement = ""
    if args.statement_file:
        statement = Path(args.statement_file).read_text(encoding="utf-8")

    latex = generate_proof(
        theorem_label=args.label,
        theorem_name=args.name or args.label,
        theorem_statement=statement,
        mode=args.mode,
        proof_content=args.proof_content or "",
    )
    _print_and_optionally_write(latex, args)


def cmd_generate_proof_stubs(args: argparse.Namespace) -> None:
    from auditor.generators.proof_stubs import generate_chapter_proof_stubs

    report = generate_chapter_proof_stubs(
        chapter_path=Path(args.path),
        write=args.write,
        overwrite=args.overwrite,
        update_chapter_yaml=args.update_chapter_yaml,
    )

    mode = "WRITE" if args.write else "DRY_RUN"
    print("# Proof Stub Generation")
    print(f"- **Mode:** {mode}")
    print(f"- **Chapter:** `{report['chapter_path']}`")
    print(f"- **Update chapter.yaml:** {report['update_chapter_yaml']}")
    print(f"- **Items:** {len(report['items'])}")
    print()
    print("| Statement Label | Proof Label | Status | Output | Message |")
    print("|-----------------|-------------|--------|--------|---------|")
    for item in report["items"]:
        print(
            f"| `{item['label']}` | `{item['proof_label']}` | "
            f"{item['status']} | `{item['output_file']}` | {item['message']} |"
        )


def cmd_generate_stub_chapter(args: argparse.Namespace) -> None:
    from auditor.generators.stub_chapter import generate_stub_chapter

    registry = _load_chapter_registry_from_volume(args.volume)
    files    = generate_stub_chapter(
        volume_path=args.volume,
        chapter_subject=args.subject,
        chapter_display_title=args.title,
        chapter_registry=registry,
        sections_known=getattr(args, "sections_known", False),
    )

    _print_generated_files(files, args)


def cmd_generate_stub_volume(args: argparse.Namespace) -> None:
    from auditor.generators.stub_volume import generate_stub_volume

    registry = []
    if args.registry_file:
        with open(args.registry_file, encoding="utf-8") as f:
            registry = yaml.safe_load(f)

    files = generate_stub_volume(
        volume_identifier=args.volume,
        volume_display_title=args.title,
        volume_scope=args.scope or "",
        chapter_registry=registry,
    )
    _print_generated_files(files, args)


def cmd_generate_breadcrumb(args: argparse.Namespace) -> None:
    from auditor.generators.breadcrumb import generate_breadcrumb

    registry = _load_chapter_registry_from_volume(args.volume)
    latex = generate_breadcrumb(
        chapter_subject=args.subject,
        chapter_display_title=args.title,
        chapter_registry=registry,
        is_stub=getattr(args, "stub", False),
    )
    _print_and_optionally_write(latex, args)


def cmd_generate_capstone(args: argparse.Namespace) -> None:
    from auditor.generators.capstone import generate_capstone

    registry = _load_chapter_registry_from_volume(args.volume)
    environments = _load_chapter_environments(Path(args.chapter_path))

    latex = generate_capstone(
        chapter_subject=args.subject,
        chapter_display_title=args.title,
        chapter_registry=registry,
        chapter_environments=environments,
        mode=args.mode,
    )
    _print_and_optionally_write(latex, args)


def cmd_test_ai(args: argparse.Namespace) -> None:
    settings = client.settings()
    print(f"Testing AI provider: {settings.provider}")
    print(f"Model: {settings.model}")
    print(f"Base URL: {settings.provider_url}")
    print(f"API key env: {settings.api_key_env}")
    response = client.test_connection()
    print(f"Provider response: {response}")
    print("AI provider plumbing OK.")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _load_chapter_registry(chapter_path: Path) -> list[dict]:
    """Loads chapter registry from the parent volume's chapter.yaml."""
    volume_yaml = chapter_path.parent / "chapter.yaml"
    if volume_yaml.exists():
        with open(volume_yaml, encoding="utf-8") as f:
            data = yaml.safe_load(f)
        return data.get("chapters", [])
    return []


def _load_chapter_registry_from_volume(volume: str) -> list[dict]:
    volume_yaml = config.REPO_ROOT / volume / "chapter.yaml"
    if volume_yaml.exists():
        with open(volume_yaml, encoding="utf-8") as f:
            data = yaml.safe_load(f)
        return data.get("chapters", [])
    return []


def _load_chapter_environments(chapter_path: Path) -> list[dict]:
    yaml_path = chapter_path / "chapter.yaml"
    if yaml_path.exists():
        with open(yaml_path, encoding="utf-8") as f:
            data = yaml.safe_load(f)
        return data.get("environments", [])
    return []


def _print_and_optionally_write(content: str, args: argparse.Namespace) -> None:
    print(content)
    if getattr(args, "out", None):
        Path(args.out).write_text(content, encoding="utf-8")
        print(f"\nWritten to: {args.out}")


def _print_generated_files(files: dict[str, str], args: argparse.Namespace) -> None:
    for filename, content in files.items():
        print(f"\n### File: {filename}\n")
        print(content)

    if getattr(args, "write", False):
        for filename, content in files.items():
            if filename == "_raw_output":
                print("WARNING: could not parse file structure from model output.")
                continue
            out_path = Path(filename)
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_text(content, encoding="utf-8")
            print(f"Written: {out_path}")


# ---------------------------------------------------------------------------
# Argument parser
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="python -m auditor",
        description="LaTeX mathematics repository auditor and generator.",
    )
    parser.add_argument(
        "-ai",
        "--ai",
        default=AI_PROVIDER,
        type=normalize_provider,
        choices=sorted(VALID_AI_PROVIDERS),
        help="AI provider for audit/generate commands: anthropic or codex. Defaults to AUDITOR_AI_PROVIDER or Anthropic.",
    )
    parser.add_argument(
        "--repoDir",
        help="Path to the repository root. Overrides REPO_ROOT and auto-discovery.",
    )
    parser.add_argument(
        "-test",
        "--test",
        action="store_true",
        help="Connect to the selected AI provider, issue a simple test request, and exit.",
    )
    sub = parser.add_subparsers(dest="command")

    # ---- audit ----
    audit = sub.add_parser("audit", help="Audit operations")
    audit_sub = audit.add_subparsers(dest="audit_target", required=True)

    # audit statement
    p = audit_sub.add_parser("statement", help="Audit a single statement environment")
    p.add_argument("file",    help="Path to the .tex file containing the environment")
    p.add_argument("--label", required=True, help="LaTeX label e.g. def:upper-bound")
    p.add_argument("--type",  required=True,
                   choices=["def", "thm", "lem", "prop", "cor", "ax"])
    p.add_argument("--chapter", help="Chapter subject name (inferred from path if omitted)")
    p.set_defaults(func=cmd_audit_statement)

    # audit proof
    p = audit_sub.add_parser("proof", help="Audit a proof file")
    p.add_argument("file",      help="Path to the proof .tex file")
    p.add_argument("--chapter", help="Chapter subject name (inferred from path if omitted)")
    p.set_defaults(func=cmd_audit_proof)

    # audit stub
    p = audit_sub.add_parser("stub", help="Audit a chapter stub structure")
    p.add_argument("path", help="Path to the chapter directory")
    p.set_defaults(func=cmd_audit_stub)

    # audit symbols
    p = audit_sub.add_parser("symbols", help="Audit predicate/notation/relation usage")
    p.add_argument("path", help="Path to the chapter directory")
    p.set_defaults(func=cmd_audit_symbols)

    # audit chapter
    p = audit_sub.add_parser("chapter", help="Full chapter audit (all environments + proofs)")
    p.add_argument("path", help="Path to the chapter directory")
    p.add_argument(
        "--resume",
        help="Resume a previous chapter audit from a run directory, run.json path, or 'latest'.",
    )
    p.set_defaults(func=cmd_audit_chapter)

    # audit toolkits
    p = audit_sub.add_parser("toolkits", help="Deterministically audit section-level toolkit placement")
    p.add_argument("path", help="Path to the chapter directory")
    p.add_argument("--plan", help="Path to an approved toolkit-plan JSON file")
    p.set_defaults(func=cmd_audit_toolkits)

    # audit box colors
    p = audit_sub.add_parser("box-colors", help="Audit tcolorbox colback/colframe usage against common/colors.tex")
    p.add_argument("path", help="Path to a chapter, directory, or .tex file")
    p.set_defaults(func=cmd_audit_box_colors)

    # ---- plan ----
    plan = sub.add_parser("plan", help="Planning operations")
    plan_sub = plan.add_subparsers(dest="plan_target", required=True)

    p = plan_sub.add_parser("toolkits", help="Ask AI to propose section-level toolkit groupings")
    p.add_argument("path", help="Path to the chapter directory")
    p.set_defaults(func=cmd_plan_toolkits)

    p = plan_sub.add_parser("proofs-to-do", help="Write root proofs-to-do.md from chapter.yaml manifests")
    p.add_argument(
        "--types",
        nargs="+",
        default=sorted(["thm", "lem", "prop", "cor"]),
        help="Environment types to include. Defaults to thm lem prop cor.",
    )
    p.add_argument(
        "--ignore-existing-todo",
        action="store_true",
        help="Only report missing/unlisted proof files; ignore TODO markers inside existing proof files.",
    )
    p.add_argument("--out", help="Output markdown path. Defaults to <repo>/proofs-to-do.md")
    p.set_defaults(func=cmd_plan_proofs_to_do)

    # ---- patch ----
    patch = sub.add_parser("patch", help="Deterministic source patch operations")
    patch_sub = patch.add_subparsers(dest="patch_target", required=True)

    p = patch_sub.add_parser("safe", help="Classify and apply safe non-AI audit remediations")
    p.add_argument("report_dir", help="Completed audit report directory containing run.json")
    p.add_argument("--apply", action="store_true", help="Apply safe patches to live source files")
    p.set_defaults(func=cmd_patch_safe)

    p = patch_sub.add_parser("generated", help="Patch a validated generated statement block into source")
    p.add_argument("chapter_path", help="Path to the chapter directory")
    p.add_argument("--label", required=True, help="Canonical label to replace")
    p.add_argument("--generated", required=True, help="Path to generated .tex block")
    p.add_argument("--type", choices=["def", "thm", "lem", "prop", "cor", "ax"])
    p.add_argument("--out-dir", help="Directory for dry-run diff and validation artifacts")
    p.add_argument("--apply", action="store_true", help="Apply the generated replacement to live source")
    p.set_defaults(func=cmd_patch_generated)

    p = patch_sub.add_parser("generated-batch", help="Batch generate/validate/patch generated statement blocks")
    p.add_argument("plan", help="Path to JSON/YAML batch plan")
    p.add_argument("--generate-missing", action="store_true", help="Generate missing output files using AI")
    p.add_argument("--out-dir", help="Batch output directory for generated files, diffs, and summaries")
    p.add_argument("--apply", action="store_true", help="Apply validated generated replacements to live source")
    p.set_defaults(func=cmd_patch_generated_batch)

    p = patch_sub.add_parser("comment-blocks", help="Insert readable comment blocks before theorem-like environments")
    p.add_argument("path", help="Chapter directory, source directory, or .tex file")
    p.add_argument("--apply", action="store_true", help="Apply the comment headers to live source")
    p.add_argument(
        "--include-exercises",
        action="store_true",
        help="Also process exercise and capstone files. By default these are skipped.",
    )
    p.set_defaults(func=cmd_patch_comment_blocks)

    # ---- validate ----
    validate = sub.add_parser("validate", help="Deterministic validation operations")
    validate_sub = validate.add_subparsers(dest="validate_target", required=True)

    p = validate_sub.add_parser("generated", help="Validate a generated statement block before source patching")
    p.add_argument("file", help="Path to the generated .tex block")
    p.add_argument("--type", choices=["def", "thm", "lem", "prop", "cor", "ax"])
    p.add_argument("--label", help="Expected canonical label")
    p.add_argument("--out", help="Optional markdown report output path")
    p.set_defaults(func=cmd_validate_generated)

    p = validate_sub.add_parser("ontology", help="Validate ontology YAML registries and migration targets")
    p.add_argument("path", nargs="?", help="Ontology directory. Defaults to <repo>/ontology")
    p.add_argument("--out", help="Optional markdown report output path")
    p.set_defaults(func=cmd_validate_ontology)

    # ---- index ----
    index = sub.add_parser("index", help="Generated repository indexes")
    index_sub = index.add_subparsers(dest="index_target", required=True)

    p = index_sub.add_parser("labels", help="Index formal mathematical labels")
    p.add_argument("path", help="Path to scan for formal math environments")
    p.add_argument("--out-dir", help="Directory for generated index files")
    p.set_defaults(func=cmd_index_labels)

    # ---- scan ----
    scan = sub.add_parser("scan", help="Scan operations (bootstrap chapter.yaml)")
    scan_sub = scan.add_subparsers(dest="scan_target", required=True)

    p = scan_sub.add_parser("chapter", help="Scan chapter directory and propose chapter.yaml")
    p.add_argument("path",    help="Path to the chapter directory")
    p.add_argument("--write", action="store_true", help="Write the chapter.yaml to disk")
    p.set_defaults(func=cmd_scan_chapter)

    # ---- trueup ----
    trueup = sub.add_parser("trueup", help="Diff scan against chapter.yaml")
    trueup_sub = trueup.add_subparsers(dest="trueup_target", required=True)

    p = trueup_sub.add_parser("chapter", help="Diff scan vs chapter.yaml")
    p.add_argument("path", help="Path to the chapter directory")
    p.set_defaults(func=cmd_trueup_chapter)

    # ---- generate ----
    gen = sub.add_parser("generate", help="Generation operations")
    gen_sub = gen.add_subparsers(dest="gen_target", required=True)

    # generate statement
    p = gen_sub.add_parser("statement", help="Generate a statement environment block")
    p.add_argument("--type",    required=True,
                   choices=["def", "thm", "lem", "prop", "cor", "ax"])
    p.add_argument("--subject", required=True, help="Mathematical content description")
    p.add_argument("--chapter", required=True, help="Chapter subject name")
    p.add_argument("--volume",  help="Volume path for chapter registry context")
    p.add_argument("--label",   help="Canonical label to use for the generated environment")
    p.add_argument("--out",     help="Write output to this file path")
    p.add_argument("--validate", action="store_true", help="Run deterministic generated-block validation after generation")
    p.set_defaults(func=cmd_generate_statement)

    # generate proof
    p = gen_sub.add_parser("proof", help="Generate a proof file")
    p.add_argument("--label",          required=True, help="Theorem label e.g. thm:cauchy-criterion")
    p.add_argument("--name",           help="Theorem display name")
    p.add_argument("--statement-file", dest="statement_file",
                   help="Path to file containing the theorem statement LaTeX")
    p.add_argument("--mode",           default="stub", choices=["stub", "full"])
    p.add_argument("--proof-content",  dest="proof_content",
                   help="Draft proof content (for full mode)")
    p.add_argument("--out",            help="Write output to this file path")
    p.set_defaults(func=cmd_generate_proof)

    # generate proof-stubs
    p = gen_sub.add_parser("proof-stubs", help="Generate missing chapter proof stub files")
    p.add_argument("path", help="Path to the chapter directory")
    p.add_argument("--write", action="store_true", help="Write generated proof stubs to disk")
    p.add_argument("--overwrite", action="store_true", help="Overwrite existing output files")
    p.add_argument(
        "--update-chapter-yaml",
        action="store_true",
        help="Populate missing proof_file/proof_files entries in chapter.yaml",
    )
    p.set_defaults(func=cmd_generate_proof_stubs)

    # generate stub-chapter
    p = gen_sub.add_parser("stub-chapter", help="Generate a chapter stub")
    p.add_argument("--volume",  required=True, help="Volume path e.g. volume-iii")
    p.add_argument("--subject", required=True, help="Chapter subject name")
    p.add_argument("--title",   required=True, help="Chapter display title")
    p.add_argument("--write",   action="store_true", help="Write files to disk")
    p.set_defaults(func=cmd_generate_stub_chapter)

    # generate stub-volume
    p = gen_sub.add_parser("stub-volume", help="Generate a volume stub")
    p.add_argument("--volume",        required=True, help="Volume identifier e.g. volume-iii")
    p.add_argument("--title",         required=True, help="Volume display title")
    p.add_argument("--scope",         help="Volume scope description")
    p.add_argument("--registry-file", dest="registry_file",
                   help="Path to YAML file containing chapter registry")
    p.add_argument("--write",         action="store_true", help="Write files to disk")
    p.set_defaults(func=cmd_generate_stub_volume)

    # generate breadcrumb
    p = gen_sub.add_parser("breadcrumb", help="Generate a breadcrumb box")
    p.add_argument("--subject", required=True, help="Chapter subject name")
    p.add_argument("--title",   required=True, help="Chapter display title")
    p.add_argument("--volume",  required=True, help="Volume path for registry")
    p.add_argument("--stub",    action="store_true", help="Append Status: Planned box")
    p.add_argument("--out",     help="Write output to this file path")
    p.set_defaults(func=cmd_generate_breadcrumb)

    # generate capstone
    p = gen_sub.add_parser("capstone", help="Generate a capstone exercise")
    p.add_argument("--subject",       required=True, help="Chapter subject name")
    p.add_argument("--title",         required=True, help="Chapter display title")
    p.add_argument("--volume",        required=True, help="Volume path for registry")
    p.add_argument("--chapter-path",  dest="chapter_path", required=True,
                   help="Path to chapter directory (for loading environments)")
    p.add_argument("--mode",          default="stub", choices=["stub", "full"])
    p.add_argument("--out",           help="Write output to this file path")
    p.set_defaults(func=cmd_generate_capstone)

    return parser


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = build_parser()
    args   = parser.parse_args()
    if args.repoDir:
        config.set_repo_root(args.repoDir)
        from auditor import loader
        loader.clear_cache()
    client.set_provider(args.ai)

    if not args.command and not args.test:
        parser.error("one of {audit,plan,patch,validate,index,scan,trueup,generate} is required unless -test is supplied")

    generate_target = getattr(args, "gen_target", "")
    deterministic_plan = args.command == "plan" and getattr(args, "plan_target", "") == "proofs-to-do"
    require_ai = args.test or (args.command == "generate" and generate_target != "proof-stubs") or (
        args.command == "audit" and getattr(args, "audit_target", "") not in {"toolkits"}
    ) or (args.command == "plan" and not deterministic_plan)
    errors = validate_config(ai_provider=args.ai, require_ai=require_ai)
    hard_errors = [e for e in errors if not e.startswith("WARNING")]
    warnings    = [e for e in errors if e.startswith("WARNING")]

    for w in warnings:
        print(f"  {w}", file=sys.stderr)

    if hard_errors:
        for e in hard_errors:
            print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)

    if args.test:
        try:
            cmd_test_ai(args)
        except Exception as exc:
            print(f"ERROR: AI provider test failed: {exc}", file=sys.stderr)
            sys.exit(1)
        return

    args.func(args)
