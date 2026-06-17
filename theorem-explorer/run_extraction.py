#!/usr/bin/env python3
"""
run_extraction.py
-----------------
One-command extraction runner for the Learning Real Analysis theorem explorer.

Usage (from the repo root):
    python theorem-explorer/run_extraction.py

This script:
  1. Runs extract_lra_chapter.py (pass 1) for each specified chapter.
  2. Runs seed_to_knowledge_json_v3_fixed6.py (pass 2) for each chapter.
  3. Merges both chapters' knowledge.json and graph-edges.json into a combined
     file at theorem-explorer/knowledge.json and theorem-explorer/graph-edges.json
     that the HTML explorer already expects.
  4. Prints a summary of node counts, edge counts, and any errors found.

Chapters extracted:
  Auto-discovered: every chapter (a directory that owns a notes/ subdir) under
  volumes i-iv. Edit VOLUMES below to widen (volume-v ... volume-viii) or narrow.
"""

from __future__ import annotations

import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
EXPLORER_DIR = REPO_ROOT / "theorem-explorer"

PASS1_SCRIPT = EXPLORER_DIR / "extract_lra_chapter.py"
PASS2_SCRIPT = EXPLORER_DIR / "seed_to_knowledge_json_v3_fixed6.py"

# Volumes whose chapters are extracted. Append "volume-v" ... "volume-viii" to widen.
VOLUMES = ["volume-i", "volume-ii", "volume-iii", "volume-iv"]


def discover_chapters(repo_root: Path) -> list[Path]:
    """Every chapter under VOLUMES: a directory that owns a notes/ subdirectory.

    Robust to nesting (volume-iii/analysis/continuity,
    volume-iii/discrete-calculus/foundations, ...). The proofs/notes tree is
    excluded — only a chapter's own notes/ marks a chapter root. main() still
    skips anything that has since lost its notes/ dir, so this stays safe.
    """
    chapters: set[Path] = set()
    for vol in VOLUMES:
        vol_dir = repo_root / vol
        if not vol_dir.exists():
            continue
        for notes_dir in vol_dir.rglob("notes"):
            if notes_dir.is_dir() and notes_dir.parent.name not in {"proofs", "notes"}:
                chapters.add(notes_dir.parent)
    return sorted(chapters)


CHAPTERS = discover_chapters(REPO_ROOT)

COMBINED_KNOWLEDGE = EXPLORER_DIR / "knowledge.json"
COMBINED_EDGES = EXPLORER_DIR / "graph-edges.json"
COMBINED_ERRORS = EXPLORER_DIR / "proof-errors.json"
COMBINED_EDGE_ERRORS = EXPLORER_DIR / "graph-edge-errors.json"


def run(cmd: list[str], label: str) -> subprocess.CompletedProcess:
    print(f"\n{'='*60}")
    print(f"  {label}")
    print(f"{'='*60}")
    result = subprocess.run(cmd, capture_output=False, text=True)
    if result.returncode != 0:
        print(f"[ERROR] {label} failed with code {result.returncode}")
    return result


def extract_chapter(chapter_root: Path) -> Path:
    """Run pass 1 (seed extractor) for one chapter. Returns .explorer dir."""
    out_dir = chapter_root / ".explorer"
    out_dir.mkdir(parents=True, exist_ok=True)
    run(
        [sys.executable, str(PASS1_SCRIPT), str(chapter_root),
         "--output-dir", str(out_dir)],
        f"Pass 1 — extract: {chapter_root.name}",
    )
    return out_dir


def compile_chapter(chapter_root: Path, explorer_dir: Path) -> Path:
    """Run pass 2 (knowledge compiler) for one chapter. Returns knowledge.json path."""
    run(
        [sys.executable, str(PASS2_SCRIPT), str(chapter_root),
         "--output-dir", str(explorer_dir)],
        f"Pass 2 — compile: {chapter_root.name}",
    )
    return explorer_dir / "knowledge.json"


def load_json(path: Path) -> dict | list | None:
    if not path.exists():
        print(f"[WARN] Not found: {path}")
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def merge_knowledge(chapter_roots: list[Path]) -> None:
    """Merge per-chapter knowledge.json files into a single combined file."""
    all_nodes: list[dict] = []
    all_edges: list[dict] = []
    all_errors: list[dict] = []
    all_edge_errors: list[dict] = []
    chapter_names: list[str] = []

    for chapter_root in chapter_roots:
        exp = chapter_root / ".explorer"
        k = load_json(exp / "knowledge.json")
        e = load_json(exp / "graph-edges.json")
        pe = load_json(exp / "proof-errors.json")
        ge = load_json(exp / "graph-edge-errors.json")

        if k and "nodes" in k:
            all_nodes.extend(k["nodes"])
            chapter_names.append(k.get("metadata", {}).get("chapter", chapter_root.name))
        if e and isinstance(e, list):
            all_edges.extend(e)
        if pe and "errors" in pe:
            all_errors.extend(pe["errors"])
        if ge and "errors" in ge:
            all_edge_errors.extend(ge["errors"])

    # Deduplicate edges
    seen_edges: set[tuple] = set()
    deduped_edges: list[dict] = []
    for edge in all_edges:
        key = (edge.get("from"), edge.get("to"), edge.get("kind"))
        if key not in seen_edges:
            seen_edges.add(key)
            deduped_edges.append(edge)

    combined = {
        "metadata": {
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "chapters": chapter_names,
            "node_count": len(all_nodes),
            "edge_count": len(deduped_edges),
            "error_count": len(all_errors) + len(all_edge_errors),
            "schema_version": "0.4",
            "script": "run_extraction.py",
        },
        "nodes": all_nodes,
        "edges": deduped_edges,
    }

    COMBINED_KNOWLEDGE.write_text(
        json.dumps(combined, indent=2, ensure_ascii=False), encoding="utf-8"
    )
    COMBINED_EDGES.write_text(
        json.dumps(deduped_edges, indent=2, ensure_ascii=False), encoding="utf-8"
    )
    COMBINED_ERRORS.write_text(
        json.dumps({
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "chapters": chapter_names,
            "error_count": len(all_errors),
            "errors": all_errors,
        }, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    COMBINED_EDGE_ERRORS.write_text(
        json.dumps({
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "chapters": chapter_names,
            "error_count": len(all_edge_errors),
            "errors": all_edge_errors,
        }, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )


def print_summary() -> None:
    k = load_json(COMBINED_KNOWLEDGE)
    if not k:
        return
    meta = k.get("metadata", {})
    nodes = k.get("nodes", [])
    edges = k.get("edges", [])

    print(f"\n{'='*60}")
    print("  EXTRACTION COMPLETE — SUMMARY")
    print(f"{'='*60}")
    print(f"  Chapters  : {', '.join(meta.get('chapters', []))}")
    print(f"  Nodes     : {meta.get('node_count', len(nodes))}")
    print(f"  Edges     : {meta.get('edge_count', len(edges))}")
    print(f"  Generated : {meta.get('generated_at', '')}")
    print()

    kind_counts: dict[str, int] = {}
    for n in nodes:
        kind_counts[n.get("kind", "?")] = kind_counts.get(n.get("kind", "?"), 0) + 1
    print("  Node kinds:")
    for kind, count in sorted(kind_counts.items(), key=lambda x: -x[1]):
        print(f"    {kind:<20} {count}")
    print()

    edge_counts: dict[str, int] = {}
    for e in edges:
        edge_counts[e.get("kind", "?")] = edge_counts.get(e.get("kind", "?"), 0) + 1
    print("  Edge kinds:")
    for kind, count in sorted(edge_counts.items(), key=lambda x: -x[1]):
        print(f"    {kind:<20} {count}")
    print()

    errors = load_json(COMBINED_ERRORS)
    edge_errors = load_json(COMBINED_EDGE_ERRORS)
    err_count = (errors or {}).get("error_count", 0)
    edge_err_count = (edge_errors or {}).get("error_count", 0)
    print(f"  Proof errors      : {err_count}")
    print(f"  Graph edge errors : {edge_err_count}")

    print(f"\n  Explorer output : {COMBINED_KNOWLEDGE}")
    print()


def main() -> None:
    if not PASS1_SCRIPT.exists():
        sys.exit(f"[ERROR] Pass 1 script not found: {PASS1_SCRIPT}")
    if not PASS2_SCRIPT.exists():
        sys.exit(f"[ERROR] Pass 2 script not found: {PASS2_SCRIPT}")

    for chapter in CHAPTERS:
        if not chapter.exists():
            print(f"[WARN] Chapter not found, skipping: {chapter}")
            continue
        if not (chapter / "notes").exists():
            print(f"[WARN] No notes/ dir in {chapter}, skipping.")
            continue
        explorer_dir = extract_chapter(chapter)
        compile_chapter(chapter, explorer_dir)

    print("\n[INFO] Merging all chapters into combined knowledge.json ...")
    merge_knowledge(CHAPTERS)
    print_summary()


if __name__ == "__main__":
    main()
