# Refactor Prompt — Notes Restructure for Learning Abstract Mathematics

Use this prompt verbatim to start a new chat session when refactoring a chapter.
Replace the bracketed placeholders with the actual values for the chapter you are working on.

---

## THE PROMPT

I am working on a LaTeX project called **Learning Abstract Mathematics** in Overleaf.
I am going to upload a zip of the project. I need you to refactor the notes for one chapter
following a documented house style. Everything you need to know is described below.

---

### 1. The project zip

I will upload `Learning-Real-Analysis-refactored.zip`. The project root is `Learning-Real-Analysis-refactored/`.
Unzip it at the start and work from `/home/claude/Learning-Real-Analysis-refactored/`.

---

### 2. The chapter to refactor

**Chapter path:** `[e.g. volume-i/predicate-logic/]`
**Notes file(s) to refactor:** `[e.g. notes.tex — a single monolithic file]`

The existing notes file(s) use flat `\subsection` / `\subsubsection` headings with bare
`\begin{definition}`, `\begin{theorem}`, `\begin{lemma}` etc. amsthm environments and no boxes.

---

### 3. What the refactor does

#### 3a. Split into subject subfolders

Read the existing notes file(s) completely first. Then group the `\subsection` blocks into
logical subject subfolders under `notes/`. Each subfolder gets an `index.tex` orchestrator
that `\input`s its files in dependency order.

The propositional-logic chapter (already done) used this grouping as a reference model:
```
notes/
  syntax/          ← language primitives, formation rules, structure
  semantics/       ← truth, models, validity, equivalence, normal forms
  proof-theory/    ← inference rules, proof procedures, derivability
  metatheory/      ← completeness theorems, compactness, major named results
  reference/       ← errors/fallacies, summary tables
```
Choose groupings appropriate to the chapter content. 3–6 subfolders is typical.
Each subfolder should have 2–4 files. A file covers one cohesive topic (one `\subsection`).

The top-level `notes/index.tex` orchestrates the subfolders:
```latex
\input{<chapter-path>/notes/syntax/index}
\input{<chapter-path>/notes/semantics/index}
...
```

#### 3b. Add boxes according to the house style rules

**The four box types — colours already defined in `main.tex`:**

| Box | colback | colframe | Use |
|-----|---------|----------|-----|
| Definition | `propbox` | `propborder` | Blue — every definition, on first appearance only |
| Axiom | `axiombox` | `axiomborder` | Violet — unproven primitives, axiom systems |
| Theorem | `thmbox` | `thmborder` | Amber — major *named* results only |
| Toolkit | `gray!6` | `gray!40` | Gray — one summary table per file, at the top |

**Box template (same for all four — only colours differ):**
```latex
\begin{tcolorbox}[colback=propbox, colframe=propborder, arc=2pt,
  left=6pt, right=6pt, top=4pt, bottom=4pt,
  title={\small\textbf{Definition (Vector Space)}},
  fonttitle=\small\bfseries]
\label{def:vector-space}
...content...
\end{tcolorbox}
```

**Rule R1 — What gets a box:**
- `DEFINITION` box: every definition on first appearance. A definition is any statement
  of the form "X is defined as / called / denoted Y." Includes recursive definitions,
  notational conventions, operator definitions.
- `AXIOM` box: every axiom — unproven primitives. One box holds the full list when an
  axiom system is stated together.
- `THEOREM` box: only when (a) the theorem has a proper name AND (b) it is the primary
  result of its section and will be cited by name throughout. Examples: Compactness
  Theorem, Craig's Interpolation, Heine-Borel, Gödel's Completeness.
- `TOOLKIT` box: one per notes file, always at the top, never mid-section. A 3–4 column
  summary table with a `\hyperref` detail link column pointing to the entries below.

**Rule R2 — What never gets a box:**
Lemmas, propositions, corollaries, remarks, examples, proofs — all bare amsthm.
Second appearances of a definition — just cite by number.

**Rule R3 — The Import Test:**
Ask: "Is this the statement someone opening to this section needs to immediately understand?"
If yes → box it. If it follows from boxed material → bare amsthm.

Anti-pattern to avoid: boxing every individual theorem when a section lists many related
results (e.g. quantifier laws, equivalence laws). These belong in the toolkit table,
with individual bare propositions below for formal statements.

#### 3c. Toolkit + detail structure

Every notes file follows this exact two-layer pattern:

**Layer 1 — Toolkit box at top:**
```latex
\begin{tcolorbox}[colback=gray!6, colframe=gray!40, arc=2pt,
  left=6pt, right=6pt, top=4pt, bottom=4pt,
  title={\small\textbf{[Topic] — Quick Reference}},
  fonttitle=\small\bfseries]
\small
\begin{tabular}{l l l l}
\toprule
\textbf{Label} & \textbf{Statement} & \textbf{Proof method} & \textbf{Detail} \\
\midrule
[row] & [statement] & [method] & \hyperref[label]{↓ Def} \\
\bottomrule
\end{tabular}
\end{tcolorbox}
```

Toolkit table columns depend on the topic type:
- For **definitions**: Concept | English meaning | Detail link
- For **results**: Label | Statement | Proof method | Detail link
- For **rules**: Rule name | Premises | Conclusion | Detail link

**Layer 2 — Detailed entries below the toolkit:**

Each entry the toolkit links to is a bare environment (`\begin{definition}`, `\begin{lemma}`,
`\begin{proposition}`, etc.) with a `\label` matching the `\hyperref` in the toolkit,
followed by remark blocks. Use whichever remarks are warranted:

```latex
\begin{definition}[Name]\label{def:name}
...formal statement...
\end{definition}

\begin{remark}[English reading]
Plain-language meaning. What is this concept *doing*?
\end{remark}

\begin{remark}[Fully quantified form]
The statement with all quantifiers written out explicitly.
\end{remark}

\begin{remark}[Proof strategy]   % only for results, not definitions
Which technique and why that variable/approach was chosen.
\end{remark}

\begin{remark}[Consequence]
What does this unlock? What later result depends on it?
\end{remark}

\begin{remark}[Common error]
A specific mistake to avoid. Only include if there is a genuine trap.
\end{remark}

\begin{remark}[Source note]
How the name/numbering differs across sources. Only if sources diverge.
\end{remark}
```

---

### 4. Quality bar

The refactored files must be **at least as good** as the existing content — all topics
accounted for. Run a content audit before deleting old files:
- List every `\subsection` and `\subsubsection` from the old file
- Confirm each appears in the new structure
- Only delete old files after confirming 100% coverage

Also: after writing each file, scan for malformed LaTeX tags (a common issue is `\end{remark>`
with `>` instead of `}`). Fix any found before proceeding.

---

### 5. Reference examples already in the project

The **propositional-logic** chapter (`volume-i/propositional-logic/`) is the completed
reference. Study it before starting:
- `notes/syntax/notes-syntax.tex` — example of a definitions-heavy file
- `notes/semantics/notes-equivalences.tex` — example of toolkit-replaces-many-theorems pattern
- `notes/proof-theory/notes-inference-rules.tex` — example of rules-style toolkit
- `notes/metatheory/notes-compactness.tex` — example of a theorem-boxed metatheory file

The **axiom-systems** chapter (`volume-i/axiom-systems/notes/`) also uses this structure
(peano/, addition/, multiplication/ subfolders) and is worth reading.

---

### 6. What NOT to touch

- Do not modify any files outside `[chapter-path]/notes/`
- Do not modify `main.tex` — the `axiombox`, `axiomborder`, `thmbox`, `thmborder` colors
  are already defined there from a previous session
- Do not modify any proof files
- Do not touch any other chapter

---

### 7. Deliverable

When done:
1. All new files written and tag-checked
2. Content audit passed (all old topics present in new structure)
3. Old flat notes file(s) deleted
4. Package the project as `Learning-Real-Analysis-refactored.zip` and provide for download

---

### 8. Chapter-specific notes

**[Fill this in when you use the prompt. Examples:]**

For `predicate-logic`, the natural subject groupings are likely:
- `syntax/` — terms, formulas, free/bound variables, substitution, formula complexity
- `semantics/` — structures/interpretations, satisfaction, models/theories, substitution lemmas
- `quantifiers/` — universal/existential, unique existential, bounded, negation, commutation,
  prenex normal form
- `proof-theory/` — quantifier inference rules, equality rules, soundness/completeness
- `translation/` — English-to-logic, scope ambiguity, square of opposition
- `reference/` — errors/fallacies, summary tables

For other chapters, read the existing notes file first and propose the groupings before
writing any files. Wait for confirmation before proceeding.

---

*This prompt was generated from the propositional-logic refactor session (March 2026).*
*Reference: `DESIGN.md` and `box-vocabulary.html` in the project root.*
