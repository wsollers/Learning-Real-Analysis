"""
client.py
Thin wrapper around model providers.
All AI calls in the entire codebase go through here.
Handles prompt assembly, provider dispatch, and response parsing.
"""

import json
import re
from typing import Any

import anthropic

from auditor.ai_provider import ProviderSettings, get_ai_provider_settings
from auditor.loader import audit_report_schema


# ---------------------------------------------------------------------------
# Client singleton
# ---------------------------------------------------------------------------

_anthropic_client: anthropic.Anthropic | None = None
_openai_client: Any | None = None
_settings: ProviderSettings = get_ai_provider_settings()


def set_provider(provider: str) -> None:
    """Selects the AI provider for subsequent client.call invocations."""
    global _anthropic_client, _openai_client, _settings
    new_settings = get_ai_provider_settings(provider)
    if new_settings != _settings:
        _anthropic_client = None
        _openai_client = None
    _settings = new_settings


def provider() -> str:
    return _settings.provider


def settings() -> ProviderSettings:
    return _settings


def _get_anthropic_client() -> anthropic.Anthropic:
    global _anthropic_client
    if _anthropic_client is None:
        if not _settings.api_key:
            raise RuntimeError(
                "API key is not set for Anthropic. "
                "Set ANTHROPIC_API_KEY or AI_API_KEY."
            )
        _anthropic_client = anthropic.Anthropic(
            api_key=_settings.api_key,
            base_url=_settings.provider_url,
        )
    return _anthropic_client


def _get_openai_client() -> Any:
    global _openai_client
    if _openai_client is None:
        if not _settings.api_key:
            raise RuntimeError(
                "API key is not set for Codex. "
                "Set OPENAI_API_KEY, CODEX_API_KEY, or AI_API_KEY."
            )
        try:
            from openai import OpenAI
        except ImportError as exc:
            raise RuntimeError(
                "The openai package is not installed. "
                "Run: python -m pip install openai"
            ) from exc
        _openai_client = OpenAI(
            api_key=_settings.api_key,
            base_url=_settings.provider_url,
        )
    return _openai_client


# ---------------------------------------------------------------------------
# Core call
# ---------------------------------------------------------------------------

def call(
    system: str,
    user: str,
    *,
    expect_json: bool = True,
    validate_report: bool = True,
) -> dict | str:
    """
    Sends a single request to the API.

    Args:
        system:      The system prompt (assembled by the caller).
        user:        The user message (the content to audit or generate for).
        expect_json: If True, parses and validates the response as JSON.
                     If False, returns the raw text string (used for symbol audit
                     which returns markdown).

    Returns:
        Parsed dict if expect_json=True.
        Raw string if expect_json=False.

    Raises:
        RuntimeError on API errors or JSON parse failures.
    """
    raw = _call_anthropic(system, user) if _settings.provider == "Anthropic" else _call_codex(system, user)

    if not expect_json:
        return raw

    return _parse_json_response(raw, validate_report=validate_report)


def test_connection() -> str:
    """
    Sends a tiny provider request to verify API key, package, URL, model,
    and network plumbing before a long audit run.
    """
    raw = call(
        "You are a connectivity test for an automated CLI. Reply with exactly OK.",
        "Reply with exactly OK.",
        expect_json=False,
    )
    response = raw.strip()
    if not response:
        raise RuntimeError("Provider returned an empty response.")
    return response


def _call_anthropic(system: str, user: str) -> str:
    client = _get_anthropic_client()
    response = client.messages.create(
        model=_settings.model,
        max_tokens=_settings.max_tokens,
        system=system,
        messages=[{"role": "user", "content": user}],
    )
    return response.content[0].text


def _call_codex(system: str, user: str) -> str:
    client = _get_openai_client()
    response = client.responses.create(
        model=_settings.model,
        input=[
            {"role": "system", "content": system},
            {"role": "user", "content": user},
        ],
    )
    return response.output_text


def _audit_report_schema_prompt() -> str:
    schema = json.dumps(audit_report_schema(), indent=2)
    return (
        "\n\n## Required JSON Output Schema\n\n"
        "Return exactly one JSON object and no surrounding prose. "
        "The object must conform to this schema. The top-level object must "
        "include audit_type, artifact_type, label, summary, checks, and "
        "violations.\n\n"
        f"```json\n{schema}\n```"
    )


def _parse_json_response(raw: str, *, validate_report: bool = True) -> dict:
    """
    Parses a JSON response from the model.
    Strips markdown code fences if present.
    Validates against the audit report schema structure.
    Raises RuntimeError on failure.
    """
    # Strip markdown fences if the model wrapped the JSON
    cleaned = raw.strip()
    if cleaned.startswith("```"):
        cleaned = re.sub(r"^```(?:json)?\s*", "", cleaned)
        cleaned = re.sub(r"\s*```$", "", cleaned)
        cleaned = cleaned.strip()

    if not cleaned.startswith("{"):
        match = re.search(r"\{.*\}", cleaned, flags=re.DOTALL)
        if match:
            cleaned = match.group(0)

    try:
        parsed = json.loads(cleaned)
    except json.JSONDecodeError as e:
        repaired = re.sub(r'\\(?!["\\/bfnrtu])', r"\\\\", cleaned)
        try:
            parsed = json.loads(repaired)
        except json.JSONDecodeError:
            raise RuntimeError(
                f"Model returned non-JSON response.\n"
                f"Parse error: {e}\n"
                f"Raw response (first 500 chars):\n{raw[:500]}"
            ) from e

    if validate_report:
        validate_audit_report(parsed)
    return parsed


def validate_audit_report(report: dict) -> None:
    """
    Validates that the parsed JSON has the required top-level fields
    of the audit report schema. Not a full JSON Schema validation —
    just a structural sanity check to catch malformed responses early.
    """
    required_fields = {"audit_type", "artifact_type", "label", "summary", "checks", "violations"}
    missing = required_fields - set(report.keys())
    if missing:
        raise RuntimeError(
            f"Model response is missing required audit report fields: {missing}\n"
            f"Report keys present: {list(report.keys())}\n"
            f"Report preview: {json.dumps(report, ensure_ascii=False)[:1000]}"
        )


# ---------------------------------------------------------------------------
# Prompt assembly helpers
# ---------------------------------------------------------------------------

def assemble_audit_system_prompt(
    base_prompt: str,
    *,
    block_registry_yaml: str = "",
    artifact_matrix_row: str = "",
    artifact_type: str = "",
) -> str:
    """
    Assembles the system prompt for a statement audit call.
    Concatenates base prompt + registry + matrix row in that order.
    """
    parts = [base_prompt]
    parts.append(_audit_report_schema_prompt())

    if artifact_type:
        parts.append(f"\n\n## Artifact Type\n\n{artifact_type}")

    if artifact_matrix_row:
        parts.append(
            f"\n\n## Requirement Row for This Artifact Type\n\n"
            f"```yaml\n{artifact_matrix_row}\n```"
        )

    if block_registry_yaml:
        parts.append(
            f"\n\n## Block Registry\n\n"
            f"```yaml\n{block_registry_yaml}\n```"
        )

    return "\n".join(parts)


def assemble_symbol_audit_system_prompt(
    base_prompt: str,
    *,
    predicates_yaml: str,
    notation_yaml: str,
    relations_yaml: str,
) -> str:
    """
    Assembles the system prompt for a symbol audit call.
    Injects all three canonical source files.
    """
    return (
        f"{base_prompt}\n\n"
        f"## predicates.yaml\n\n```yaml\n{predicates_yaml}\n```\n\n"
        f"## notation.yaml\n\n```yaml\n{notation_yaml}\n```\n\n"
        f"## relations.yaml\n\n```yaml\n{relations_yaml}\n```"
    )


def assemble_generate_system_prompt(
    base_prompt: str,
    *,
    block_registry_yaml: str = "",
    artifact_matrix_row: str = "",
    artifact_type: str = "",
    predicates_yaml: str = "",
    notation_yaml: str = "",
    relations_yaml: str = "",
    chapter_registry: str = "",
    formal_label_index: str = "",
    candidate_label_context: str = "",
) -> str:
    """
    Assembles the system prompt for a generation call.
    """
    parts = [base_prompt]

    if artifact_type:
        parts.append(f"\n\n## Artifact Type\n\n{artifact_type}")

    if artifact_matrix_row:
        parts.append(
            f"\n\n## Requirement Row for This Artifact Type\n\n"
            f"```yaml\n{artifact_matrix_row}\n```"
        )

    if block_registry_yaml:
        parts.append(
            f"\n\n## Block Registry\n\n"
            f"```yaml\n{block_registry_yaml}\n```"
        )

    if predicates_yaml:
        parts.append(f"\n\n## predicates.yaml\n\n```yaml\n{predicates_yaml}\n```")

    if notation_yaml:
        parts.append(f"\n\n## notation.yaml\n\n```yaml\n{notation_yaml}\n```")

    if relations_yaml:
        parts.append(f"\n\n## relations.yaml\n\n```yaml\n{relations_yaml}\n```")

    if chapter_registry:
        parts.append(f"\n\n## Chapter Registry\n\n```yaml\n{chapter_registry}\n```")

    if formal_label_index:
        parts.append(f"\n\n## Formal Mathematical Label Index\n\n```json\n{formal_label_index}\n```")

    if candidate_label_context:
        parts.append(f"\n\n## Candidate Existing Labels\n\n{candidate_label_context}")

    return "\n".join(parts)
