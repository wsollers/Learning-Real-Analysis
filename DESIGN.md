# House Style Guide — Learning Abstract Mathematics

> **Quick reference:** Open [`box-vocabulary.html`](box-vocabulary.html) in a browser for the
> visual design sample with colour swatches, live box previews, and the full-page simulation.

---

## 0. Mandatory Pre-Flight for All Proof Commands

Before executing ANY of these commands:
- `add standard proof for <name>`
- `add justified proof for <name>`
- `convert proof standard <name>`

Execute these steps IN ORDER before touching any file:

1. Search project knowledge: `"proof workflow steps proofs-to-do"` → load DESIGN.md §6.5
2. Search project knowledge: `"proofs-to-do <name>"` → get the exact source file path and current status from `proofs-to-do.md`
3. Open the source file identified in step 2 — not any other file with a similar name
4. Only then proceed with DESIGN.md §6.5 Steps 1–6 in order

**No exceptions. Do not infer the source file from memory or search results.**

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


## 6. Proof Architecture

### 6.1 Directory Structure

Each chapter's `proofs/` folder contains three subdirectories:

```
proofs/
  notes/
    index.tex       ← Orchestrator: \input each proof in dependency order
    <proof-id>.tex  ← One proof per file
  exercises/
    index.tex       ← Orchestrator: \input each exercise proof
    CANDIDATES.md   ← Log of results available as future exercises
    <proof-id>.tex  ← One exercise proof per file
  scratch/
    (never compiled — working drafts only)
```

`proofs/scratch/` is never `\input`'d anywhere.

---

### 6.2 Proof Classification

**Notes proofs** (`proofs/notes/`) are canonical reference proofs. A result
earns a notes proof entry if any of the following hold:

- The proof introduces a technique that recurs (induction, epsilon arguments,
  equivalence class manipulation, diagonal arguments)
- The result is load-bearing for a subsequent construction
- The proof structure is non-obvious and worth studying again
- The proof is a canonical example of a general pattern

Propositions and corollaries whose proofs follow immediately from preceding
results do **not** get a notes proof entry — log them in
`proofs/exercises/CANDIDATES.md` instead.

**Exercise proofs** (`proofs/exercises/`) are working practice and disposable.

---

### 6.3 Proof File Naming

File names use the theorem name, lowercased, with **hyphens** instead of
spaces, underscores, or special characters. Do not include `$`, `\neq`,
`\leq`, or LaTeX markup in file names.

```
Tao 2.1.4                          → tao-2-1-4.tex
Tao 2.1.6 — 4 ≠ 0                 → tao-2-1-6-four-neq-zero.tex
Commutativity of Addition           → commutativity-of-addition.tex
Bolzano–Weierstrass                 → bolzano-weierstrass.tex
ε-Characterization of Supremum      → eps-characterization-of-supremum.tex
Uniqueness of the Additive Identity → uniqueness-of-additive-identity.tex
```

The file lives in the `proofs/notes/` folder of the **same chapter** as the
notes file that contains the theorem. For example:

```
Notes file:  volume-i/axiom-systems/notes/peano/notes-peano-numerals.tex
Proof file:  volume-i/axiom-systems/proofs/notes/tao-2-1-4.tex
Index file:  volume-i/axiom-systems/proofs/notes/index.tex
```

---

### 6.4 Label Conventions

Every theorem-like environment in the notes **must** carry a `\label` before
its proof file is created. Labels follow the environment-type prefix:

| Environment   | Label prefix | Example                        |
|---------------|-------------|--------------------------------|
| `theorem`     | `thm:`      | `\label{thm:bolzano-weierstrass}` |
| `lemma`       | `lem:`      | `\label{lem:add-zero-right}`   |
| `proposition` | `prop:`     | `\label{prop:tao-2-1-4}`       |
| `corollary`   | `cor:`      | `\label{cor:sum-zero}`         |
| `definition`  | `def:`      | `\label{def:supremum}`         |

The proof file's anchor label uses the prefix `prf:` with the **same root**
as the notes label:

```
Notes label:  \label{prop:tao-2-1-4}
Proof label:  \label{prf:tao-2-1-4}
```

The root identifier matches the proof file name (with hyphens replacing dots
where needed):

```
File name:   tao-2-1-4.tex
Root:        tao-2-1-4
Notes label: \label{prop:tao-2-1-4}
Proof label: \label{prf:tao-2-1-4}
```

---

### 6.5 Step-by-Step Procedure for Adding a Proof

Follow these steps in order every time a proof is added.

**Step 1 — Add a label to the notes environment (if missing)**

Open the notes file. Find the `\begin{proposition}[...]` (or `\begin{theorem}`,
`\begin{lemma}`, `\begin{corollary}`) that needs a proof. Add `\label{...}`
immediately after the opening line, before the statement text:

```latex
\begin{proposition}[Tao 2.1.4]
\label{prop:tao-2-1-4}
$3$ is a natural number.
\end{proposition}
```

**Step 2 — Make the theorem name itself the proof link**

Replace the plain `[Name]` in the theorem's optional argument with a
`\texorpdfstring{\hyperref[prf:...]{Name}}{Name}` pair.
This makes the theorem name render as a clickable blue link directly in the
proposition heading — no separate remark line, no visual clutter.

```latex
\begin{proposition}[\texorpdfstring{\hyperref[prf:root-id]{Name}}{Name}]
\label{prop:root-id}
Statement.
\end{proposition}
```

The PDF-string argument (second brace of `\texorpdfstring`) is the plain-text
version written into PDF bookmarks — omit math and commands here.

For names containing math (e.g. `$4 \neq 0$`), write the math in the first
argument and a plain fallback in the second:

```latex
\begin{proposition}[\texorpdfstring{%
  \hyperref[prf:tao-2-1-6]{Tao 2.1.6 --- $4 \neq 0$}%
}{Tao 2.1.6 --- 4 neq 0}]
\label{prop:tao-2-1-6}
$4$ is not equal to $0$.
\end{proposition}
```

**No separate `\begin{remark}[Proof]` block.** Do not add one. The link lives
in the heading. This is the only correct pattern — an earlier version of this
document described a `\begin{remark}[Proof]` forward-nav block; that pattern
is retired and must not be used. Existing explanatory remarks (`[Why ...]`,
`[Intuition]`, etc.) follow `\end{proposition}` as before — they are unaffected.

**Prerequisite in `main.tex`:** the following must appear once after
`\hypersetup{...}` (already present in this project):
```latex
\pdfstringdefDisableCommands{%
  \def\hyperref[#1]#2{#2}%
}
```
This prevents hyperref from erroring when it writes theorem titles to PDF
bookmarks.

**Step 3 — Create the proof file**

Create `<proof-id>.tex` in the chapter's `proofs/notes/` directory.
The file has exactly four parts, in this order:

```latex
% =========================================================
% Proof: [Full theorem name]
% Source: [path to notes file relative to project root]
% =========================================================

\subsection*{[Theorem name as it appears in notes]}
\label{prf:root-identifier}

\begin{remark}[Return]
\hyperref[prop:root-identifier]{$\leftarrow$ Back to Proposition ([Name]) in Notes}
\end{remark}

\begin{proof}
\Claim [Full statement copied verbatim from notes.]

[Proof body — see §6.6 for format]
\end{proof}

\begin{remark}[Proof shape]
[One paragraph: which technique was used and why.]
\end{remark}

\begin{remark}[Dependencies]
[Which definitions, axioms, or prior results this proof requires, with \ref labels.]
\end{remark}
```

Rules for the four parts:

1. **Header comment** — always include the full theorem name and the path to
   the notes file. This makes the file self-documenting.

2. **`\subsection*` + `\label{prf:...}`** — the subsection heading is what
   appears in the compiled PDF. The label is what the notes file's
   `\hyperref` targets. Both are required.

3. **Return remark** — always the first environment after the label. Use
   `$\leftarrow$` for the arrow. Reference the environment type correctly
   (Proposition / Theorem / Lemma / Corollary).

4. **`\Claim` inside the proof body** — the first line of the proof body
   states the claim using `\Claim [statement.]`. Copy the statement verbatim
   from the notes so the proof file is self-contained when read in isolation.
   Do **not** use `\begin{proposition}`, `\begin{corollary}`, or any other
   theorem-like environment for this purpose (see Rule PB-1 in §7.5).

5. **Proof body** — see §6.6 for normal vs. thorough mode.

6. **Closing remarks** — always include at minimum `[Proof shape]` and
   `[Dependencies]`. Add `[Common error]` or `[Generalisation]` when warranted.

**Step 4 — Update the chapter's `proofs/notes/index.tex`**

Add an `\input{}` line for the new proof file. Lines must appear in
**dependency order**: if proof B uses a result established in proof A, then A's
`\input` line comes first. Replace the `% (empty)` placeholder on first use.

```latex
% ---- [Notes source file name] ----
% Dependency order: [list results in order]
\input{volume-i/axiom-systems/proofs/notes/tao-2-1-4}
\input{volume-i/axiom-systems/proofs/notes/tao-2-1-6}
\input{volume-i/axiom-systems/proofs/notes/tao-2-1-8}
```

The `\input` path is **always relative to the project root** (i.e. relative
to where `main.tex` lives), never relative to the index file itself.

---

### 6.6 Two Proof Modes

All proofs default to **normal mode** unless explicitly requested otherwise.

#### Normal Mode

Normal mode proofs are clean, concise academic prose using the house macros.
No step-by-step justification table. Commentary is woven into the prose.

The full template is in `common/proof-template.tex`. The essential shapes are:

**Direct proof:**
```latex
\begin{proof}
\Given [hypotheses in plain English].
\Goal To show [conclusion in one sentence].
\Strategy We proceed [directly / by induction / by contradiction / ...].
\Recall [key definition or lemma, with \ref if labelled].

[Main argument as prose paragraphs, using:]
\Let $x$ be arbitrary.
\Choose $\delta := \dots$ (such a $\delta$ exists because \dots).
\ByDef [unpack the definition at the critical step].
\ByThm{[Name or \ref]} [cite prior result].
\Therefore [intermediate conclusion].

\Hence [restate final conclusion matching the theorem statement]. \AsReq
\end{proof}
```

**Induction:**
```latex
\begin{proof}
\Strategy We proceed by induction on $n$.

\textbf{Base case} ($n = 0$). [Proof for base value.] \checkbox

\textbf{Inductive hypothesis.} Assume [P(n)] for some fixed $n$.

\textbf{Inductive step} (show $P(n) \Rightarrow P(n+1)$).
[Argument using IH, ending with:] \checkbox

\Hence $P(n)$ holds for all $n \in \mathbb{N}$. \AsReq
\end{proof}
```

**Bidirectional (iff / set equality):**
```latex
\begin{proof}
We prove both directions.

\medskip
\noindent$(\Rightarrow)$ \quad \textit{Assume $P$; we show $Q$.}
\Given ... \Goal ...
[Argument.] \checkbox

\medskip
\noindent$(\Leftarrow)$ \quad \textit{Assume $Q$; we show $P$.}
\Given ... \Goal ...
[Argument.] \AsReq
\end{proof}
```

**Contradiction / contrapositive:**
```latex
% Contradiction
\begin{proof}
\Strategy Suppose, toward a contradiction, that [negation of conclusion].
[Derive contradiction.]
This contradicts [what]. \Therefore [conclusion] must hold. \AsReq
\end{proof}

% Contrapositive
\begin{proof}
\Strategy We prove the contrapositive: assume $\neg Q$ and deduce $\neg P$.
\ByAss $\neg Q$, i.e.\ [unpack].
[Argument establishing $\neg P$.]
\Therefore $\neg P$. \AsReq
\end{proof}
```

**Equations in normal mode:**

| Need | LaTeX |
|------|-------|
| Unnumbered display | `\[ ... \]` |
| Numbered (cite with `\eqref`) | `\begin{equation}\label{eq:name} ... \end{equation}` |
| Unnumbered multi-line | `\begin{align*} ... \end{align*}` |
| Numbered multi-line | `\begin{align} ... \label{eq:row} \end{align}` |
| Annotated step | `&= \dots && \text{(reason)}` |

**Tag macros** (available but used sparingly in normal mode — inline only,
never in a column):

| Macro | Meaning |
|-------|---------|
| `\tagBC` | Base Case |
| `\tagIS` | Inductive Step |
| `\tagIH` | Inductive Hypothesis applied |
| `\tagDU` | Definition Unpacked |
| `\tagTA` | Theorem Applied |
| `\tagAM` | Algebraic Manipulation |
| `\tagCS` | Case Split |
| `\tagEX` | Existence argument |
| `\tagUW` | Universal/Witness introduction |

#### Thorough Mode

Thorough mode uses the three-column tag/step/justification table format.
Use it only when explicitly requested, or for proofs where every logical
step must be visibly justified (Bolzano–Weierstrass, Archimedean property,
construction-level arguments).

Column types `T` (tag), `S` (step, displaymath), `J` (justification) are
defined in `main.tex`. See `volume-iii/algebra/abstract-algebra/notes/sample-theorem.tex`
for a complete worked example.

```latex
\noindent\textit{Part N: [Part title].}
\medskip

\noindent
\begin{tabular}{T S J}
\toprule
\textbf{Tag} & \multicolumn{1}{p{0.44\textwidth}}{\textbf{Step}} & \textbf{Justification} \\
\midrule
\addlinespace[4pt]

\tagDU & [math step] & [Justification text.] \\[10pt]
\tagTA & [math step] & [Justification text.] \\[10pt]
\tagAM & [math step] & [Justification text.] \\[6pt]

\bottomrule
\end{tabular}
```

Sub-arguments (contradiction boxes, case splits) use the `subproof` mdframed
environment defined in `sample-theorem.tex`.

---

### 6.7 Exercise Candidates Log

`proofs/exercises/CANDIDATES.md` records results that are good exercise
material but don't warrant a notes proof entry:

```
| Result | Notes file | What the proof requires |
|--------|------------|-------------------------|
| Prop 2.2.5 — Associativity | natural-numbers/notes/arithmetic/addition/notes-add-toolkit.tex | Induction on c; uses lem:add-zero-right and lem:add-succ-right |
```
---

## 7. Session Commands

These are the three commands William uses to add and manage proof files.
Each command triggers a precise, repeatable workflow. All three are
documented here so any session can execute them without improvisation.

---

### 7.1 `add standard proof for <theorem-name>`

**What it means:** Create a normal-mode proof file for the named theorem,
add labels and nav links to the notes file, update the chapter index, and
mark the entry in `proofs-to-do.md` as done.

**When to use:** Default for all new proofs unless justified mode is
explicitly requested.

**Full procedure:**

1. **Locate the theorem in the notes files.**
   Search the chapter's `notes/` tree for the theorem name.
   Read the full statement and any surrounding remarks to understand context,
   dependencies, and what axioms/definitions are in play.

2. **Add `\label` to the notes environment** (if missing — §6.4):
   ```latex
   \begin{proposition}[Name]
   \label{prop:root-id}
   Statement.
   \end{proposition}
   ```

3. **Make the theorem name the proof link** (§6.5 Step 2):
   ```latex
   \begin{proposition}[\texorpdfstring{\hyperref[prf:root-id]{Name}}{Name}]
   \label{prop:root-id}
   Statement.
   \end{proposition}
   ```
   No separate `\begin{remark}[Proof]` block. The name in the heading IS the link.

4. **Create the proof file** at `<chapter>/proofs/notes/<proof-id>.tex`
   using the four-part structure (§6.5 Step 3):
   - Header comment (theorem name + source file path)
   - `\subsection*{...}\label{prf:root-id}`
   - `\begin{remark}[Return]` with `\hyperref[prop:root-id]` back-link
   - `\Claim [statement.]` as the first line of the proof body
   - `\begin{proof}...\end{proof}` in **normal mode** (§6.6)
   - `\begin{remark}[Proof shape]` and `\begin{remark}[Dependencies]`

5. **Update `<chapter>/proofs/notes/index.tex`** (§6.5 Step 4):
   Add `\input{<full-path-from-root>/<proof-id>}` in dependency order.

6. **Mark `proofs-to-do.md`:**
   Change `· ` to `✓ ` in the Status column for this entry.

7. **Compile and verify** — `lualatex -interaction=nonstopmode main.tex` —
   confirm no new errors and the page count is sensible.

---

### 7.2 `add justified proof for <theorem-name>`

**What it means:** Create a thorough-mode proof file using the three-column
tag/step/justification table, with a full Study Notes section at the end
explaining where each tool appears and why. All nav links and index updates
apply identically to §7.1.

**When to use:** When explicitly requested, or for proofs where every logical
step must be visibly justified — Bolzano–Weierstrass, Archimedean property,
ε-characterisations, construction-level arguments, and any proof William
specifically wants to study in detail.

**Differences from standard:**

The proof body uses the three-column `{T S J}` table format (§6.6 Thorough Mode)
instead of prose. The file additionally contains a **Study Notes** section after
the proof:

```latex
\noindent\hrule
\medskip
\noindent\textbf{Study Notes.}
\medskip
\noindent\textit{Where did each tool appear?}
\medskip

\noindent
\begin{tabular}{p{0.25\textwidth} p{0.65\textwidth}}
\toprule
\textbf{Tool} & \textbf{Role in this proof} \\
\midrule
\addlinespace[4pt]
[Definition / axiom / theorem used]
&
[One or two sentences: what it contributed and why it was needed at that point.]
\\[8pt]
\bottomrule
\end{tabular}
```

The proof file header comment should note `% Mode: justified` so the mode
is visible without reading the body.

Multi-part proofs use `\noindent\textit{Part N: ...}` section headers between
table blocks. Sub-arguments (contradiction, case splits) use the `subproof`
mdframed environment from `sample-theorem.tex`.

Tag legend (include at the top of the proof, after the return remark):
```latex
{\small
  \tagDU\ = Definition Unpacked \quad
  \tagTA\ = Theorem Applied \quad
  \tagAM\ = Algebraic Manipulation \quad
  \tagIH\ = Inductive Hypothesis \quad
  \tagBC\ = Base Case \quad
  \tagCS\ = Case Split
}
```
Only list the tags that actually appear in the proof.

All other steps (label, nav remark, index update, proofs-to-do update,
compile check) are identical to §7.1.

---

### 7.3 `convert proof standard <theorem-name>`

**What it means:** Replace an existing justified proof file with a normal-mode
version of the same proof. Used after William has studied and internalised a
proof and wants to reclaim space in the compiled document.

**When to use:** Only on request. Never convert proofs automatically.

**Full procedure:**

1. **Read the existing justified proof file** in full.
   Extract: the theorem statement, the logical structure of the proof
   (all the steps and their justifications), and the Dependencies remark.

2. **Write the normal-mode proof** (§6.6 Normal Mode) capturing the same
   logical content in prose, using house macros (`\Given`, `\Goal`,
   `\Strategy`, `\ByThm{}`, `\AsReq`, etc.).
   The prose must preserve every logical dependency — nothing may be silently
   dropped just because the table column is gone.

3. **Overwrite the existing proof file** with the new normal-mode content.
   Keep the file name, the `\subsection*` heading, the `\label{prf:...}`,
   and the return remark unchanged — only the proof body changes.

4. **Remove the Study Notes section** if present (it belongs to justified mode).
   Keep `\begin{remark}[Proof shape]` and `\begin{remark}[Dependencies]`.

5. **Update the header comment** from `% Mode: justified` to `% Mode: standard`.

6. **Do not touch** the notes file (labels and nav links stay identical),
   the index (the `\input` line is unchanged), or `proofs-to-do.md`
   (status stays `✓`).

7. **Compile and verify** — confirm no new errors.

---

### 7.4 Session Strategy

`proofs-to-do.md` is the canonical work list. Work through it in order.

**First pass per chapter:** complete every Easy proof in the chapter before
starting Medium or Hard proofs in that chapter. This ensures the proof
infrastructure (labels, nav links, index entries) is in place before tackling
results that depend on the easy ones.

**After the first pass:** one proof at a time, in the dependency order given
by the row numbers in `proofs-to-do.md`.

**Tracking:** after each proof is completed, update the Status column in
`proofs-to-do.md` from `·` to `✓` before ending the session.

**The `→` status** marks a proof that was started but not finished (e.g.
the proof file exists but has no content, or a compile error was left
unresolved). Use it to flag interrupted work.

---

### 7.5 Proof File Rules — Prohibited Environments and Naming

Two hard rules govern the content of every proof body.

**Rule PB-1 — No theorem-like environments in a generated proof file.**

The environments `\begin{proposition}`, `\begin{theorem}`, `\begin{lemma}`,
`\begin{corollary}`, `\begin{definition}`, and `\begin{axiom}` must **never**
appear in a proof file. Every such environment participates in the global counter
for its type, so placing one inside a proof inserts a spurious numbered item into
the document's theorem sequence.

The claim being proved is stated using `\Claim` (inline bold marker) as the
**first line of `\begin{proof}`**, with the statement copied verbatim from the
notes. Use `\begin{claim}...\end{claim}` (auto-numbered, `\label`-able) for any
formal sub-result within a multi-part proof. Never use `\begin{proposition}` or
any sibling environment for either purpose.

**Rule PB-2 — Use proper names, not shorthand codes, when citing axioms and
theorems.**

Inside a proof body, every citation of a prior result must use the result's
**full descriptive name**, not a code like `P1`, `P2`, `ax:zf-pairing`,
or any other symbol that is opaque without the surrounding document context.

- **Wrong:** `\ByThm{P2} applied to $0$: $0\pp \in \mathbb{N}$.`
- **Right:** `\ByThm{Closure under successor} applied to $0$: $0\pp \in \mathbb{N}$.`

- **Wrong:** `By the Axiom of Pairing, \dots`
- **Right:** `\ByThm{Pairing} \dots` with the full name "Pairing" matching the
  axiom's heading.

The name in `\ByThm{...}` must be the name as it appears in the notes
(the heading of the definition, axiom, proposition, or theorem box). Do not
invent abbreviations. If the notes give both a code (e.g.\ P1) and a
description (e.g.\ "zero is a natural number"), use the **description**.

This rule applies to the proof body and to all remarks in the proof file. Do
not use codes in `[Proof shape]` or `[Dependencies]` remarks either.

---

### 7.6 The `\Claim` / `\begin{claim}` Distinction

Two separate tools exist for claims:

| Tool | Form | Use |
|------|------|-----|
| `\Claim` | `\newcommand` — inline bold "Claim." | Inline assertion in prose mid-paragraph |
| `\begin{claim}...\end{claim}` | `\newenvironment` — auto-numbered, `\label`-able | Formal sub-result within a multi-claim proof |

Use `\begin{claim}` whenever the claim will be referred back to with `\cref`.
Use `\Claim` for a one-off assertion that needs no back-reference.
Never use `\begin{claim}` and `\Claim` for the same logical statement.
