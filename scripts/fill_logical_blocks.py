#!/usr/bin/env python3
"""
fill_logical_blocks.py
======================
Reads note files, calls the Anthropic API for each missing logical-block
gap, and writes the patched result back to disk.

Usage (from repo root):
    python scripts/fill_logical_blocks.py            # all 14 gaps
    python scripts/fill_logical_blocks.py --gaps 1 3 5   # specific gaps
    python scripts/fill_logical_blocks.py --dry-run      # print diffs, no write
    python scripts/fill_logical_blocks.py --gaps 1 --debug  # show raw API response

Each gap is processed independently. If one fails the rest continue.
A .bak file is written next to each modified file before the first edit.
"""

from __future__ import annotations
import argparse
import os
import re
import sys
import textwrap
from pathlib import Path

# ── dependency check ──────────────────────────────────────────────────────────
try:
    import anthropic
except ImportError:
    sys.exit("pip install anthropic --break-system-packages")

REPO   = Path(__file__).resolve().parent.parent
CLIENT = anthropic.Anthropic()   # reads ANTHROPIC_API_KEY from env
MODEL  = "claude-sonnet-4-20250514"

# ── file paths ────────────────────────────────────────────────────────────────
CONT = REPO / "volume-iii/analysis/continuity/notes"
DIFF = REPO / "volume-iii/analysis/differentiation/notes"

F_CONTINUITY   = CONT / "point-continuity/notes-continuity.tex"
F_DISCONTINUITY= CONT / "point-continuity/notes-discontinuity.tex"
F_UNIFORM      = CONT / "uniform-continuity/notes-uniform-continuity.tex"
F_DERIV_DEF    = DIFF / "derivative-definition/notes-derivative-definition.tex"
F_MVT          = DIFF / "mean-value-theorem/notes-mean-value-theorem.tex"


# ── system prompt (constitution summary) ─────────────────────────────────────
SYSTEM = textwrap.dedent(r"""
You are a LaTeX editor for a formal real-analysis repository.
You fill MISSING logical blocks in existing note files.
You do NOT regenerate existing content. You do NOT rewrite anything.
You insert ONLY the blocks listed in the task.

=== GOVERNING RULES ===

Block order after every def/thm/lem/prop/cor (DESIGN.md §10.2):
  1. remark*[Standard quantified statement]          — always
  2. remark*[Definition predicate reading]           — defs when predicate exists
     remark*[Predicate reading]                      — thm-like
  3. remark*[Negated quantified statement]           — when illuminating
  4. remark*[Negation predicate reading]             — if step 3 generated
  5. remark*[Failure modes]                          — when applicable
  6. remark*[Failure mode decomposition]             — if step 5 generated
  7. remark*[Contrapositive quantified statement]    — thm/lem/prop/cor, illuminating
  8. remark*[Contrapositive predicate reading]       — if step 7 generated
  9. remark*[Interpretation]
 10. remark*[Dependencies] or \NoLocalDependencies

NOTATION DISCIPLINE (Rule A):
  \operatorname{...} predicate names appear ONLY in blocks 2,4,6,8.
  They must NEVER appear in blocks 1,3,7 (standard/negated/contrapositive
  quantified statements) or in definition/theorem bodies.

NEGATION: push \neg inward using quantifier duals:
  \forall <-> \exists,  \land <-> \lor,
  (P => Q) negates as (P \land \neg Q).

CONTRAPOSITIVE: swap hypothesis and conclusion, negate both.

LONG DISPLAYS: multi-clause statements use aligned inside \[ \].

OUTPUT FORMAT:
  Raw LaTeX only. No prose commentary outside LaTeX. No code fences.
  Begin immediately with the first \begin{remark*} to insert.
  End after the last \end{remark*} to insert.
  Do NOT output blocks that already exist in the file.
  Do NOT output the surrounding context.
""").strip()


# ── gap specifications ────────────────────────────────────────────────────────
# Each gap has:
#   id          : int (1-14)
#   file        : Path
#   label       : str  (env label for logging)
#   missing     : list of block header strings to generate
#   anchor_label: unique \label{...} string that identifies the environment
#   insert_before: exact string before which we splice (after finding anchor)
#   task        : str  (mathematical instructions for the API)

GAPS: list[dict] = [

    # ── CONTINUITY ────────────────────────────────────────────────────────────

    {
        "id": 1,
        "file": F_CONTINUITY,
        "label": "def:continuous-at-point-seq",
        "missing": ["Failure modes", "Failure mode decomposition"],
        "anchor_label": r"\label{def:continuous-at-point-seq}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: def:continuous-at-point-seq
            MISSING BLOCKS: remark*[Failure modes]  AND  remark*[Failure mode decomposition]

            The definition states: f is continuous at c iff for every (x_n) in A,
            x_n -> c implies f(x_n) -> f(c).

            The negated quantified statement and negation predicate reading already
            exist in the file. Do not regenerate them.

            Generate exactly two remark* blocks:

            remark*[Failure modes]
              Prose. Continuity at c fails sequentially when some (x_n) in A with
              x_n -> c has f(x_n) not converging to f(c). Name two sub-branches:
              (i) f(x_n) converges but to a value other than f(c);
              (ii) f(x_n) does not converge at all (oscillates or diverges).

            remark*[Failure mode decomposition]
              Formal display. Use underbrace to label the two conjuncts of the
              existential witness. No predicate names here — standard notation only.
              Pattern:
                \neg(\text{cont. at }c) \Longleftrightarrow
                \exists (x_n)\subseteq A :
                  \underbrace{x_n \to c}_{\text{valid approach}}
                  \;\land\;
                  \underbrace{f(x_n) \not\to f(c)}_{\text{image sequence fails}}.
        """).strip(),
    },

    {
        "id": 2,
        "file": F_DISCONTINUITY,
        "label": "def:sequential-discontinuity-at-a-point",
        "missing": ["Negation predicate reading", "Failure modes", "Failure mode decomposition"],
        "anchor_label": r"\label{def:sequential-discontinuity-at-a-point}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: def:sequential-discontinuity-at-a-point
            MISSING: remark*[Negation predicate reading]
                     remark*[Failure modes]
                     remark*[Failure mode decomposition]

            The negated quantified statement already exists and reads:
              f is continuous at c <=> for all (x_n) in A: x_n->c => f(x_n)->f(c).
            Do NOT regenerate the negated quantified statement.

            Generate three remark* blocks in order:

            remark*[Negation predicate reading]
              \neg\operatorname{DiscontinuousAt}(f,c)
              \Longleftrightarrow \operatorname{ContinuousAt}(f,c)
              \Longleftrightarrow [full quantified form using standard notation].

            remark*[Failure modes]
              Prose. The characterization fails — meaning f is NOT
              sequentially discontinuous — when every sequence (x_n) in A
              converging to c has f(x_n) -> f(c). No sequential witness exists.

            remark*[Failure mode decomposition]
              Formal display of the negation with underbrace. Standard notation
              only (no \operatorname in this block).
              \neg\operatorname{DiscontinuousAt}(f,c)
              \Longleftrightarrow
              \underbrace{\forall (x_n)\subseteq A:\;
                x_n\to c \;\Rightarrow\; f(x_n)\to f(c)
              }_{\text{every approach preserves the limit}}.
        """).strip(),
    },

    {
        "id": 3,
        "file": F_DISCONTINUITY,
        "label": "def:neighborhood-discontinuity-at-a-point",
        "missing": ["Definition predicate reading", "Negated quantified statement",
                    "Negation predicate reading", "Failure modes", "Failure mode decomposition"],
        "anchor_label": r"\label{def:neighborhood-discontinuity-at-a-point}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: def:neighborhood-discontinuity-at-a-point
            MISSING: remark*[Definition predicate reading]
                     remark*[Negated quantified statement]
                     remark*[Negation predicate reading]
                     remark*[Failure modes]
                     remark*[Failure mode decomposition]

            The standard quantified statement already exists:
              f discontinuous at c
              <=> exists V nbhd of f(c), forall U nbhd of c,
                  exists x in U cap A: f(x) not in V.
            Do NOT regenerate it.

            Generate five remark* blocks:

            remark*[Definition predicate reading]
              \operatorname{DiscontinuousAt}(f,c) \coloneqq
              \exists V \text{ nbhd of } f(c)\;
              \forall U \text{ nbhd of } c\;
              \exists x\in U\cap A:\; f(x)\notin V.

            remark*[Negated quantified statement]
              The negation = continuity. Quantifier reversal (exists->forall,
              forall->exists) and negate the predicate.
              f continuous at c <=>
              forall V nbhd of f(c), exists U nbhd of c,
              forall x in U cap A: f(x) in V.
              Standard notation only — no \operatorname in this block.

            remark*[Negation predicate reading]
              \neg\operatorname{DiscontinuousAt}(f,c)
              \Longleftrightarrow \operatorname{ContinuousAt}(f,c)
              \Longleftrightarrow [full quantifier-reversed form].

            remark*[Failure modes]
              Prose. The neighbourhood discontinuity characterization fails
              (= f is actually continuous) when every output neighbourhood V of f(c)
              can be matched by some input neighbourhood U such that f maps
              all of U cap A into V. No perpetually-missed output neighbourhood exists.

            remark*[Failure mode decomposition]
              Make the quantifier reversal visually explicit with underbrace.
              \neg\operatorname{DiscontinuousAt}(f,c)
              \Longleftrightarrow
              \forall V \text{ nbhd of } f(c)\;
              \underbrace{\exists U \text{ nbhd of } c}_{\text{some input window}}
              :\;
              \underbrace{\forall x\in U\cap A:\; f(x)\in V}_{\text{all outputs inside tolerance}}.
        """).strip(),
    },

    {
        "id": 4,
        "file": F_DISCONTINUITY,
        "label": "lem:equivalent-discontinuity-at-a-point",
        "missing": ["Predicate reading", "Failure modes"],
        "anchor_label": r"\label{lem:equivalent-discontinuity-at-a-point}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: lem:equivalent-discontinuity-at-a-point
            MISSING: remark*[Predicate reading]
                     remark*[Failure modes]

            The standard quantified statement already shows the three-way
            biconditional chain (1)<->(2)<->(3). Do NOT regenerate it.

            Generate two remark* blocks:

            remark*[Predicate reading]
              Use header "Predicate reading" (not "Definition predicate reading").
              Express:
              \operatorname{DiscontinuousAt}(f,c)
              \Longleftrightarrow [eps-delta form]
              \Longleftrightarrow [sequential form]
              \Longleftrightarrow [neighbourhood form].
              All three lines use \operatorname{DiscontinuousAt}.

            remark*[Failure modes]
              Prose. The equivalence cannot fail for functions at limit points of A:
              the proof that all three are equivalent is constructive in both directions.
              Mention the degenerate case: if c is not a limit point of A, the
              punctured-ball conditions are vacuously satisfied by every L, and the
              equivalence becomes trivial rather than informative. No formal display needed.
        """).strip(),
    },

    {
        "id": 5,
        "file": F_DISCONTINUITY,
        "label": "def:types-of-discontinuity-at-a-point",
        "missing": ["Definition predicate reading", "Negation predicate reading",
                    "Failure modes", "Failure mode decomposition"],
        "anchor_label": r"\label{def:types-of-discontinuity-at-a-point}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: def:types-of-discontinuity-at-a-point
            MISSING: remark*[Definition predicate reading]
                     remark*[Negation predicate reading]
                     remark*[Failure modes]
                     remark*[Failure mode decomposition]

            The standard quantified statement and negated quantified statement
            already exist. Do NOT regenerate them.

            The three types:
              Removable: exists L in R: lim_{x->x0} f(x) = L and L != f(x0).
              Jump: exists L-, L+ in R: left-limit = L-, right-limit = L+, L- != L+.
              Essential: not removable.

            Generate four remark* blocks:

            remark*[Definition predicate reading]
              Define using \coloneqq:
              \operatorname{RemovableDiscontinuity}(f,x_0)
              \operatorname{JumpDiscontinuity}(f,x_0)
              \operatorname{EssentialDiscontinuity}(f,x_0)
              Use \operatorname{FunctionLimit}, \operatorname{LeftLimit},
              \operatorname{RightLimit} for the limit predicates.

            remark*[Negation predicate reading]
              Negate each of the three predicates using \neg\operatorname{...}.
              Standard form: neg Removable means forall L: not(limit=L) or L=f(x0).
              neg Jump means at least one one-sided limit missing or L- = L+.
              neg Essential = Removable.

            remark*[Failure modes]
              Prose, one bullet per type. What must fail for each type not to hold.
              Removable fails if no bilateral limit exists, or limit = f(x0).
              Jump fails if a one-sided limit is missing, or both sides agree.
              Essential is residual: every non-removable discontinuity is essential.

            remark*[Failure mode decomposition]
              Formal display for neg Removable and neg Jump using underbrace.
              Standard notation only (no \operatorname in underbrace labels).
        """).strip(),
    },

    {
        "id": 6,
        "file": F_UNIFORM,
        "label": "prop:lipschitz-implies-uc",
        "missing": ["Contrapositive quantified statement",
                    "Contrapositive predicate reading",
                    "Strict implication note"],
        "anchor_label": r"\label{prop:lipschitz-implies-uc}",
        "insert_before": r"\begin{remark*}[Dependencies]",
        "task": textwrap.dedent(r"""
            TARGET: prop:lipschitz-implies-uc
            MISSING: remark*[Contrapositive quantified statement]
                     remark*[Contrapositive predicate reading]
                     remark*[Strict implication note]

            These blocks go AFTER the existing Interpretation block and
            BEFORE the Dependencies block. Insert before \begin{remark*}[Dependencies].

            The proposition: exists K>=0 s.t. |f(x)-f(y)| <= K|x-y| for all x,y in A
            => f is uniformly continuous on A.

            Generate three remark* blocks:

            remark*[Contrapositive quantified statement]
              neg UniformlyContinuous => for all K>=0, exists x,y in A: |f(x)-f(y)|>K|x-y|.
              Standard notation only (no \operatorname in this block).
              One sentence of prose is fine: "If f fails to be uniformly continuous
              on A, then f is not Lipschitz on A with any constant."

            remark*[Contrapositive predicate reading]
              \neg\operatorname{UniformlyContinuous}(f,A)
              \;\Longrightarrow\;
              \neg\exists K\ge 0:\;\operatorname{Lipschitz}(f,A,K).

            remark*[Strict implication note]
              Use header "Strict implication note" (this is a non-standard remark*
              title, acceptable here as a named note).
              Prose only. The implication Lipschitz => UC is strict.
              Canonical counterexample: f(x)=sqrt(x) on [0,1] is uniformly
              continuous (Heine-Cantor) but |f(x)-f(0)|/|x-0| = 1/sqrt(x) -> inf
              as x->0+, so no Lipschitz constant exists at the origin.
              Relevance to GPU Lipschitz-constant estimation: uniform continuity
              of a simulation function does not supply a computable K for the
              Lipschitz condition; only an explicit Lipschitz bound does.
        """).strip(),
    },

    # ── DIFFERENTIATION ───────────────────────────────────────────────────────

    {
        "id": 7,
        "file": F_DERIV_DEF,
        "label": "def:topological-definition-of-derivative-at-a-point",
        "missing": ["Negated quantified statement", "Negation predicate reading", "Failure modes"],
        "anchor_label": r"\label{def:topological-definition-of-derivative-at-a-point}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: def:topological-definition-of-derivative-at-a-point
            MISSING: remark*[Negated quantified statement]
                     remark*[Negation predicate reading]
                     remark*[Failure modes]

            Standard quantified statement and predicate reading exist. Do NOT regenerate.

            The definition: f differentiable at c with derivative L iff
              forall V nbhd of L, exists U nbhd of c,
              forall x in (U\{c}) cap D: (f(x)-f(c))/(x-c) in V.

            Generate three remark* blocks:

            remark*[Negated quantified statement]
              Negate: exists V nbhd of L, forall U nbhd of c,
              exists x in (U\{c}) cap D: (f(x)-f(c))/(x-c) not in V.
              Standard notation only — no \operatorname.

            remark*[Negation predicate reading]
              \neg\operatorname{DerivativeAt}_{\mathrm{top}}(f,c,L)
              \Longleftrightarrow
              \exists V\ni L\;\forall U\ni c\;\exists x\in(U\setminus\{c\})\cap D:\;
              \frac{f(x)-f(c)}{x-c}\notin V.

            remark*[Failure modes]
              Prose. The topological derivative fails when a fixed output neighbourhood
              of L cannot be entered by the difference quotient no matter how tightly
              the input is restricted around c. Geometric reading: no punctured
              neighbourhood around c maps all secant slopes into any prescribed
              window around L.
        """).strip(),
    },

    {
        "id": 8,
        "file": F_DERIV_DEF,
        "label": "def:sequential-definition-of-derivative-at-a-point",
        "missing": ["Negated quantified statement", "Negation predicate reading",
                    "Failure modes", "Failure mode decomposition"],
        "anchor_label": r"\label{def:sequential-definition-of-derivative-at-a-point}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: def:sequential-definition-of-derivative-at-a-point
            MISSING: remark*[Negated quantified statement]
                     remark*[Negation predicate reading]
                     remark*[Failure modes]
                     remark*[Failure mode decomposition]

            Standard quantified statement and predicate reading exist. Do NOT regenerate.

            The definition: forall (x_n) in A\{c}: x_n->c => (f(x_n)-f(c))/(x_n-c)->L.

            Generate four remark* blocks:

            remark*[Negated quantified statement]
              exists (x_n) in A\{c}: x_n->c AND (f(x_n)-f(c))/(x_n-c) does NOT -> L.
              Standard notation only.
              This is the PRIMARY TOOL for disproving differentiability.

            remark*[Negation predicate reading]
              \neg\operatorname{DerivativeAt}_{\mathrm{seq}}(f,c,L)
              \Longleftrightarrow
              \exists (x_n)\subseteq A\setminus\{c\}:\;
              x_n\to c \;\land\;
              \tfrac{f(x_n)-f(c)}{x_n-c}\not\to L.

            remark*[Failure modes]
              Prose. Failure = a single approach sequence (x_n)->c along which the
              difference quotients either: (i) diverge to infinity (vertical tangent);
              (ii) oscillate without settling; (iii) converge but to a value other
              than L (wrong slope candidate). A single such sequence certifies
              non-differentiability.

            remark*[Failure mode decomposition]
              Formal display with underbrace labelling the two conjuncts.
              Standard notation only (no \operatorname in underbrace labels).
              After the display, add a sentence naming the two canonical witness types:
              two subsequences with different limiting difference quotients
              (e.g., f(x)=|x| at c=0), or quotients growing without bound.
        """).strip(),
    },

    {
        "id": 9,
        "file": F_DERIV_DEF,
        "label": "thm:derivative-equivalence",
        "missing": ["Predicate reading", "Negated quantified statement", "Failure modes"],
        "anchor_label": r"\label{thm:derivative-equivalence}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: thm:derivative-equivalence
            MISSING: remark*[Predicate reading]
                     remark*[Negated quantified statement]
                     remark*[Failure modes]

            The standard quantified statement currently just says "(i)<->(ii)<->(iii)".
            Do NOT regenerate it.

            Generate three remark* blocks:

            remark*[Predicate reading]
              \operatorname{DerivativeAt}(f,c,L)
              \Longleftrightarrow \operatorname{DerivativeAt}_{\mathrm{top}}(f,c,L)
              \Longleftrightarrow \operatorname{DerivativeAt}_{\mathrm{seq}}(f,c,L).
              One sentence: all three predicates name the same underlying condition
              on the difference quotient of f at c.

            remark*[Negated quantified statement]
              All three predicates fail simultaneously since they are equivalent.
              Reduce to the sequential negation as the computationally useful form:
              neg (i) <-> neg (ii) <-> neg (iii)
              <-> exists (x_n) in A\{c}: x_n->c and (f(x_n)-f(c))/(x_n-c) not->L.
              Standard notation only.

            remark*[Failure modes]
              Prose. The equivalence cannot produce contradictory verdicts:
              if c is a limit point of A, all three forms agree. Degenerate case:
              if c is not a limit point of A, the punctured-ball and sequence
              conditions are vacuously satisfied by every candidate L simultaneously,
              making the equivalence trivially true for all L rather than
              characterizing a unique derivative.
        """).strip(),
    },

    {
        "id": 10,
        "file": F_DERIV_DEF,
        "label": "prop:derivative-h-form-equivalence",
        "missing": ["Negated quantified statement", "Failure modes"],
        "anchor_label": r"\label{prop:derivative-h-form-equivalence}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: prop:derivative-h-form-equivalence
            MISSING: remark*[Negated quantified statement]
                     remark*[Failure modes]

            Standard quantified statement exists: DerivativeAt(f,c,L) <->
            lim_{h->0} (f(c+h)-f(c))/h = L. Do NOT regenerate it.

            Generate two remark* blocks:

            remark*[Negated quantified statement]
              neg DerivativeAt(f,c,L) <-> lim_{h->0}(f(c+h)-f(c))/h != L
              (limit does not exist or equals a value other than L).
              Note: h->0 means h != 0, i.e. the limit is over h in D-c, h != 0.
              Standard notation only.

            remark*[Failure modes]
              Prose. Two branches:
              (i) The incremental ratio (f(c+h)-f(c))/h has no finite limit as h->0
                  (corner, cusp, or vertical tangent at c).
              (ii) The ratio converges to a finite value different from L
                   (proposed L is the wrong slope).
        """).strip(),
    },

    {
        "id": 11,
        "file": F_DERIV_DEF,
        "label": "thm:differentiable-implies-continuous",
        "missing": ["Predicate reading", "Contrapositive quantified statement",
                    "Contrapositive predicate reading"],
        "anchor_label": r"\label{thm:differentiable-implies-continuous}",
        "insert_before": r"\begin{remark*}[Failure mode decomposition]",
        "task": textwrap.dedent(r"""
            TARGET: thm:differentiable-implies-continuous
            MISSING: remark*[Predicate reading]
                     remark*[Contrapositive quantified statement]
                     remark*[Contrapositive predicate reading]

            These go BEFORE the existing remark*[Failure mode decomposition] block.
            The standard quantified statement exists. Do NOT regenerate it.

            The theorem: DifferentiableAt(f,c) => ContinuousAt(f,c).

            Generate three remark* blocks:

            remark*[Predicate reading]
              \operatorname{DifferentiableAt}(f,c)
              \;\Longrightarrow\;
              \operatorname{ContinuousAt}(f,c).

            remark*[Contrapositive quantified statement]
              neg ContinuousAt(f,c) => neg DifferentiableAt(f,c).
              "If f is not continuous at c, then f is not differentiable at c."
              Standard notation only (no \operatorname in this block).
              This is the standard proof tool: to show non-differentiability,
              show discontinuity.

            remark*[Contrapositive predicate reading]
              \neg\operatorname{ContinuousAt}(f,c)
              \;\Longrightarrow\;
              \neg\operatorname{DifferentiableAt}(f,c).
        """).strip(),
    },

    {
        "id": 12,
        "file": F_DERIV_DEF,
        "label": "thm:uniqueness-of-the-derivative",
        "missing": ["Predicate reading", "Contrapositive quantified statement",
                    "Contrapositive predicate reading", "Failure modes"],
        "anchor_label": r"\label{thm:uniqueness-of-the-derivative}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: thm:uniqueness-of-the-derivative
            MISSING: remark*[Predicate reading]
                     remark*[Contrapositive quantified statement]
                     remark*[Contrapositive predicate reading]
                     remark*[Failure modes]

            Standard quantified statement exists:
              DerivativeAt(f,c,L) AND DerivativeAt(f,c,L') => L = L'.
            Do NOT regenerate it.

            Generate four remark* blocks:

            remark*[Predicate reading]
              \operatorname{DerivativeAt}(f,c,L)
              \;\land\;
              \operatorname{DerivativeAt}(f,c,L')
              \;\Longrightarrow\; L = L'.

            remark*[Contrapositive quantified statement]
              L != L' => neg(DerivativeAt(f,c,L) AND DerivativeAt(f,c,L')).
              "Two distinct candidates cannot both be the derivative of f at c."
              Standard notation only.

            remark*[Contrapositive predicate reading]
              L \ne L'
              \;\Longrightarrow\;
              \neg\operatorname{DerivativeAt}(f,c,L)
              \;\lor\;
              \neg\operatorname{DerivativeAt}(f,c,L').

            remark*[Failure modes]
              Prose, including the proof sketch.
              The uniqueness proof assumes L != L' and takes eps = |L-L'|/2 > 0.
              The definition supplies delta making the difference quotient within eps
              of both L and L' simultaneously, forcing |L-L'| < 2*eps = |L-L'|,
              a contradiction. The cluster-point hypothesis on c is essential:
              without it, the punctured-ball condition is vacuous and the limit is
              trivially satisfied by every value.
        """).strip(),
    },

    {
        "id": 13,
        "file": F_MVT,
        "label": "thm:rolles-theorem",
        "missing": ["Predicate reading", "Contrapositive quantified statement",
                    "Contrapositive predicate reading", "Failure modes"],
        "anchor_label": r"\label{thm:rolles-theorem}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: thm:rolles-theorem
            MISSING: remark*[Predicate reading]
                     remark*[Contrapositive quantified statement]
                     remark*[Contrapositive predicate reading]
                     remark*[Failure modes]

            Standard quantified statement exists. Do NOT regenerate.

            Hypotheses: ContinuousOn(f,[a,b]), DifferentiableOn(f,(a,b)), f(a)=f(b).
            Conclusion: exists c in (a,b): f'(c) = 0.

            Generate four remark* blocks:

            remark*[Predicate reading]
              \operatorname{ContinuousOn}(f,[a,b])
              \;\land\;
              \operatorname{DifferentiableOn}(f,(a,b))
              \;\land\;
              f(a)=f(b)
              \;\Longrightarrow\;
              \exists c\in(a,b):\;\operatorname{DerivativeAt}(f,c,0).

            remark*[Contrapositive quantified statement]
              forall c in (a,b): f'(c) != 0
              => neg ContinuousOn(f,[a,b]) OR neg DifferentiableOn(f,(a,b)) OR f(a)!=f(b).
              "If no interior critical point exists, at least one hypothesis fails."
              Standard notation only.

            remark*[Contrapositive predicate reading]
              \bigl(\forall c\in(a,b):\;\neg\operatorname{DerivativeAt}(f,c,0)\bigr)
              \;\Longrightarrow\;
              \neg\operatorname{ContinuousOn}(f,[a,b])
              \;\lor\;
              \neg\operatorname{DifferentiableOn}(f,(a,b))
              \;\lor\;
              f(a)\ne f(b).

            remark*[Failure modes]
              Prose with \begin{itemize}. One bullet per dropped hypothesis.
              (i) Continuity fails: f(x)=sgn(x-1/2) on [0,1] — same endpoints,
                  jump in interior, no horizontal tangent.
              (ii) Differentiability fails: f(x)=|x-1/2| on [0,1] — same endpoints,
                   corner at 1/2, f'(1/2) undefined.
              (iii) f(a)!=f(b): f(x)=x on [0,1] has f'>0 everywhere.
        """).strip(),
    },

    {
        "id": 14,
        "file": F_MVT,
        "label": "thm:mean-value-theorem",
        "missing": ["Predicate reading", "Contrapositive quantified statement",
                    "Contrapositive predicate reading", "Failure modes"],
        "anchor_label": r"\label{thm:mean-value-theorem}",
        "insert_before": r"\begin{remark*}[Interpretation]",
        "task": textwrap.dedent(r"""
            TARGET: thm:mean-value-theorem
            MISSING: remark*[Predicate reading]
                     remark*[Contrapositive quantified statement]
                     remark*[Contrapositive predicate reading]
                     remark*[Failure modes]

            Standard quantified statement exists. Do NOT regenerate.

            Hypotheses: ContinuousOn(f,[a,b]), DifferentiableOn(f,(a,b)).
            Conclusion: exists c in (a,b): f'(c) = (f(b)-f(a))/(b-a).

            Generate four remark* blocks:

            remark*[Predicate reading]
              \operatorname{ContinuousOn}(f,[a,b])
              \;\land\;
              \operatorname{DifferentiableOn}(f,(a,b))
              \;\Longrightarrow\;
              \exists c\in(a,b):\;
              \operatorname{DerivativeAt}\!\bigl(f,c,\tfrac{f(b)-f(a)}{b-a}\bigr).

            remark*[Contrapositive quantified statement]
              forall c in (a,b): f'(c) != (f(b)-f(a))/(b-a)
              => neg ContinuousOn(f,[a,b]) OR neg DifferentiableOn(f,(a,b)).
              "If no interior point achieves the average rate of change,
              f must fail continuity or differentiability."
              Standard notation only.

            remark*[Contrapositive predicate reading]
              \Bigl(\forall c\in(a,b):\;
              f'(c)\ne\tfrac{f(b)-f(a)}{b-a}\Bigr)
              \;\Longrightarrow\;
              \neg\operatorname{ContinuousOn}(f,[a,b])
              \;\lor\;
              \neg\operatorname{DifferentiableOn}(f,(a,b)).

            remark*[Failure modes]
              Prose with \begin{itemize}. One bullet per dropped hypothesis.
              (i) Continuity fails: a step function whose endpoints straddle the jump
                  — no interior point matches the secant slope.
              (ii) Differentiability fails: f(x)=|x| on [-1,1] — secant slope is 0
                   but f'(x)=+-1 everywhere f is differentiable.
              Close with: "The MVT is an existence result. It guarantees some c
              exists but gives no formula for it."
        """).strip(),
    },
]


# ── helpers ────────────────────────────────────────────────────────────────────

def find_after(text: str, anchor_label: str, search_str: str) -> int:
    """
    Return the index of the first occurrence of search_str that appears
    AFTER anchor_label in text. Returns -1 if not found.
    """
    anchor_pos = text.find(anchor_label)
    if anchor_pos == -1:
        # Try compact whitespace match
        compact_anchor = re.sub(r'\s+', ' ', anchor_label)
        compact_text   = re.sub(r'\s+', ' ', text)
        pos = compact_text.find(compact_anchor)
        if pos == -1:
            return -1
        anchor_pos = pos   # approximate; fine for ordering

    idx = text.find(search_str.strip(), anchor_pos)
    return idx


def already_has_block(text: str, anchor_label: str, block_header: str) -> bool:
    """
    Return True if block_header (e.g. 'Failure modes') already exists
    after anchor_label in text.
    """
    search = rf'\begin{{remark*}}[{re.escape(block_header)}]'
    idx = find_after(text, anchor_label, search)
    return idx != -1


def call_api(gap: dict, file_text: str, debug: bool = False) -> str:
    """
    Call the Anthropic API with the full file text as context and the gap task.
    Returns the raw response text.
    """
    user_msg = textwrap.dedent(f"""
        Below is the FULL CONTENT of the file you are editing.
        Study it carefully to understand the existing blocks before generating.

        FILE: {gap['file'].name}
        ========================================================
        {file_text}
        ========================================================

        TASK:
        {gap['task']}

        Generate ONLY the missing remark* blocks listed in the task.
        Output raw LaTeX. Begin with the first \\begin{{remark*}} to insert.
        End after the last \\end{{remark*}} to insert.
        No explanatory prose outside the LaTeX.
    """).strip()

    if debug:
        print(f"\n{'='*60}\nAPI USER MESSAGE (gap {gap['id']}):\n{user_msg[:800]}...\n{'='*60}")

    response = CLIENT.messages.create(
        model=MODEL,
        max_tokens=4096,
        system=SYSTEM,
        messages=[{"role": "user", "content": user_msg}],
    )
    return response.content[0].text


def strip_code_fences(text: str) -> str:
    """Remove markdown code fences if the model accidentally adds them."""
    text = re.sub(r'^```[a-z]*\n?', '', text, flags=re.MULTILINE)
    text = re.sub(r'\n?```$', '', text, flags=re.MULTILINE)
    return text.strip()


def apply_gap(gap: dict, texts: dict[Path, str], dry_run: bool, debug: bool) -> bool:
    """
    Process one gap: check if already present, call API if not, splice result.
    texts is a mutable dict Path->current_text (accumulates edits in memory).
    Returns True if a change was made.
    """
    path = gap["file"]
    text = texts[path]

    # Check if ALL missing blocks are already present
    all_present = all(
        already_has_block(text, gap["anchor_label"], b) for b in gap["missing"]
    )
    if all_present:
        print(f"  GAP {gap['id']:2d} [{gap['label']}]: already complete, skipping")
        return False

    # Find splice point
    insert_idx = find_after(text, gap["anchor_label"], gap["insert_before"])
    if insert_idx == -1:
        print(f"  GAP {gap['id']:2d} [{gap['label']}]: "
              f"ERROR — insert_before string not found after anchor")
        return False

    print(f"  GAP {gap['id']:2d} [{gap['label']}]: calling API…", end="", flush=True)
    raw = call_api(gap, text, debug=debug)
    cleaned = strip_code_fences(raw)

    if debug:
        print(f"\n--- API RESPONSE (gap {gap['id']}) ---\n{cleaned}\n---")

    # Ensure the block ends with a blank line before insert_before
    if not cleaned.endswith("\n\n"):
        cleaned = cleaned.rstrip("\n") + "\n\n"

    new_text = text[:insert_idx] + cleaned + text[insert_idx:]

    if dry_run:
        # Print a brief diff
        import difflib
        diff = list(difflib.unified_diff(
            text.splitlines(), new_text.splitlines(),
            fromfile=f"{path.name} (before)",
            tofile=f"{path.name} (after)",
            lineterm="", n=2
        ))
        print(f" DRY RUN — {len(diff)} diff lines")
        for line in diff[:60]:
            print("    " + line)
        if len(diff) > 60:
            print(f"    ... ({len(diff)-60} more lines)")
    else:
        texts[path] = new_text
        print(" done")

    return not dry_run


def write_files(texts: dict[Path, str], originals: dict[Path, str]) -> None:
    """Write modified files to disk, first backing up any that changed."""
    for path, new_text in texts.items():
        if new_text == originals[path]:
            continue
        bak = path.with_suffix(".tex.bak")
        if not bak.exists():
            bak.write_text(originals[path], encoding="utf-8")
            print(f"  Backup: {bak.name}")
        path.write_text(new_text, encoding="utf-8")
        print(f"  Written: {path.name}")


# ── main ──────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="Fill missing logical blocks via API")
    parser.add_argument("--gaps", nargs="+", type=int, metavar="N",
                        help="gap IDs to process (default: all)")
    parser.add_argument("--dry-run", action="store_true",
                        help="print diffs but do not write files")
    parser.add_argument("--debug", action="store_true",
                        help="print raw API messages and responses")
    args = parser.parse_args()

    selected_ids = set(args.gaps) if args.gaps else {g["id"] for g in GAPS}
    selected = [g for g in GAPS if g["id"] in selected_ids]

    if not selected:
        sys.exit("No matching gaps found.")

    # Check all target files exist
    needed_files = {g["file"] for g in selected}
    for p in needed_files:
        if not p.exists():
            sys.exit(f"File not found: {p}")

    # Load all needed files once
    originals: dict[Path, str] = {p: p.read_text(encoding="utf-8") for p in needed_files}
    texts: dict[Path, str] = dict(originals)  # mutable working copy

    print(f"Processing {len(selected)} gap(s) "
          f"{'(DRY RUN)' if args.dry_run else ''}")
    print("─" * 60)

    changed = False
    for gap in selected:
        try:
            if apply_gap(gap, texts, dry_run=args.dry_run, debug=args.debug):
                changed = True
        except Exception as exc:
            print(f"\n  GAP {gap['id']:2d} [{gap['label']}]: FAILED — {exc}")
            if args.debug:
                import traceback; traceback.print_exc()

    print("─" * 60)

    if not args.dry_run and changed:
        print("Writing files…")
        write_files(texts, originals)
    elif not changed:
        print("No changes.")
    else:
        print("Dry run complete. No files written.")


if __name__ == "__main__":
    main()
