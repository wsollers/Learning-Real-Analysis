#!/usr/bin/env python3
"""
apply_audit_fixes.py  —  run from the repo root.

Applies:
  1. blue!4/blue!30 -> defbox/defborder (definitions) or thmbox/thmborder (theorems)
     on differentiation note files.
  2. tcolorbox wrappers for bare theorem-like environments in both chapters.
"""

from __future__ import annotations
import re
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent


def fix_colors(text: str) -> str:
    def replacement(m: re.Match) -> str:
        rest = text[m.end():]
        env_m = re.match(r'\s*\\begin\{(\w+)\}', rest)
        if not env_m:
            return m.group(0)
        env = env_m.group(1)
        if env == 'definition':
            return r'\begin{tcolorbox}[colback=defbox, colframe=defborder]'
        elif env in ('theorem', 'lemma', 'proposition', 'corollary'):
            return r'\begin{tcolorbox}[colback=thmbox, colframe=thmborder]'
        elif env == 'axiom':
            return r'\begin{tcolorbox}[colback=axiombox, colframe=axiomborder]'
        return m.group(0)
    return re.sub(r'\\begin\{tcolorbox\}\[colback=blue!4[^\]]*\]', replacement, text)


def wrap_env(text: str, label: str) -> str:
    label_pos = text.find(f'\\label{{{label}}}')
    if label_pos == -1:
        print(f"  WARNING: label {label} not found")
        return text
    before = text[:label_pos]
    begin_m = None
    for m in re.finditer(r'\\begin\{(theorem|proposition|lemma|corollary)\}(?:\[[^\]]*\])?', before):
        begin_m = m
    if not begin_m:
        print(f"  WARNING: no env begin before {label}")
        return text
    preceding = text[:begin_m.start()]
    last_begins = list(re.finditer(r'\\begin\{(\w+)\}', preceding))
    if last_begins and last_begins[-1].group(1) == 'tcolorbox':
        return text  # already wrapped
    env_name = begin_m.group(1)
    depth = 0
    end_pos = begin_m.start()
    for em in re.finditer(r'\\(?:begin|end)\{' + env_name + r'\}', text[begin_m.start():]):
        if em.group(0).startswith('\\begin'):
            depth += 1
        else:
            depth -= 1
            if depth == 0:
                end_pos = begin_m.start() + em.end()
                break
    block = text[begin_m.start():end_pos]
    wrapped = r'\begin{tcolorbox}[colback=thmbox, colframe=thmborder]' + '\n' + block + '\n' + r'\end{tcolorbox}'
    return text[:begin_m.start()] + wrapped + text[end_pos:]


DIFF_COLOR_FILES = [
    REPO / 'volume-iii/analysis/differentiation/notes/derivative-definition/notes-derivative-definition.tex',
    REPO / 'volume-iii/analysis/differentiation/notes/algebra-of-derivatives/notes-algebra-of-derivatives.tex',
    REPO / 'volume-iii/analysis/differentiation/notes/derivative-geometry/notes-derivative-geometry.tex',
    REPO / 'volume-iii/analysis/differentiation/notes/derivative-geometry/notes-understanding-derivatives.tex',
    REPO / 'volume-iii/analysis/differentiation/notes/chain-rule/notes-caratheodory.tex',
    REPO / 'volume-iii/analysis/differentiation/notes/one-sided-derivatives/notes-one-sided-derivatives.tex',
]

BARE_ENVS: dict[Path, list[str]] = {
    REPO / 'volume-iii/analysis/continuity/notes/point-continuity/notes-discontinuity.tex': [
        'lem:equivalent-discontinuity-at-a-point',
    ],
    REPO / 'volume-iii/analysis/continuity/notes/uniform-continuity/notes-uniform-continuity.tex': [
        'prop:sequential-uniform-continuity',
        'prop:uniform-continuity-cauchy',
        'prop:lipschitz-implies-uc',
    ],
    REPO / 'volume-iii/analysis/continuity/notes/global-theorems/notes-ivt.tex': [
        'thm:location-of-roots',
        'thm:preservation-of-intervals',
        'thm:darboux-property',
    ],
    REPO / 'volume-iii/analysis/continuity/notes/monotone-functions/notes-monotone-functions.tex': [
        'thm:monotone-one-sided-limits',
        'cor:monotone-continuity-criterion',
        'prop:monotone-discontinuities-first-kind',
        'cor:jump-intervals-for-monotone-discontinuities',
        'cor:monotone-continuous-on-dense-set',
        'prop:continuous-injective-iff-strictly-monotone',
    ],
    REPO / 'volume-iii/analysis/differentiation/notes/mean-value-theorem/notes-mean-value-theorem.tex': [
        'thm:rolles-theorem',
        'thm:mean-value-theorem',
    ],
}


def main() -> None:
    print("=== Applying audit fixes ===\n")

    for path in DIFF_COLOR_FILES:
        if not path.exists():
            print(f"SKIP (not found): {path.name}")
            continue
        text = path.read_text(encoding='utf-8')
        before = text.count('blue!4')
        fixed = fix_colors(text)
        after = fixed.count('blue!4')
        if before != after:
            path.write_text(fixed, encoding='utf-8')
            print(f"color: {path.name}  blue!4 {before} → {after}")
        else:
            print(f"color: {path.name}  already clean")

    print()
    for path, labels in BARE_ENVS.items():
        if not path.exists():
            print(f"SKIP (not found): {path.name}")
            continue
        text = path.read_text(encoding='utf-8')
        for label in labels:
            text = wrap_env(text, label)
        path.write_text(text, encoding='utf-8')
        print(f"wrap: {path.name}  ({len(labels)} envs)")

    print("\nDone. Re-run  python theorem-explorer/run_extraction.py  to rebuild the explorer.")


if __name__ == '__main__':
    main()
