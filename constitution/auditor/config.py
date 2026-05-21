"""
config.py
All repository paths and API settings.
Update this file when the repository layout changes.
Nothing else in the codebase hardcodes paths.

Multi-repo layout (as of 2026)
-------------------------------
The project is split across several repos:

  Learning-Real-Analysis  — monorepo: constitution, canonical YAMLs, auditor,
                            docker build, full omnibus LaTeX build
  lra-common              — shared LaTeX infrastructure (common/, bibliography/)
                            synced to all volume repos via GitHub Actions
  lra-volume-i .. v       — per-volume Overleaf targets; each is self-contained
  lra-lean                — Lean 4 formalization
  lra-nurbs               — NURBS/DDE C++ engine
  lra-knowledge-explorer  — theorem extraction pipeline + HTML graph

The auditor always loads constitution files and canonical YAMLs from the monorepo.
When working in a volume repo, set REPO_ROOT (or pass --repoDir) to point at the
Learning-Real-Analysis clone.

The LaTeX content to audit may live in a different directory from the monorepo.
Use set_latex_root() to set that path separately.
"""

import os
from pathlib import Path

from auditor.ai_provider import get_ai_provider_settings, normalize_provider

# ---------------------------------------------------------------------------
# Repository root (monorepo — constitution, canonical YAMLs, auditor)
# Can be set via --repoDir, environment variable REPO_ROOT, or auto-discovery.
# ---------------------------------------------------------------------------

def _discover_repo_root() -> Path:
    explicit = os.environ.get("REPO_ROOT")
    if explicit:
        return Path(explicit).resolve()
    start = Path.cwd().resolve()
    for path in [start, *start.parents]:
        if (path / "constitution" / "master.md").exists():
            return path
        if path.name == "constitution" and (path / "master.md").exists():
            return path.parent
    return start


def set_repo_root(repo_dir: str | Path | None = None) -> Path:
    """
    Sets the monorepo root (Learning-Real-Analysis) and recomputes all derived
    path globals for constitution files, prompts, and canonical YAMLs.

    This must always point at the Learning-Real-Analysis monorepo, regardless
    of which volume repo you are currently editing.
    """
    global REPO_ROOT, CONSTITUTION_DIR, SCHEMA_DIR, PROMPTS_DIR, RESPONSE_SCHEMA_DIR
    global BLOCK_REGISTRY_PATH, ARTIFACT_MATRIX_PATH, FILE_SCHEMA_PATH, AUDIT_REPORT_SCHEMA_PATH
    global PROMPTS, PREDICATES_PATH, NOTATION_PATH, RELATIONS_PATH, CANONICAL_SOURCES, REPORTS_DIR

    REPO_ROOT = Path(repo_dir).resolve() if repo_dir else _discover_repo_root()

    CONSTITUTION_DIR = REPO_ROOT / "constitution"
    SCHEMA_DIR = CONSTITUTION_DIR / "schema"
    PROMPTS_DIR = CONSTITUTION_DIR / "prompts"
    RESPONSE_SCHEMA_DIR = CONSTITUTION_DIR / "schemas"

    BLOCK_REGISTRY_PATH = SCHEMA_DIR / "block-registry.yaml"
    ARTIFACT_MATRIX_PATH = SCHEMA_DIR / "artifact-matrix.yaml"
    FILE_SCHEMA_PATH = SCHEMA_DIR / "file-schema.yaml"

    AUDIT_REPORT_SCHEMA_PATH = RESPONSE_SCHEMA_DIR / "audit-report.json"

    PROMPTS = {
        "audit_statement": PROMPTS_DIR / "audit-statement.md",
        "audit_proof": PROMPTS_DIR / "audit-proof.md",
        "audit_stub": PROMPTS_DIR / "audit-stub.md",
        "audit_symbols": PROMPTS_DIR / "audit-chapter-symbols.md",
        "plan_toolkits": PROMPTS_DIR / "plan-toolkits.md",
        "generate_statement": PROMPTS_DIR / "generate-statement.md",
        "generate_proof": PROMPTS_DIR / "generate-proof.md",
        "generate_stub_chapter": PROMPTS_DIR / "generate-stub-chapter.md",
        "generate_stub_volume": PROMPTS_DIR / "generate-stub-volume.md",
        "generate_breadcrumb": PROMPTS_DIR / "generate-breadcrumb.md",
        "generate_capstone": PROMPTS_DIR / "generate-capstone.md",
    }

    # Canonical YAML sources always live in the monorepo root.
    # They are never duplicated in volume repos.
    PREDICATES_PATH = REPO_ROOT / "predicates.yaml"
    NOTATION_PATH = REPO_ROOT / "notation.yaml"
    RELATIONS_PATH = REPO_ROOT / "relations.yaml"

    CANONICAL_SOURCES = {
        "predicates": PREDICATES_PATH,
        "notation": NOTATION_PATH,
        "relations": RELATIONS_PATH,
    }

    REPORTS_DIR = REPO_ROOT / "reports"
    return REPO_ROOT


# ---------------------------------------------------------------------------
# LaTeX content root — may differ from REPO_ROOT in the multi-repo layout.
#
# When the user is editing lra-volume-i (for example), they pass the volume
# repo path as the LaTeX root. The auditor scans LaTeX from there but loads
# constitution + canonical YAMLs from REPO_ROOT.
#
# Default: same as REPO_ROOT (monorepo layout, all volumes present).
# ---------------------------------------------------------------------------

LATEX_ROOT: Path = Path()


def set_latex_root(latex_dir: str | Path | None = None) -> Path:
    """
    Sets the LaTeX content root. Use this when auditing a volume repo checkout
    that is separate from the monorepo.

    Example (CLI):
        --latexDir /path/to/lra-volume-i
    """
    global LATEX_ROOT
    LATEX_ROOT = Path(latex_dir).resolve() if latex_dir else REPO_ROOT
    return LATEX_ROOT


# ---------------------------------------------------------------------------
# Global path variables — initialized by set_repo_root() at module load time.
# ---------------------------------------------------------------------------

REPO_ROOT = Path()
CONSTITUTION_DIR = Path()
SCHEMA_DIR = Path()
PROMPTS_DIR = Path()
RESPONSE_SCHEMA_DIR = Path()
BLOCK_REGISTRY_PATH = Path()
ARTIFACT_MATRIX_PATH = Path()
FILE_SCHEMA_PATH = Path()
AUDIT_REPORT_SCHEMA_PATH = Path()
PROMPTS: dict[str, Path] = {}
PREDICATES_PATH = Path()
NOTATION_PATH = Path()
RELATIONS_PATH = Path()
CANONICAL_SOURCES: dict[str, Path] = {}
REPORTS_DIR = Path()

set_repo_root()
set_latex_root()

# ---------------------------------------------------------------------------
# API settings
# ---------------------------------------------------------------------------

AI_PROVIDER = normalize_provider(os.environ.get("AUDITOR_AI_PROVIDER", "Anthropic"))
MAX_TOKENS  = int(os.environ.get("AUDITOR_MAX_TOKENS", "4096"))

# ---------------------------------------------------------------------------
# LaTeX environment names → artifact types
# ---------------------------------------------------------------------------

ENV_TO_TYPE: dict[str, str] = {
    "definition":   "def",
    "theorem":      "thm",
    "lemma":        "lem",
    "proposition":  "prop",
    "corollary":    "cor",
    "axiom":        "ax",
}

TYPE_TO_ENV: dict[str, str] = {v: k for k, v in ENV_TO_TYPE.items()}

THEOREM_LIKE_ENVS = set(ENV_TO_TYPE.keys())

# Environments that may have proof files
PROVABLE_TYPES = {"thm", "lem", "prop", "cor"}

# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate_config(ai_provider: str | None = None, require_ai: bool = True) -> list[str]:
    """
    Returns a list of error messages for any missing required files.
    Called at startup. If errors are returned the script should exit.
    """
    errors = []
    try:
        provider_settings = get_ai_provider_settings(ai_provider or AI_PROVIDER)
    except ValueError as exc:
        provider_settings = None
        errors.append(str(exc))

    required = [
        BLOCK_REGISTRY_PATH,
        ARTIFACT_MATRIX_PATH,
        FILE_SCHEMA_PATH,
        AUDIT_REPORT_SCHEMA_PATH,
        *PROMPTS.values(),
    ]

    for path in required:
        if not path.exists():
            errors.append(f"Missing constitution file: {path}")

    # Canonical source files are warned about but not hard errors —
    # they may not exist yet in a fresh repository.
    for name, path in CANONICAL_SOURCES.items():
        if not path.exists():
            errors.append(
                f"WARNING: Canonical source file missing: {path} "
                f"({name} audits will be skipped). "
                f"Set REPO_ROOT to the Learning-Real-Analysis monorepo path."
            )

    if require_ai and provider_settings and not provider_settings.has_api_key:
        if provider_settings.provider == "Anthropic":
            accepted = "ANTHROPIC_API_KEY or AI_API_KEY"
        else:
            accepted = "OPENAI_API_KEY, CODEX_API_KEY, or AI_API_KEY"
        errors.append(
            f"API key is not set for {provider_settings.provider}. "
            f"Set one of: {accepted}."
        )

    return errors
