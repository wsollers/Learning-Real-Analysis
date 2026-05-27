"""
generators/statement.py
Generates a single formal statement block. Returns raw LaTeX.
"""

import yaml
import json
import re

from auditor import client, loader
from auditor import config


def generate_statement(
    artifact_type: str,
    content_description: str,
    chapter_subject: str,
    chapter_registry: list[dict] | None = None,
    label: str | None = None,
) -> str:
    base_prompt = loader.prompt("generate_statement")
    registry = loader.block_registry()
    matrix_row = loader.matrix_row(artifact_type)

    registry_yaml = yaml.dump(
        {"blocks": list(registry.values())},
        default_flow_style=False,
        allow_unicode=True,
    )
    matrix_yaml = yaml.dump(
        matrix_row,
        default_flow_style=False,
        allow_unicode=True,
    )
    chapter_registry_yaml = yaml.dump(
        chapter_registry or [],
        default_flow_style=False,
        allow_unicode=True,
    )
    formal_label_index = _load_formal_label_index()
    candidate_label_context = _candidate_label_context(
        formal_label_index,
        artifact_type=artifact_type,
        content_description=content_description,
        chapter_subject=chapter_subject,
        requested_label=label,
    )

    system = client.assemble_generate_system_prompt(
        base_prompt,
        block_registry_yaml=registry_yaml,
        artifact_matrix_row=matrix_yaml,
        artifact_type=artifact_type,
        predicates_yaml=loader.canonical_source("predicates"),
        notation_yaml=loader.canonical_source("notation"),
        relations_yaml=loader.canonical_source("relations"),
        chapter_registry=chapter_registry_yaml,
        formal_label_index=formal_label_index,
        candidate_label_context=candidate_label_context,
    )

    user = (
        f"## Chapter Subject\n\n{chapter_subject}\n\n"
        f"## Content Description\n\n{content_description}"
    )

    return client.call(system, user, expect_json=False)


def _load_formal_label_index() -> str:
    index_dir = config.REPORTS_DIR / "indexes"
    candidates = sorted(
        index_dir.glob("*-formal-label-index.json"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    if not candidates:
        return ""
    return candidates[0].read_text(encoding="utf-8")


def _candidate_label_context(
    index_text: str,
    *,
    artifact_type: str,
    content_description: str,
    chapter_subject: str,
    requested_label: str | None = None,
) -> str:
    lines: list[str] = []
    if requested_label:
        lines.append(
            "The caller supplied this canonical label. Use it exactly for the "
            f"generated environment label: `{requested_label}`."
        )

    if not index_text:
        return "\n\n".join(lines)

    try:
        data = json.loads(index_text)
    except json.JSONDecodeError:
        return "\n\n".join(lines)

    description_tokens = _search_tokens(content_description)
    primary_slug = _primary_slug(content_description)
    chapter_key = chapter_subject.strip().lower()
    scored: list[tuple[int, dict]] = []

    for item in data.get("items", []):
        if item.get("artifact_type") != artifact_type:
            continue
        if chapter_key and str(item.get("chapter", "")).lower() != chapter_key:
            continue

        haystack = " ".join(
            str(item.get(key, ""))
            for key in ("label", "title", "section", "subsection", "file")
        ).lower()
        score = sum(1 for token in description_tokens if token in haystack)
        label_suffix = str(item.get("label", "")).split(":", 1)[-1].lower()
        title_slug = _slug(str(item.get("title", "")))
        if primary_slug and label_suffix == primary_slug:
            score += 50
        elif primary_slug and primary_slug in label_suffix:
            score += 20
        if primary_slug and title_slug == primary_slug:
            score += 50
        elif primary_slug and primary_slug in title_slug:
            score += 20
        if score > 0:
            scored.append((score, item))

    scored.sort(key=lambda pair: (-pair[0], pair[1].get("label", "")))
    candidates = [item for _, item in scored[:8]]
    if candidates:
        lines.append(
            "Possible existing labels for this same item are listed below. "
            "If one matches the requested mathematical item, reuse its label "
            "exactly instead of inventing a synonym label."
        )
        for item in candidates:
            lines.append(
                "- "
                f"`{item.get('label')}` | title: {item.get('title')} | "
                f"file: {item.get('file')}:{item.get('line')}"
            )

    return "\n".join(lines)


def _search_tokens(text: str) -> set[str]:
    stop = {
        "and", "are", "be", "chapter", "define", "definition", "element",
        "every", "exactly", "for", "foundational", "generate", "if", "in",
        "is", "let", "local", "of", "or", "set", "such", "subset", "that",
        "the", "then", "to", "use", "when", "with",
    }
    return {
        token
        for token in re.findall(r"[a-zA-Z][a-zA-Z0-9-]*", text.lower())
        if len(token) >= 4 and token not in stop
    }


def _primary_slug(text: str) -> str:
    first_sentence = re.split(r"[.:\n]", text.strip(), maxsplit=1)[0]
    first_sentence = re.split(r"\bLet\b", first_sentence, flags=re.IGNORECASE)[0]
    first_sentence = re.sub(r"\b(of|for|on|in|with|from)\b.*$", "", first_sentence, flags=re.IGNORECASE)
    return _slug(first_sentence)


def _slug(text: str) -> str:
    words = re.findall(r"[a-zA-Z0-9]+", text.lower())
    stop = {"a", "an", "and", "the"}
    return "-".join(word for word in words if word not in stop)
