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
from auditor.config import AI_PROVIDER, validate_config, REPO_ROOT


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
    audit_chapter(chapter_path=Path(args.path))


def cmd_scan_chapter(args: argparse.Namespace) -> None:
    from auditor.scanner import scan_chapter, scan_result_to_yaml

    chapter_path = Path(args.path)
    result = scan_chapter(chapter_path)

    if result.warnings:
        print("\nScanner warnings:")
        for w in result.warnings:
            print(f"  ⚠  {w}")

    yaml_str = scan_result_to_yaml(result)

    print(f"\nScanned {len(result.environments)} environment(s) "
          f"and {len(result.proof_files)} proof file(s).\n")
    print("Proposed chapter.yaml (environments + proof_files sections):\n")
    print(yaml_str)

    yaml_path = chapter_path / "chapter.yaml"

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
    registry = _load_chapter_registry_from_volume(args.volume) if args.volume else None

    latex = generate_statement(
        artifact_type=args.type,
        content_description=args.subject,
        chapter_subject=args.chapter,
        chapter_registry=registry,
    )
    _print_and_optionally_write(latex, args)


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
    volume_yaml = REPO_ROOT / volume / "chapter.yaml"
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
        choices=["Anthropic", "Codex"],
        help="AI provider for audit/generate commands. Defaults to AUDITOR_AI_PROVIDER or Anthropic.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

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
    p.set_defaults(func=cmd_audit_chapter)

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
    p.add_argument("--out",     help="Write output to this file path")
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
    client.set_provider(args.ai)

    require_ai = args.command in {"audit", "generate"}
    errors = validate_config(ai_provider=args.ai, require_ai=require_ai)
    hard_errors = [e for e in errors if not e.startswith("WARNING")]
    warnings    = [e for e in errors if e.startswith("WARNING")]

    for w in warnings:
        print(f"  {w}", file=sys.stderr)

    if hard_errors:
        for e in hard_errors:
            print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)

    args.func(args)
