#!/usr/bin/env python3
"""
scaffold.py — Create the full canonical directory and stub file structure
for the Learning Real Analysis repository.

Run from the repo root:
    python scripts/scaffold.py

Creates all directories and stub index.tex / capstone files.
Safe to re-run: skips files that already exist.
"""

import os
from pathlib import Path

BASE = Path(__file__).parent.parent  # repo root

# =========================================================
# Chapter definitions
# =========================================================

CHAPTERS = {
    "volume-i": [
        "propositional-logic",
        "predicate-logic",
        "sets-relations-functions",
        "axiom-systems",
        "proof-techniques",
        "algebras-of-sets",
        "lambda-calculus",
        "type-theory",
        "model-theory",
    ],
    "volume-ii": [
        "natural-numbers",
        "integers",
        "rationals",
        "reals",
        "complex-numbers",
    ],
    "volume-iii/analysis": [
        "bounding",
        "sequences",
        "continuity",
        "differentiation",
        "riemann-integration",
        "series",
        "function-sequences",
    ],
    "volume-iii/algebra": [
        "abstract-algebra",
    ],
    "volume-iii/metric-spaces": [
        "metric-spaces",
    ],
    "volume-iv": [
        "geometric-modeling",
        "computational-linear-algebra",
        "ordinary-differential-equations",
    ],
}

# =========================================================
# Stub templates
# =========================================================

CHAPTER_INDEX = """\
% =========================================================
% Chapter: {title}
% Status: STUB
% =========================================================

% ---- Breadcrumb (fill before use) ----
% \\begin{{tcolorbox}}[colback=gray!6, colframe=gray!40, arc=2pt,
%   left=8pt, right=8pt, top=6pt, bottom=6pt,
%   title={{\\small\\textbf{{Where You Are in the Journey}}}},
%   fonttitle=\\small\\bfseries]
% \\begin{{center}}\\small
%   [Prior] $\\;\\to\\;$ \\textbf{{{title}}} $\\;\\to\\;$ [Next]
% \\end{{center}}
% \\end{{tcolorbox}}

\\input{{{vol_ch}/notes/index}}
\\input{{{vol_ch}/proofs/index}}
"""

NOTES_INDEX = """\
% =========================================================
% Notes index: {chapter}
% =========================================================
% \\input{{.../<section>/notes-<section>}}
"""

PROOFS_MAIN = """\
% =========================================================
% Proofs orchestrator: {chapter}
% =========================================================
\\input{{{vol_ch}/proofs/exercises/index}}
\\input{{{vol_ch}/proofs/notes/index}}
"""

PROOFS_NOTES_INDEX = """\
% =========================================================
% Proof files: {chapter}
% =========================================================
% \\input{{.../<proof-id>}}
"""

PROOFS_EXERCISES_INDEX = """\
% =========================================================
% Exercise proofs: {chapter}
% =========================================================
% \\input{{.../capstone-{chapter}}}
"""

CAPSTONE = """\
% Capstone exercise: {title}
% See DESIGN.md §1.4
% TODO
"""

CHAPTER_YAML = """\
# =========================================================
# chapter.yaml — {title}
# {vol_ch}/
#
# Machine-readable card data for all labelled environments.
# Replaces Flash macros in .tex files.
# =========================================================

chapter: {chapter}
volume: {volume}
path: {vol_ch}

entries:
  # Add entries here as notes are written.
  # Schema: see DESIGN.md §10-yaml
"""

# =========================================================
# Scaffold
# =========================================================

def make(path: Path, content: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        print(f"  exists   {path.relative_to(BASE)}")
    else:
        path.write_text(content, encoding="utf-8")
        print(f"  created  {path.relative_to(BASE)}")


def scaffold():
    created = 0
    skipped = 0

    for vol, chapters in CHAPTERS.items():
        volume_root = vol.split("/")[0]  # e.g. "volume-iii"

        for chapter in chapters:
            vol_ch = f"{vol}/{chapter}"
            title = chapter.replace("-", " ").title()

            files = {
                f"{vol_ch}/index.tex": CHAPTER_INDEX.format(
                    title=title, vol_ch=vol_ch),
                f"{vol_ch}/notes/index.tex": NOTES_INDEX.format(
                    chapter=chapter),
                f"{vol_ch}/proofs/index.tex": PROOFS_MAIN.format(
                    chapter=chapter, vol_ch=vol_ch),
                f"{vol_ch}/proofs/notes/index.tex": PROOFS_NOTES_INDEX.format(
                    chapter=chapter),
                f"{vol_ch}/proofs/exercises/index.tex": PROOFS_EXERCISES_INDEX.format(
                    chapter=chapter),
                f"{vol_ch}/proofs/exercises/capstone-{chapter}.tex": CAPSTONE.format(
                    title=title),
                f"{vol_ch}/chapter.yaml": CHAPTER_YAML.format(
                    title=title, chapter=chapter,
                    volume=volume_root, vol_ch=vol_ch),
            }

            print(f"\n{vol_ch}/")
            for rel, content in files.items():
                p = BASE / rel
                existed = p.exists()
                make(p, content)
                if existed:
                    skipped += 1
                else:
                    created += 1

    print(f"\n{'='*50}")
    print(f"Created: {created}  |  Skipped (already exist): {skipped}")


if __name__ == "__main__":
    scaffold()
