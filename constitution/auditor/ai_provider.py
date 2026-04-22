"""
ai_provider.py
Provider-neutral AI configuration for the auditor.

The rest of the auditor should depend on ProviderSettings instead of reading
provider-specific environment variables directly.
"""

from __future__ import annotations

import os
from dataclasses import dataclass


VALID_AI_PROVIDERS = {"Anthropic", "Codex"}


@dataclass(frozen=True)
class ProviderSettings:
    provider: str
    api_key: str
    api_key_env: str
    provider_url: str
    provider_url_env: str
    model: str
    model_env: str
    max_tokens: int

    @property
    def has_api_key(self) -> bool:
        return bool(self.api_key)


def normalize_provider(provider: str | None) -> str:
    value = (provider or os.environ.get("AUDITOR_AI_PROVIDER") or "Anthropic").strip()
    if value.lower() == "anthropic":
        return "Anthropic"
    if value.lower() == "codex":
        return "Codex"
    raise ValueError(f"Unknown AI provider: {value}. Expected Anthropic or Codex.")


def _first_env(*names: str) -> tuple[str, str]:
    for name in names:
        value = os.environ.get(name, "")
        if value:
            return value, name
    return "", names[-1] if names else ""


def _int_env(name: str, default: int) -> int:
    value = os.environ.get(name, "")
    if not value:
        return default
    try:
        return int(value)
    except ValueError:
        return default


def get_ai_provider_settings(provider: str | None = None) -> ProviderSettings:
    selected = normalize_provider(provider)
    max_tokens = _int_env("AUDITOR_MAX_TOKENS", 4096)

    if selected == "Anthropic":
        api_key, api_key_env = _first_env("ANTHROPIC_API_KEY", "AI_API_KEY")
        provider_url, provider_url_env = _first_env(
            "ANTHROPIC_PROVIDER_URL",
            "ANTHROPIC_BASE_URL",
            "AI_PROVIDER_URL",
        )
        return ProviderSettings(
            provider=selected,
            api_key=api_key,
            api_key_env=api_key_env,
            provider_url=provider_url or "https://api.anthropic.com",
            provider_url_env=provider_url_env,
            model=os.environ.get("ANTHROPIC_MODEL", "claude-sonnet-4-20250514"),
            model_env="ANTHROPIC_MODEL",
            max_tokens=max_tokens,
        )

    api_key, api_key_env = _first_env("OPENAI_API_KEY", "CODEX_API_KEY", "AI_API_KEY")
    provider_url, provider_url_env = _first_env(
        "OPENAI_PROVIDER_URL",
        "OPENAI_BASE_URL",
        "CODEX_PROVIDER_URL",
        "CODEX_BASE_URL",
        "AI_PROVIDER_URL",
    )
    return ProviderSettings(
        provider=selected,
        api_key=api_key,
        api_key_env=api_key_env,
        provider_url=provider_url or "https://api.openai.com/v1",
        provider_url_env=provider_url_env,
        model=os.environ.get("CODEX_MODEL") or os.environ.get("OPENAI_MODEL", "gpt-5.1-codex"),
        model_env="CODEX_MODEL",
        max_tokens=max_tokens,
    )
