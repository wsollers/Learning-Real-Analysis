#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

DEFAULT_SCHEMA_VERSION = "0.3"
RECOGNIZED_EDGE_KINDS = {"depends_on", "equivalent_to", "implies", "prereq", "used_by"}

LABEL_RE = re.compile(r"\\label\{[^{}]+\}")
HYPERREF_LINE_RE = re.compile(r"^\s*\\hyperref\[[^\]]+\]\{.*?\}\s*$", re.MULTILINE | re.DOTALL)
BEGIN_ENV_RE = re.compile(r"\\begin\{([A-Za-z*]+)\}(?:\[[^\]]*\])?")
END_ENV_RE = re.compile(r"\\end\{([A-Za-z*]+)\}")
ITEM_RE = re.compile(r"\\item(?:\[[^\]]*\])?\s*")
HYPERREF_TARGET_RE = re.compile(r"\\hyperref\[([^\]]+)\]")
COMMAND_SPACES_RE = re.compile(r"\\(?:smallskip|medskip|bigskip|noindent|phantomsection|newpage)\b")
TITLE_BRACKET_RE = re.compile(r"^\s*\[[^\]]*\]\s*")
COMMENT_RE = re.compile(r"(?<!\\)%.*")
MATH_DISPLAY_WRAP_RE = re.compile(r"^\s*\\\[(.*)\\\]\s*$", re.DOTALL)
INLINE_MATH_DOLLAR_WRAP_RE = re.compile(r"^\s*\$(.*)\$\s*$", re.DOTALL)
MULTISPACE_RE = re.compile(r"[ \t]+")
MULTIBLANK_RE = re.compile(r"\n{3,}")

FIELD_DEFAULTS: dict[str, Any] = {
    "id": "",
    "kind": "",
    "name": "",
    "deck": "",
    "source": "",
    "proof_source": "",
    "question": "",
    "statement_display": "",
    "statement_tex": "",
    "hint": "",
    "proposition": "",
    "formal": "",
    "fully_quantified_form": "",
    "negation_display": "",
    "negation_formal": "",
    "contrapositive": "",
    "depends_on_ids": [],
    "used_by_ids": [],
    "prereq_ids": [],
    "equivalent_to_ids": [],
    "implies_ids": [],
    "aliases": [],
    "tags": [],
    "symbol": "",
    "category": "",
    "proof_sketch": "",
    "proof_idea": "",
    "proof_type": "",
    "method": "",
    "uses_text": "",
    "source_text": "",
    "section": "",
    "has_proof_file": False,
    "ignored": False,
    "is_theorem_like": True,
    "depends_on_titles": [],
    "used_by_titles": [],
    "prereq_titles": [],
    "equivalent_to_titles": [],
    "implies_titles": [],
}


def decode_b64(s: str | None) -> str:
    if not s:
        return ""
    return base64.b64decode(s).decode("utf-8", errors="replace")


def humanize_slug(slug: str) -> str:
    if not slug:
        return ""
    return slug.replace("-", " ").replace("_", " ").title()


def strip_comments(text: str) -> str:
    lines = []
    for line in text.splitlines():
        lines.append(COMMENT_RE.sub("", line))
    return "\n".join(lines)


def cleanup_statement_tex(text: str) -> str:
    t = decode_b64(text) if not isinstance(text, str) else text
    t = strip_comments(t)
    t = TITLE_BRACKET_RE.sub("", t, count=1)
    t = LABEL_RE.sub("", t)
    t = HYPERREF_LINE_RE.sub("", t)
    t = COMMAND_SPACES_RE.sub(" ", t)
    t = MULTISPACE_RE.sub(" ", t)
    t = MULTIBLANK_RE.sub("\n\n", t)
    return t.strip()


def extract_body_from_env(raw_env_tex: str) -> str:
    text = strip_comments(raw_env_tex).strip()
    m = BEGIN_ENV_RE.search(text)
    if not m:
        return text
    start = m.end()
    body = text[start:]
    end_matches = list(END_ENV_RE.finditer(body))
    if end_matches:
        body = body[: end_matches[-1].start()]
    return body.strip()


def clean_remark_content(raw_b64: str) -> str:
    raw = decode_b64(raw_b64)
    body = extract_body_from_env(raw)
    body = strip_comments(body)
    body = COMMAND_SPACES_RE.sub(" ", body)
    body = MULTIBLANK_RE.sub("\n\n", body)
    return body.strip()


def latex_to_basic_html(text: str) -> str:
    t = text.strip()
    def replace_list_env(src: str, env_name: str, tag: str) -> str:
        pattern = re.compile(rf"\\begin\{{{env_name}\}}(.*?)\\end\{{{env_name}\}}", re.DOTALL)
        while True:
            m = pattern.search(src)
            if not m:
                break
            content = m.group(1)
            parts = [p.strip() for p in ITEM_RE.split(content) if p.strip()]
            items = "".join(f"<li>{p}</li>" for p in parts)
            repl = f"<{tag} style=\"margin:.6em 0 .6em 1.5em\">{items}</{tag}>"
            src = src[: m.start()] + repl + src[m.end() :]
        return src
    t = replace_list_env(t, "enumerate", "ol")
    t = replace_list_env(t, "itemize", "ul")
    t = HYPERREF_LINE_RE.sub("", t)
    t = LABEL_RE.sub("", t)
    t = COMMAND_SPACES_RE.sub(" ", t)
    t = MULTIBLANK_RE.sub("\n\n", t)
    return t.strip()


def choose_shortest_nonempty(items: list[str]) -> str:
    cleaned = [i.strip() for i in items if i and i.strip()]
    if not cleaned:
        return ""
    return min(cleaned, key=len)


def strip_leading_proof_heading(text: str) -> str:
    t = text.strip()
    t = re.sub(r'^\\textbf\{[^{}]*?Proof\.?\}\.?(?:\\\\)?\s*', '', t, count=1)
    t = re.sub(r'^Detailed\s+Learning\s+Proof\.?\s*~?\\\\\s*', '', t, flags=re.IGNORECASE)
    t = re.sub(r'^Professional\s+Standard\s+Proof\.?\s*~?\\\\\s*', '', t, flags=re.IGNORECASE)
    return t.strip()


def extract_detailed_learning_steps(text: str) -> list[str]:
    body = strip_leading_proof_heading(text)
    pattern = re.compile(
        r'\\textbf\{Step\s+([^{}]+?)\.\}\s*(.*?)(?=(?:\\textbf\{Step\s+[^{}]+?\.\})|\Z)',
        re.DOTALL | re.IGNORECASE,
    )
    steps: list[str] = []
    for m in pattern.finditer(body):
        step_no = m.group(1).strip()
        step_body = m.group(2).strip()
        if step_body:
            steps.append(f"Step {step_no}. {step_body}")
    return steps


def extract_dependencies_from_blocks(blocks: list[dict[str, Any]]) -> list[str]:
    ids: list[str] = []
    for blk in blocks:
        if blk.get("env_name") == "remark*" and (blk.get("title") or "").strip().lower() == "dependencies":
            raw = clean_remark_content(blk.get("raw_latex_b64", ""))
            ids.extend(HYPERREF_TARGET_RE.findall(raw))
    seen = set()
    out = []
    for x in ids:
        if x not in seen:
            seen.add(x)
            out.append(x)
    return out


def summarize_proof_blocks(blocks: list[dict[str, Any]], node_id: str, proof_source_path: str) -> tuple[str, str, str, str, dict[str, Any] | None]:
    proof_texts: list[str] = []
    proof_structure = ""
    method = ""
    detailed_steps: list[str] = []
    found_detailed_learning_proof = False

    for blk in blocks:
        env = blk.get("env_name", "")
        title = (blk.get("title") or "").strip().lower()
        if env == "proof":
            raw = decode_b64(blk.get("raw_latex_b64", ""))
            body = extract_body_from_env(raw)
            body = strip_comments(body).strip()
            if body:
                proof_texts.append(body)
                if "detailed learning proof" in body.lower():
                    found_detailed_learning_proof = True
                    detailed_steps = extract_detailed_learning_steps(body)
        elif env == "remark*" and title == "proof structure":
            proof_structure = clean_remark_content(blk.get("raw_latex_b64", ""))
        elif env == "remark*" and title == "method":
            method = clean_remark_content(blk.get("raw_latex_b64", ""))

    if detailed_steps:
        proof_sketch = "\n".join(detailed_steps)
    else:
        proof_sketch = choose_shortest_nonempty(proof_texts)

    proof_type = ""
    if proof_texts:
        joined = "\n\n".join(proof_texts[:2]).lower()
        if "induct" in joined:
            proof_type = "induction"
        elif "contradiction" in joined:
            proof_type = "contradiction"
        elif "contrapositive" in joined:
            proof_type = "contrapositive"
        else:
            proof_type = "direct"

    error = None
    if proof_source_path and proof_texts and not found_detailed_learning_proof:
        error = {
            "type": "missing_detailed_learning_proof",
            "node_id": node_id,
            "proof_source": proof_source_path,
            "message": "Proof file has proof content but no Detailed Learning Proof block.",
        }

    return proof_sketch, proof_structure, proof_type, method, error


def classify_remark_fields(remark_blocks: list[dict[str, Any]]) -> dict[str, str]:
    out = {
        "formal": "",
        "fully_quantified_form": "",
        "negation_display": "",
        "negation_formal": "",
        "contrapositive": "",
        "proposition": "",
        "source_text": "",
    }
    for blk in remark_blocks:
        title = (blk.get("title") or "").strip().lower()
        content = clean_remark_content(blk.get("raw_latex_b64", ""))
        if not content:
            continue
        if title == "standard quantified statement":
            out["fully_quantified_form"] = content
            if not out["formal"]:
                out["formal"] = content
        elif title in {"definition predicate reading", "predicate statement", "predicate reading", "definition predicate language"}:
            out["proposition"] = content
            if not out["formal"]:
                out["formal"] = content
        elif title in {"negated quantified statement", "negation predicate reading"}:
            out["negation_display"] = content
            out["negation_formal"] = content
        elif title == "contrapositive quantified statement":
            out["contrapositive"] = content
        elif title == "interpretation":
            out["source_text"] = content
    return out


def build_node(seed_node: dict[str, Any], chapter_name: str) -> tuple[dict[str, Any], dict[str, Any] | None]:
    node = json.loads(json.dumps(FIELD_DEFAULTS))
    node["id"] = seed_node["id"]
    node["kind"] = seed_node.get("kind", "")
    node["name"] = seed_node.get("title") or seed_node.get("label") or seed_node["id"]
    node["deck"] = humanize_slug(seed_node.get("section_slug") or seed_node.get("note_dir") or chapter_name)
    node["section"] = seed_node.get("section_slug") or ""
    node["source"] = seed_node.get("source_path") or ""
    node["proof_source"] = seed_node.get("proof_source_path") or ""
    node["question"] = f"State the {node['kind'].lower()}: {node['name']}."

    statement_tex = cleanup_statement_tex(decode_b64(seed_node.get("body_latex_b64", "")))
    node["statement_tex"] = statement_tex
    node["statement_display"] = latex_to_basic_html(statement_tex)
    node["has_proof_file"] = bool(seed_node.get("proof_source_path"))
    node["is_theorem_like"] = True

    remark_fields = classify_remark_fields(seed_node.get("remark_blocks", []))
    node.update(remark_fields)

    proof_sketch, proof_idea, proof_type, method, error = summarize_proof_blocks(
        seed_node.get("proof_file_blocks", []),
        node_id=node["id"],
        proof_source_path=node["proof_source"],
    )
    node["proof_sketch"] = proof_sketch
    node["proof_idea"] = proof_idea
    node["proof_type"] = proof_type
    node["method"] = method

    deps = extract_dependencies_from_blocks(seed_node.get("proof_file_blocks", []))
    node["uses_text"] = ", ".join(deps)

    node["raw_latex_b64"] = seed_node.get("raw_latex_b64", "")
    node["body_latex_b64"] = seed_node.get("body_latex_b64", "")
    node["title_latex_b64"] = seed_node.get("title_latex_b64", "")
    node["proof_raw_latex_b64"] = seed_node.get("proof_raw_latex_b64", "")
    node["remark_blocks"] = seed_node.get("remark_blocks", [])
    node["proof_file_blocks"] = seed_node.get("proof_file_blocks", [])
    node["seed_labels"] = seed_node.get("labels", [])
    node["seed_proof_refs"] = seed_node.get("proof_refs", [])
    node["seed_theorem_refs"] = seed_node.get("theorem_refs", [])
    node["seed_proof_return_targets"] = seed_node.get("proof_return_targets", [])
    node["env_name"] = seed_node.get("env_name", "")
    node["text_preview"] = seed_node.get("text_preview", "")
    return node, error


def load_seed(seed_dir: Path) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    knowledge_seed_path = seed_dir / "knowledge-seed.json"
    edge_seed_path = seed_dir / "graph-edges-seed.json"
    missing = [str(p) for p in [knowledge_seed_path, edge_seed_path] if not p.exists()]
    if missing:
        raise FileNotFoundError(
            "Missing seed file(s): " + ", ".join(missing)
            + ". Run the first-pass extractor first, or point --output-dir to the folder containing the seed JSON files."
        )
    knowledge_seed = json.loads(knowledge_seed_path.read_text(encoding="utf-8"))
    edge_seed = json.loads(edge_seed_path.read_text(encoding="utf-8"))
    return knowledge_seed, edge_seed


def build_edges(nodes: list[dict[str, Any]], edge_seed: list[dict[str, Any]]) -> list[dict[str, str]]:
    node_ids = {n["id"] for n in nodes}
    edges: list[dict[str, str]] = []
    seen: set[tuple[str, str, str]] = set()

    def add(frm: str, to: str, kind: str) -> None:
        if frm not in node_ids or to not in node_ids:
            return
        key = (frm, to, kind)
        if key in seen:
            return
        seen.add(key)
        edges.append({"from": frm, "to": to, "kind": kind})

    for n in nodes:
        if n.get("uses_text"):
            for dep in [x.strip() for x in n["uses_text"].split(",") if x.strip()]:
                add(n["id"], dep, "depends_on")

    for e in edge_seed:
        kind = e.get("kind", "")
        if kind in RECOGNIZED_EDGE_KINDS:
            add(e.get("from", ""), e.get("to", ""), kind)

    for e in list(edges):
        if e["kind"] == "depends_on":
            add(e["to"], e["from"], "used_by")
        elif e["kind"] == "used_by":
            add(e["to"], e["from"], "depends_on")
        elif e["kind"] == "equivalent_to":
            add(e["to"], e["from"], "equivalent_to")
    return edges


def attach_edge_lists(nodes: list[dict[str, Any]], edges: list[dict[str, str]]) -> None:
    by_id = {n["id"]: n for n in nodes}
    for e in edges:
        frm, to, kind = e["from"], e["to"], e["kind"]
        if kind == "depends_on":
            by_id[frm]["depends_on_ids"].append(to)
        elif kind == "used_by":
            by_id[frm]["used_by_ids"].append(to)
        elif kind == "prereq":
            by_id[frm]["prereq_ids"].append(to)
        elif kind == "equivalent_to":
            by_id[frm]["equivalent_to_ids"].append(to)
        elif kind == "implies":
            by_id[frm]["implies_ids"].append(to)

    def names(ids: list[str]) -> list[str]:
        return [by_id[x]["name"] for x in ids if x in by_id]

    for n in nodes:
        for field in ["depends_on_ids", "used_by_ids", "prereq_ids", "equivalent_to_ids", "implies_ids"]:
            seen = set()
            dedup = []
            for x in n[field]:
                if x not in seen:
                    seen.add(x)
                    dedup.append(x)
            n[field] = dedup
        n["depends_on_titles"] = names(n["depends_on_ids"])
        n["used_by_titles"] = names(n["used_by_ids"])
        n["prereq_titles"] = names(n["prereq_ids"])
        n["equivalent_to_titles"] = names(n["equivalent_to_ids"])
        n["implies_titles"] = names(n["implies_ids"])


def build_metadata(chapter_root: Path, chapter_name: str, nodes: list[dict[str, Any]], edges: list[dict[str, str]], errors: list[dict[str, Any]]) -> dict[str, Any]:
    return {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "roots": [str(chapter_root)],
        "node_count": len(nodes),
        "edge_count": len(edges),
        "error_count": len(errors),
        "edge_kinds": sorted({e["kind"] for e in edges}) or sorted(RECOGNIZED_EDGE_KINDS),
        "card_count": 0,
        "schema_version": DEFAULT_SCHEMA_VERSION,
        "script": "seed_to_knowledge_json_v3.py",
        "chapter": chapter_name,
    }


def convert_chapter(chapter_root: Path, output_dir: Path | None = None) -> tuple[Path, Path, Path]:
    out_dir = resolve_output_dir(chapter_root, output_dir)
    seed_dir = out_dir if output_dir is not None else (chapter_root / ".explorer")
    knowledge_seed, edge_seed = load_seed(seed_dir)
    chapter_name = knowledge_seed.get("chapter") or chapter_root.name
    nodes: list[dict[str, Any]] = []
    errors: list[dict[str, Any]] = []
    for seed in knowledge_seed.get("nodes", []):
        node, error = build_node(seed, chapter_name)
        nodes.append(node)
        if error:
            errors.append(error)
    edges = build_edges(nodes, edge_seed)
    attach_edge_lists(nodes, edges)
    payload = {
        "metadata": build_metadata(chapter_root, chapter_name, nodes, edges, errors),
        "nodes": nodes,
        "edges": edges,
    }
    out_dir = output_dir or (chapter_root / ".explorer")
    out_dir.mkdir(parents=True, exist_ok=True)
    knowledge_path = out_dir / "knowledge.json"
    graph_edges_path = out_dir / "graph-edges.json"
    errors_path = out_dir / "proof-errors.json"
    knowledge_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    graph_edges_path.write_text(json.dumps(edges, indent=2, ensure_ascii=False), encoding="utf-8")
    errors_payload = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "chapter": chapter_name,
        "error_count": len(errors),
        "errors": errors,
    }
    errors_path.write_text(json.dumps(errors_payload, indent=2, ensure_ascii=False), encoding="utf-8")
    return knowledge_path, graph_edges_path, errors_path


def main() -> None:
    parser = argparse.ArgumentParser(description="Second pass: convert LRA seed extraction into explorer-ready knowledge.json")
    parser.add_argument("chapter_root", type=Path, help="Path to a single chapter root containing .explorer/knowledge-seed.json")
    parser.add_argument("--output-dir", type=Path, default=None, help="Optional output directory. Default: <chapter>/.explorer")
    args = parser.parse_args()
    knowledge_path, edges_path, errors_path = convert_chapter(args.chapter_root.resolve(), args.output_dir.resolve() if args.output_dir else None)
    print(f"Wrote {knowledge_path}")
    print(f"Wrote {edges_path}")
    print(f"Wrote {errors_path}")


if __name__ == "__main__":
    main()
