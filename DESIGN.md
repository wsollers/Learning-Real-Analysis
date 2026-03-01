# House Style Guide — Learning Abstract Mathematics

> **Quick reference:** Open [`box-vocabulary.html`](box-vocabulary.html) in a browser for the
> visual design sample with colour swatches, live box previews, and the full-page simulation.

---

## 1. File and Folder Conventions

### Compilation
Compile `main.tex` from the project root in Overleaf.
All `\input{}` paths are **relative to the project root**.

### Volume Layout

```
main.tex                        ← Root file (\input per volume)
blueprint-macros.tex            ← Shared tcolorbox macros (blueprints only)
box-vocabulary.html             ← Visual design reference (open in browser)
bibliography/
appendices/
common/
glossary-foundations.tex
map.tex

volume-i/                       ← VOLUME I: Mathematical Logic
  index.tex                     ← \part{} + chapter inputs
  propositional-logic/
  predicate-logic/
  sets-relations-functions/
  axiom-systems/
  proof-techniques/
  model-theory/                 ← STUB
  type-theory/                  ← STUB

volume-ii/                      ← VOLUME II: Foundations of Formal Number Systems
  index.tex
  natural-numbers/              ← STUB (Peano in volume-i/axiom-systems)
  integers/
  rationals/                    ← STUB
  reals/
  complex/                      ← STUB

volume-iii/                     ← VOLUME III: Abstract Mathematics
  index.tex
  analysis/
    real-analysis/
    metric-spaces/
    point-set-topology/
    measure-theory/             ← STUB
  algebra/
    algebraic-structures/
    set-algebras/
    linear-algebra/
    abstract-algebra/
    algebraic-geometry/
```

### Chapter Folder Structure

```
<chapter>/
  index.tex           ← \chapter{}, \section{}s, \input{}s — no content here
  notes/
    index.tex         ← Orchestrator: \input each topic file in dependency order
    <topic>/          ← Subject subfolder when a topic has 3+ files
      index.tex       ← \input each file in the subfolder
      <file>.tex      ← One concept, one file
  proofs/
    index.tex         ← Orchestrator: \subsection*{Source} headers + \input each proof
    <source>/         ← One folder per source (bjorndahl/, suppes/, gallian/, etc.)
      <ID>.tex        ← One proof per file
  capstone.tex        ← End-of-chapter synthesis
```

### Reading Lists

Reading list files (`reading-list.tex`) have been **removed** from the project.
Annotated bibliographies should be maintained externally (e.g.\ in Zotero or
a separate reference document) rather than compiled into the LaTeX document.
All `\section{Reading List}` entries have been removed from chapter `index.tex` files.

### Proof File Naming

```
<SUBJ>-<SRC>-C<ch>[-S<sec>]-E<ex>[a|b]
  PL-BJO-C01-E02a          Propositional Logic, Bjørndahl, Ch 1, Ex 2a
  RA-TAO-C04-S4.1-E01      Real Analysis, Tao, Ch 4, §4.1, Ex 1
  AA-GAL-C07-E19           Abstract Algebra, Gallian, Ch 7, Ex 19
  LA-AXL-C01-S1.B-E03      Linear Algebra, Axler, Ch 1, §1.B, Ex 3
```

Source abbreviations: `BJO` Bjørndahl · `SUP` Suppes · `GER` Gerstein ·
`TAO` Tao · `GAL` Gallian · `AXL` Axler · `MEN` Mendelson · `ABB` Abbott ·
`ROS` Rosen · `FAX` forall x

---

## 2. Box Vocabulary

> See `box-vocabulary.html` for a rendered preview of every box type and a
> full-page simulation showing how they sit alongside bare environments.

There are **four box types** and **six bare environments**. The governing
principle is: boxes mark *what you need to know*; everything else flows as
plain text with amsthm formatting.

### 2.1 Colour Palette

| Box | LaTeX colours | Role |
|-----|---------------|------|
| Definition | `colback=propbox, colframe=propborder` (steel blue) | Naming and pinning concepts |
| Axiom | `colback=axiombox, colframe=axiomborder` (deep violet) | Foundational, unproven primitives |
| Theorem | `colback=thmbox, colframe=thmborder` (warm amber) | Major named results |
| Toolkit | `colback=gray!6, colframe=gray!40` (neutral gray) | Quick-reference summary tables |

Add these colours to `main.tex` alongside the existing `propbox`/`propborder`:

```latex
% Axiom box
\definecolor{axiombox}   {HTML}{f3f0fa}
\definecolor{axiomborder}{HTML}{8a6abf}
% Theorem box
\definecolor{thmbox}     {HTML}{fff8f0}
\definecolor{thmborder}  {HTML}{c07840}
```

### 2.2 Box Templates

**Definition box** — blue, used for every definition on first appearance:
```latex
\begin{tcolorbox}[colback=propbox, colframe=propborder, arc=2pt,
  left=6pt, right=6pt, top=4pt, bottom=4pt,
  title={\small\textbf{Definition 1.2 (Vector Space)}},
  fonttitle=\small\bfseries]
...content...
\end{tcolorbox}
```

**Axiom box** — violet, used for foundational primitives:
```latex
\begin{tcolorbox}[colback=axiombox, colframe=axiomborder, arc=2pt,
  left=6pt, right=6pt, top=4pt, bottom=4pt,
  title={\small\textbf{Axiom P5 (Mathematical Induction)}},
  fonttitle=\small\bfseries]
...content...
\end{tcolorbox}
```

**Theorem box** — amber, used for major named results:
```latex
\begin{tcolorbox}[colback=thmbox, colframe=thmborder, arc=2pt,
  left=6pt, right=6pt, top=4pt, bottom=4pt,
  title={\small\textbf{Theorem (Compactness — Tao §X.Y)}},
  fonttitle=\small\bfseries]
...content...
\end{tcolorbox}
```

**Toolkit box** — gray, one per section, always at the top:
```latex
\begin{tcolorbox}[colback=gray!6, colframe=gray!40, arc=2pt,
  left=6pt, right=6pt, top=4pt, bottom=4pt,
  title={\small\textbf{Addition Toolkit — Quick Reference}},
  fonttitle=\small\bfseries]
...summary table...
\end{tcolorbox}
```

---


---

## 2b. Chapter Entry Pattern — Breadcrumb and Roadmap

Every chapter that has a `notes.tex` (or where `notes/index.tex` serves as the
entry point) **must** open with two elements in this order:

### 1. Breadcrumb Box

The breadcrumb box shows the reader where they are in the full
curriculum progression. It uses the gray toolkit style:

```latex
\begin{tcolorbox}[
  colback=gray!6,
  colframe=gray!40,
  arc=2pt,
  left=8pt, right=8pt, top=6pt, bottom=6pt,
  title={\small\textbf{Where You Are in the Journey}},
  fonttitle=\small\bfseries
]
\begin{center}
\small
Propositional Logic
$\;\to\;$ Predicate Calculus
$\;\to\;$ \textbf{[Current Chapter]}
$\;\to\;$ [Next Chapter]
$\;\to\;$ $\cdots$
\end{center}

\medskip
\noindent\textbf{How we got here.}
[One paragraph: what the preceding chapters contributed and why this
chapter is the natural next step.]

\medskip
\noindent\textbf{What this chapter builds.}
[One paragraph: the core content and central objects of study.]

\medskip
\noindent\textbf{Where this leads.}
[One paragraph: how this chapter's results feed the next.]
\end{tcolorbox}
\vspace{1em}
```

### 2. Structural Roadmap

Immediately after the breadcrumb, a `\subsection*{Structural Roadmap}`
block gives the enumerated progression of topics within the chapter:

```latex
\subsection*{Structural Roadmap}

Each major topic is organised as:
\begin{center}
\textbf{Definitions $\longrightarrow$ Main Theorems
$\longrightarrow$ Consequences and Structural Insight}
\end{center}

The global progression is:
\begin{enumerate}
  \item [First major topic]
  \item [Second major topic]
  \ldots
\end{enumerate}

\begin{remark}[Primary sources]
[Books and chapter references driving this chapter.]
\end{remark}
```

### Rules

- The breadcrumb lives in `notes.tex` (or `notes/index.tex` when no `notes.tex` exists)
- It is the **first** thing in the file — before any `\subsection` or content
- Every chapter, including STUB chapters, has a breadcrumb in its `index.tex`
- STUB chapters place the breadcrumb + a *Status: Planned* box directly in `index.tex`
- Fully written chapters place the breadcrumb in `notes.tex` and input it via `index.tex`

---

## 3. Governing Rules

### Rule R1 — What gets a box

| Type | Rule |
|------|------|
| **Definition** | **Always boxed, on first appearance only.** A definition is any statement of the form "X is defined as / called / denoted Y." Includes recursive definitions, notational conventions introduced for the first time, and operator definitions (addition, multiplication, order, inner product, etc.). |
| **Axiom** | **Always boxed.** An axiom is an unproven primitive: Peano axioms, ZFC axioms, field axioms, vector space axioms. When a full axiom system is listed, one box holds the entire list. |
| **Theorem** | **Boxed only when:** (a) it has a proper name (Compactness Theorem, Heine-Borel, De Morgan's Laws as a group), **and** (b) it is the primary result of its section and will be cited by name throughout later notes. Unnamed theorems are never boxed — they are propositions. |
| **Toolkit** | **One per section, at the top only.** The gray toolkit box is for a quick-reference summary table listing all key results in the section with their proof methods. It appears *before* the detailed bare environments begin. Never mid-section. |

### Rule R2 — What never gets a box

- **Lemmas** — always bare. Lemmas are infrastructure.
- **Propositions** — always bare. They derive from definitions.
- **Corollaries** — always bare. They are consequences.
- **Remarks** — always bare. Use *italic text* or bold opener for emphasis.
- **Examples** — always bare.
- **Proofs** — always bare.
- **Second appearances** — if Definition 2.2.1 was boxed in §2.2, never box it again in §3.4. Just cite: "By Definition 2.2.1 …"

### Rule R3 — The Import Test

> *"Is this the statement that someone opening to this section needs to immediately understand and remember?"*

- **Yes → box it** (if it qualifies under R1).
- **It follows from the boxed material → don't box it.**

**Example (correct):** In a section on Vector Spaces, the eight axioms live in one Definition box. The proof that the zero vector is unique is a bare Proposition.

**Example (correct):** In a section specifically titled *The Compactness Theorem*, the theorem statement lives in a Theorem box.

**Anti-pattern to avoid:** Boxing every individual logical equivalence in an equivalences section (Double Negation, Commutativity, Associativity, …). These should be collected into **one Toolkit table** at the section head, with individual bare Propositions below for formal statements. The box is for the *collection*, not each member.

---

## 4. Toolkit + Detail Pattern

The standard pattern for a notes topic file is:

```
1. Toolkit box         ← summary table: label | statement | proof method | detail link
2. Bare environments   ← lemma / proposition / corollary / remark / proof
   in logical dependency order
```

### Toolkit Table Format

The toolkit table has four columns. The last column is a `\hyperref` link to
the detailed entry below:

```latex
\begin{tcolorbox}[colback=gray!6, colframe=gray!40, arc=2pt, ...]
\small
\begin{tabular}{l l l l}
\toprule
\textbf{Label} & \textbf{Statement} & \textbf{Proof method} & \textbf{Detail} \\
\midrule
L2.2.2 & $n + 0 = n$              & Induction on $n$  & \hyperref[lem:add-zero-right]{↓} \\
L2.2.3 & $n + (m\pp) = (n+m)\pp$ & Induction on $n$  & \hyperref[lem:add-succ-right]{↓} \\
P2.2.4 & $n + m = m + n$          & Induction on $n$  & \hyperref[prop:add-comm]{↓} \\
\bottomrule
\end{tabular}
\end{tcolorbox}
```

### Detail Entry Format

Each detailed entry below the toolkit is a bare `lemma` / `proposition` /
`definition` environment with a `\label` matching the toolkit link,
followed by the fully quantified statement, then remarks covering:

```latex
\begin{lemma}[Tao 2.2.2 — Zero on the right]
\label{lem:add-zero-right}
For all $n \in \mathbb{N}$,\quad $n + 0 = n$.
\end{lemma}

\begin{remark}[Intuition]
A1 gives $0 + m = m$ (zero on the left). This lemma gives $n + 0 = n$
(zero on the right). They are different facts — commutativity is not yet proved.
\end{remark}

\begin{remark}[Fully quantified form]
$\forall n \in \mathbb{N},\ n + 0 = n$.
\end{remark}

\begin{remark}[Proof strategy]
Induct on $n$. Base: $0 + 0 = 0$ by A1. Step: assume $n + 0 = n$;
show $(n\pp) + 0 = n\pp$ by A2 then IH.
\end{remark}

\begin{remark}[Consequence]
Together with L2.2.3 this enables the proof of commutativity (P2.2.4).
\end{remark}
```

### Standard Remark Sequence After a Result

Not every remark applies to every result. Use what is warranted:

| Remark tag | Content |
|------------|---------|
| `[Intuition]` | Plain-English meaning; what the result is *doing* |
| `[Fully quantified form]` | The statement written out with all quantifiers explicit |
| `[Proof strategy]` | Which technique (induction on what variable, contradiction, construction) and why |
| `[Consequence]` | What this result unlocks or is used to prove |
| `[Common error]` | A mistake to avoid when applying this result |
| `[Source note]` | How this result's name or numbering differs across sources |

---

## 5. Content Status

| Chapter | Breadcrumb | Notes | Status |
|---------|------------|-------|--------|
| Propositional Logic | ✓ | ✓ | Complete |
| Predicate Logic | ✓ | ✓ | Complete |
| Sets, Relations, Functions | ✓ | ✓ | Complete |
| Axiom Systems | ✓ | ✓ | Complete |
| Proof Techniques | ✓ | ✓ | Complete |
| Model Theory | ✓ | — | STUB |
| Type Theory | ✓ | — | STUB |
| Natural Numbers | ✓ | — | STUB (Peano content in Axiom Systems) |
| Integers | ✓ | ✓ | Complete |
| Rationals | ✓ | — | STUB |
| Real Numbers (ℝ) | ✓ | ✓ | Complete (foundations + sequences + series + constructions) |
| Complex | ✓ | — | STUB |
| Real Analysis | ✓ | ✓ | Notes in Vol II; analysis proper TBD |
| Metric Spaces | ✓ | ✓ | Complete |
| Point-Set Topology | ✓ | ✓ | Complete |
| Measure Theory | ✓ | — | STUB |
| Algebraic Structures | ✓ | ✓ | Complete |
| Set Algebras | ✓ | ✓ | Complete |
| Linear Algebra | ✓ | ✓ | Complete |
| Abstract Algebra | ✓ | ✓ | Complete |
| Algebraic Geometry | ✓ | ✓ | Complete |
