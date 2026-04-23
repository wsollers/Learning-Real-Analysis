from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml


REQUIRED_FILES = [
    "ontology.yaml",
    "notation.yaml",
    "terms.yaml",
    "predicates.yaml",
    "operators.yaml",
    "relations.yaml",
    "migration.yaml",
]

DOUBLE_QUOTED_BACKSLASH_RE = re.compile(r":\s*\"[^\"\n]*\\[^\"\n]*\"")


@dataclass
class OntologyValidationResult:
    root: Path
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    loaded_files: list[str] = field(default_factory=list)
    entity_count: int = 0
    relation_count: int = 0
    migration_count: int = 0

    @property
    def clean(self) -> bool:
        return not self.errors


def validate_ontology(ontology_dir: Path) -> OntologyValidationResult:
    result = OntologyValidationResult(root=ontology_dir)
    docs: dict[str, Any] = {}

    for name in REQUIRED_FILES:
        path = ontology_dir / name
        if not path.exists():
            result.errors.append(f"Missing required file: {name}")
            continue
        _check_yaml_quoting(path, result)
        try:
            docs[name] = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
            result.loaded_files.append(name)
        except Exception as exc:
            result.errors.append(f"{name}: YAML parse error: {exc}")

    if result.errors:
        return result

    registry = _build_registry(docs, result)
    _validate_entities(docs, result)
    _validate_relations(docs, registry, result)
    _validate_migration(docs, registry, result)
    _validate_operator_duals(docs, registry, result)
    _validate_derived_predicates(docs, registry, result)
    return result


def format_ontology_validation_markdown(result: OntologyValidationResult) -> str:
    status = "CLEAN" if result.clean else "FAIL"
    lines = [
        "# Ontology Validation",
        f"- **Directory:** {result.root}",
        f"- **Result:** {status}",
        f"- **Files loaded:** {len(result.loaded_files)}",
        f"- **Entities:** {result.entity_count}",
        f"- **Relations:** {result.relation_count}",
        f"- **Migration entries:** {result.migration_count}",
        "",
        "## Errors",
        "",
    ]
    lines.extend([f"- {msg}" for msg in result.errors] or ["_None._"])
    lines += ["", "## Warnings", ""]
    lines.extend([f"- {msg}" for msg in result.warnings] or ["_None._"])
    return "\n".join(lines) + "\n"


def _check_yaml_quoting(path: Path, result: OntologyValidationResult) -> None:
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if DOUBLE_QUOTED_BACKSLASH_RE.search(line):
            result.warnings.append(
                f"{path.name}:{line_no}: double-quoted string contains a backslash; "
                "prefer single quotes for LaTeX."
            )


def _build_registry(docs: dict[str, Any], result: OntologyValidationResult) -> dict[str, dict[str, Any]]:
    registry: dict[str, dict[str, Any]] = {}
    names_by_kind: dict[tuple[str, str], str] = {}
    names_across_kinds: dict[str, list[str]] = {}

    for file_name, list_key, expected_prefix, expected_kind in [
        ("terms.yaml", "terms", "term:", None),
        ("predicates.yaml", "predicates", "pred:", "predicate"),
        ("operators.yaml", "operators", "op:", "operator"),
    ]:
        for entity in docs[file_name].get(list_key, []) or []:
            entity_id = entity.get("id")
            name = entity.get("name")
            if not entity_id:
                result.errors.append(f"{file_name}: entity without id")
                continue
            if entity_id in registry:
                result.errors.append(f"Duplicate ontology id: {entity_id}")
                continue
            registry[entity_id] = {"file": file_name, **entity}
            result.entity_count += 1

            if not str(entity_id).startswith(expected_prefix):
                result.errors.append(f"{entity_id}: id must start with {expected_prefix}")
            if expected_kind and entity.get("kind") != expected_kind:
                result.errors.append(f"{entity_id}: kind must be {expected_kind}")
            if name:
                key = (file_name, name)
                if key in names_by_kind:
                    result.errors.append(f"Duplicate canonical name in {file_name}: {name}")
                names_by_kind[key] = entity_id
                names_across_kinds.setdefault(name, []).append(entity_id)

    for name, ids in names_across_kinds.items():
        if len(ids) > 1:
            result.warnings.append(f"Name appears in multiple registries: {name} -> {', '.join(ids)}")
    return registry


def _validate_entities(docs: dict[str, Any], result: OntologyValidationResult) -> None:
    for file_name, list_key in [
        ("terms.yaml", "terms"),
        ("predicates.yaml", "predicates"),
        ("operators.yaml", "operators"),
    ]:
        for entity in docs[file_name].get(list_key, []) or []:
            entity_id = entity.get("id", "<missing id>")
            for field_name in ["name", "kind", "category", "returns"]:
                if field_name not in entity:
                    result.errors.append(f"{entity_id}: missing required field `{field_name}`")
            if file_name != "operators.yaml" and "arguments" not in entity:
                result.errors.append(f"{entity_id}: missing required field `arguments`")
            if file_name == "operators.yaml" and "input_kind" not in entity:
                result.errors.append(f"{entity_id}: missing required field `input_kind`")


def _validate_relations(
    docs: dict[str, Any],
    registry: dict[str, dict[str, Any]],
    result: OntologyValidationResult,
) -> None:
    rel_doc = docs["relations.yaml"]
    edge_kinds = set(rel_doc.get("edge_kinds") or [])
    for idx, relation in enumerate(rel_doc.get("relations", []) or [], start=1):
        result.relation_count += 1
        kind = relation.get("kind")
        source = relation.get("from")
        target = relation.get("to")
        if kind not in edge_kinds:
            result.errors.append(f"relations[{idx}]: unknown edge kind `{kind}`")
        if source not in registry:
            result.errors.append(f"relations[{idx}]: unknown source `{source}`")
        if target not in registry:
            result.errors.append(f"relations[{idx}]: unknown target `{target}`")


def _validate_migration(
    docs: dict[str, Any],
    registry: dict[str, dict[str, Any]],
    result: OntologyValidationResult,
) -> None:
    migration = docs["migration.yaml"]
    for section in ["legacy_predicate_aliases", "legacy_term_aliases", "legacy_operator_aliases"]:
        for legacy_name, entry in (migration.get(section) or {}).items():
            result.migration_count += 1
            target = entry.get("target")
            if target not in registry:
                result.errors.append(f"{section}.{legacy_name}: unknown target `{target}`")

    for legacy_id, entry in (migration.get("legacy_node_id_map") or {}).items():
        result.migration_count += 1
        target = entry.get("ontology")
        if target not in registry:
            result.errors.append(f"legacy_node_id_map.{legacy_id}: unknown ontology target `{target}`")

    for idx, rewrite in enumerate(migration.get("tex_rewrites", []) or [], start=1):
        result.migration_count += 1
        new_kind = rewrite.get("new_kind")
        if new_kind and new_kind not in {"term", "predicate", "operator"}:
            result.errors.append(f"tex_rewrites[{idx}]: invalid new_kind `{new_kind}`")


def _validate_operator_duals(
    docs: dict[str, Any],
    registry: dict[str, dict[str, Any]],
    result: OntologyValidationResult,
) -> None:
    for op in docs["operators.yaml"].get("operators", []) or []:
        dual = op.get("dual")
        if dual and dual not in registry:
            result.errors.append(f"{op.get('id')}: unknown dual `{dual}`")
        elif dual and not str(dual).startswith("op:"):
            result.errors.append(f"{op.get('id')}: dual must be an operator id")


def _validate_derived_predicates(
    docs: dict[str, Any],
    registry: dict[str, dict[str, Any]],
    result: OntologyValidationResult,
) -> None:
    for pred in docs["predicates.yaml"].get("predicates", []) or []:
        derived_from = pred.get("derived_from")
        if derived_from and derived_from not in registry:
            result.errors.append(f"{pred.get('id')}: unknown derived_from `{derived_from}`")
        elif derived_from and not str(derived_from).startswith("op:"):
            result.errors.append(f"{pred.get('id')}: derived_from must be an operator id")
