"""
loader.py
Reads and caches constitution files, canonical source files, and prompts.
All file I/O for static assets goes through here.
Nothing else reads these files directly.
"""

import json
import functools
from pathlib import Path

import yaml

from auditor import config


# ---------------------------------------------------------------------------
# Internal cache
# ---------------------------------------------------------------------------

_cache: dict[str, object] = {}


def _load_yaml(path: Path) -> dict:
    if str(path) not in _cache:
        with open(path, "r", encoding="utf-8") as f:
            _cache[str(path)] = yaml.safe_load(f)
    return _cache[str(path)]


def _load_text(path: Path) -> str:
    if str(path) not in _cache:
        with open(path, "r", encoding="utf-8") as f:
            _cache[str(path)] = f.read()
    return _cache[str(path)]


def _load_json(path: Path) -> dict:
    if str(path) not in _cache:
        with open(path, "r", encoding="utf-8") as f:
            _cache[str(path)] = json.load(f)
    return _cache[str(path)]


# ---------------------------------------------------------------------------
# Public accessors
# ---------------------------------------------------------------------------

def block_registry() -> dict:
    """Returns the full block registry as a dict keyed by block id."""
    raw = _load_yaml(config.BLOCK_REGISTRY_PATH)
    return {block["id"]: block for block in raw["blocks"]}


def artifact_matrix() -> dict:
    """Returns the artifact matrix as a dict keyed by block_id → {artifact_type: requirement}."""
    return _load_yaml(config.ARTIFACT_MATRIX_PATH)["matrix"]


def file_schema() -> dict:
    """Returns the full file schema dict."""
    return _load_yaml(config.FILE_SCHEMA_PATH)


def audit_report_schema() -> dict:
    """Returns the JSON schema for audit reports."""
    return _load_json(config.AUDIT_REPORT_SCHEMA_PATH)


def prompt(name: str) -> str:
    """
    Returns the text of the named prompt file.
    name must be a key from config.PROMPTS.
    Raises KeyError if name is unknown, FileNotFoundError if file is missing.
    """
    path = config.PROMPTS[name]
    return _load_text(path)


def canonical_source(name: str) -> str:
    """
    Returns the raw text of a canonical source file (predicates, notation, relations).
    Returns empty string with a warning if the file does not exist.
    """
    path = config.CANONICAL_SOURCES[name]
    if not path.exists():
        return f"# WARNING: {name}.yaml not found at {path}\n"
    return _load_text(path)


def matrix_row(artifact_type: str) -> dict[str, str]:
    """
    Returns the requirement mapping for a single artifact type.
    e.g. matrix_row("def") → {"toolkit_box": "R", "box": "R", "proof_link": "N", ...}
    """
    matrix = artifact_matrix()
    row = {}
    for block_id, type_map in matrix.items():
        row[block_id] = type_map.get(artifact_type, "N")
    return row


def registry_entry(block_id: str) -> dict:
    """Returns the full registry entry for a single block id."""
    reg = block_registry()
    if block_id not in reg:
        raise KeyError(f"Unknown block id: {block_id}")
    return reg[block_id]


def all_block_ids() -> list[str]:
    """Returns block ids in registry order."""
    raw = _load_yaml(config.BLOCK_REGISTRY_PATH)
    return [block["id"] for block in raw["blocks"]]


def clear_cache() -> None:
    """Clears the file cache. Useful for testing."""
    _cache.clear()
