#!/usr/bin/env python3
"""
gen-folders.py — Generate canonical stub chapter structure.

Usage:
    python scripts/gen-folders.py volume-iv ordinary-differential-equations
    python scripts/gen-folders.py volume-iii analysis/new-chapter --sections sec1 sec2 sec3

Generates the full canonical folder tree with stub index.tex files,
stub notes-<section>.tex files, and a stub proofs-to-do-<chapter>.md.
The output of this script defines the canonical structure per DESIGN.md §1.3.
"""

import argparse
import os
from pathlib import Path

STUB_NOTES = """% =========================================================
% Notes: {section}
% Chapter: {chapter}
% =========================================================

% ---- Toolkit ----
% \begin{{tcolorbox}}[...]
% ...
% \end{{tcolorbox}}

% ---- Content ----
% \begin{{definition}}[Name]
% \label{{def:name}}
% ...
% \end{{definition}}

% \begin{{remark*}}[Fully quantified form]
% ...
% \end{{remark*}}

% \begin{{remark*}}[Flash]
% \FlashQ{{...}}
% \FlashA{{...}}
% \end{{remark*}}
"""

STUB_NOTES_INDEX = """% =========================================================
% Notes index: {chapter}
% =========================================================
% \input{{.../{section}/notes-{section}}}
"""

STUB_PROOFS_INDEX = """% =========================================================
% Proofs index: {chapter}
% =========================================================
% \input{{.../<proof-id>}}
"""

STUB_EXERCISES_INDEX = """% =========================================================
% Exercises index: {chapter}
% =========================================================
% \input{{.../capstone-{chapter}}}
"""

STUB_CHAPTER_INDEX = """% =========================================================
% Chapter: {chapter}
% =========================================================

% ---- Breadcrumb ----
% \begin{{tcolorbox}}[...]
% \begin{{center}}\small
%   [Prior] $\;\to\;$ \textbf{{{chapter_title}}} $\;\to\;$ [Next]
% \end{{center}}
% ...
% \end{{tcolorbox}}

\input{{{volume}/{chapter}/notes/index}}
\input{{{volume}/{chapter}/proofs/index}}
"""

STUB_PROOFS_MAIN_INDEX = """% =========================================================
% Proofs orchestrator: {chapter}
% =========================================================
\input{{{volume}/{chapter}/proofs/exercises/index}}
\input{{{volume}/{chapter}/proofs/notes/index}}
"""

STUB_PROOFS_TODO = """# proofs-to-do — {chapter_title}

Sorted by logical dependency tier.

---

## TIER 0 — Foundational

| Tier | Label | Name | Type | Proof | Flash | Difficulty |
|------|-------|------|------|-------|-------|------------|
| 0 | | | DEF | · | · | Easy |

---

## Summary

| Tier | Topic | Proof Done | Proof Total | Flash Done | Flash Total |
|------|-------|-----------|-------------|-----------|-------------|
| 0 | Foundational | 0 | 0 | 0 | 0 |
| **Total** | | **0** | **0** | **0** | **0** |
"""

def make(path, content):
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    if not Path(path).exists():
        Path(path).write_text(content)
        print(f"  created  {path}")
    else:
        print(f"  exists   {path}")

def gen(volume, chapter_path, sections):
    parts = chapter_path.split("/")
    chapter = parts[-1]
    chapter_title = chapter.replace("-", " ").title()
    vol_ch = f"{volume}/{chapter_path}"

    print(f"\nGenerating: {vol_ch}")
    print(f"  Sections: {sections or ['(none — add manually)']}")

    # Chapter index
    make(f"{vol_ch}/index.tex",
         STUB_CHAPTER_INDEX.format(volume=volume, chapter=chapter_path, chapter_title=chapter_title))

    # notes/index.tex
    make(f"{vol_ch}/notes/index.tex",
         STUB_NOTES_INDEX.format(chapter=chapter, section="<section>"))

    # Section stubs
    for sec in (sections or []):
        make(f"{vol_ch}/notes/{sec}/notes-{sec}.tex",
             STUB_NOTES.format(section=sec, chapter=chapter))

    # proofs/
    make(f"{vol_ch}/proofs/index.tex",
         STUB_PROOFS_MAIN_INDEX.format(volume=volume, chapter=chapter_path))
    make(f"{vol_ch}/proofs/notes/index.tex",
         STUB_PROOFS_INDEX.format(chapter=chapter))
    make(f"{vol_ch}/proofs/exercises/index.tex",
         STUB_EXERCISES_INDEX.format(chapter=chapter))
    make(f"{vol_ch}/proofs/exercises/capstone-{chapter}.tex",
         f"% Capstone exercise for {chapter_title}\n% See DESIGN.md §1.4\n")

    # proofs-to-do
    make(f"proofs-to-do-{chapter}.md",
         STUB_PROOFS_TODO.format(chapter_title=chapter_title))

    print(f"  Done.")

def main():
    p = argparse.ArgumentParser(description="Generate canonical chapter stub structure.")
    p.add_argument("volume", help="e.g. volume-iv")
    p.add_argument("chapter", help="e.g. ordinary-differential-equations or analysis/new-chapter")
    p.add_argument("--sections", nargs="+", default=[], metavar="SEC",
                   help="Section names to stub, e.g. --sections first-order systems")
    args = p.parse_args()
    gen(args.volume, args.chapter, args.sections)

if __name__ == "__main__":
    main()
