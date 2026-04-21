"""
config.py
All repository paths and API settings.
Update this file when the repository layout changes.
Nothing else in the codebase hardcodes paths.
"""

import os
from pathlib import Path

from auditor.ai_provider import get_ai_provider_settings

# ---------------------------------------------------------------------------
# Repository root
# Must be set via environment variable REPO_ROOT or defaults to cwd.
# ---------------------------------------------------------------------------

REPO_ROOT = Path(os.environ.get("REPO_ROOT", ".")).resolve()

# ---------------------------------------------------------------------------
# Constitution paths
# ---------------------------------------------------------------------------

CONSTITUTION_DIR    = REPO_ROOT / "constitution"
SCHEMA_DIR          = CONSTITUTION_DIR / "schema"
PROMPTS_DIR         = CONSTITUTION_DIR / "prompts"
RESPONSE_SCHEMA_DIR = CONSTITUTION_DIR / "schemas"

BLOCK_REGISTRY_PATH  = SCHEMA_DIR / "block-registry.yaml"
ARTIFACT_MATRIX_PATH = SCHEMA_DIR / "artifact-matrix.yaml"
FILE_SCHEMA_PATH     = SCHEMA_DIR / "file-schema.yaml"

AUDIT_REPORT_SCHEMA_PATH = RESPONSE_SCHEMA_DIR / "audit-report.json"

# Prompt files
PROMPTS = {
    "audit_statement":      PROMPTS_DIR / "audit-statement.md",
    "audit_proof":          PROMPTS_DIR / "audit-proof.md",
    "audit_stub":           PROMPTS_DIR / "audit-stub.md",
    "audit_symbols":        PROMPTS_DIR / "audit-chapter-symbols.md",
    "generate_statement":   PROMPTS_DIR / "generate-statement.md",
    "generate_proof":       PROMPTS_DIR / "generate-proof.md",
    "generate_stub_chapter":PROMPTS_DIR / "generate-stub-chapter.md",
    "generate_stub_volume": PROMPTS_DIR / "generate-stub-volume.md",
    "generate_breadcrumb":  PROMPTS_DIR / "generate-breadcrumb.md",
    "generate_capstone":    PROMPTS_DIR / "generate-capstone.md",
}

# ---------------------------------------------------------------------------
# Canonical source files (repository root)
# ---------------------------------------------------------------------------

PREDICATES_PATH = REPO_ROOT / "predicates.yaml"
NOTATION_PATH   = REPO_ROOT / "notation.yaml"
RELATIONS_PATH  = REPO_ROOT / "relations.yaml"

CANONICAL_SOURCES = {
    "predicates": PREDICATES_PATH,
    "notation":   NOTATION_PATH,
    "relations":  RELATIONS_PATH,
}

# ---------------------------------------------------------------------------
# Reports directory
# ---------------------------------------------------------------------------

REPORTS_DIR = REPO_ROOT / "reports"

# ---------------------------------------------------------------------------
# API settings
# ---------------------------------------------------------------------------

AI_PROVIDER = os.environ.get("AUDITOR_AI_PROVIDER", "Anthropic")
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
                f"({name} audits will be skipped)"
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
