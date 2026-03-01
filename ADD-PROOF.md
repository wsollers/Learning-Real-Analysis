# ADD-PROOF.md
## Prompt: Adding a Proof Stub to This Project

Use this document as a complete specification when adding one or more proof stubs. Read it in full before writing any files.

---

## 0. What You Are Doing

A **proof stub** is a single `.tex` file containing:
- A structured header (source, goal, logical form, proof strategy)
- A `proof` environment that may be complete, partial, or contain only a skeleton with comments
- A remarks block

Every stub must also be registered in two places: the chapter **proofs index** and the chapter **worksheet**.

---

## 1. Locate the Right Chapter

The project has three volumes. Map the source material to the correct chapter folder:

```
volume-i/
  propositional-logic/
  predicate-logic/
  sets-relations-functions/     ← set theory, order theory, relations, functions
  axiom-systems/
  proof-techniques/

volume-ii/
  natural-numbers/
  integers/
  rationals/
  reals/
  complex/

volume-iii/
  algebra/
    algebraic-structures/
    linear-algebra/             ← Axler and similar
    abstract-algebra/           ← Gallian and similar
  analysis/
    real-analysis/
    metric-spaces/
    point-set-topology/
```

Inside every chapter the proof infrastructure is:

```
<chapter>/
  proofs/
    index.tex                   ← master \input list + worksheets section
    <source>/                   ← one subfolder per source author/text
      <ID>.tex                  ← one proof per file
    worksheets/
      <source>.tex              ← one worksheet per source
```

---

## 2. Choose the Proof ID

IDs follow the pattern:

```
<SUBJ>-<SRC>-C<ch>[-S<sec>]-<TYPE><num>[a|b|…]
```

| Segment | Meaning |
|---------|---------|
| `SUBJ` | Chapter subject abbreviation (see table below) |
| `SRC`  | Source abbreviation (see table below) |
| `C<ch>` | Chapter number in the source, zero-padded to 2 digits: `C01`, `C07` |
| `S<sec>` | Section within chapter, if needed: `S2.4`, `S6.5` (omit if not useful) |
| `E<num>` | Exercise number |
| `X<num>` | Example number (Axler-style `X1.35`) |
| `P<num>` | Proposition/theorem number |
| `a`,`b`… | Sub-part suffix when a single exercise has labelled parts |

**Subject abbreviations:**

| Code | Chapter |
|------|---------|
| `PL` | Propositional Logic |
| `PC` | Predicate Calculus / Predicate Logic |
| `SRF` | Sets, Relations, Functions |
| `AS` | Axiom Systems |
| `PT` | Proof Techniques |
| `RA` | Real Analysis / Natural Numbers / Integers / Reals |
| `LA` | Linear Algebra |
| `AA` | Abstract Algebra |
| `MS` | Metric Spaces |

**Source abbreviations in current use:**

| Code | Author / Text |
|------|--------------|
| `AXL` | Axler, *Linear Algebra Done Right* |
| `GAL` | Gallian, *Contemporary Abstract Algebra* |
| `TAO` | Tao, *Analysis I* |
| `ABB` | Abbott, *Understanding Analysis* |
| `ROS` | Ross, *Elementary Analysis* |
| `MEN` | Mendelson, *Number Systems and the Foundations of Analysis* |
| `STO` | Stoll, *Set Theory and Logic* |
| `GER` | Gerstein, *Introduction to Mathematical Structures and Proofs* |
| `HAL` | Halmos, *Naive Set Theory* |
| `SUP` | Suppes, *Axiomatic Set Theory* |
| `KF`  | Kolmogorov & Fomin, *Introductory Real Analysis* |
| `DEA` | Dean, *Order Theory* (Brown lecture notes) |
| `CHI` | Chiossi, *Essential Mathematics for Undergraduates* |
| `BJO` | Bjørndahl (predicate logic notes) |

For a new source not in this list: choose a 3-letter code, add it here, and add a bib entry to `bibliography/analysis.bib`.

**Examples:**

```
SRF-STO-C07-S7.4        Sets/Relations/Functions, Stoll, Ch 7, §7.4
RA-MEN-C02-S2.4-E9a     Real Analysis, Mendelson, Ch 2, §2.4, Ex 9(a)
LA-AXL-C01-1C-X1.35     Linear Algebra, Axler, Ch 1, §1C, Example 1.35
SRF-DEA-C08-E29         SRF, Dean, Ch 8, Ex 29
AA-GAL-C07-E19          Abstract Algebra, Gallian, Ch 7, Ex 19
```

---

## 3. Create the Proof File

Create the file at:
```
<chapter>/proofs/<source>/<ID>.tex
```

Create the `<source>/` subfolder if it does not exist yet.

### 3a. Standard Format (Volume I and II; most of Volume III)

Used by: all SRF proofs, all RA proofs, all AA proofs, algebraic structures, metric spaces, etc.

```latex
% --------------------------------------
% Proof: <ID>
% --------------------------------------
\clearpage
\phantomsection
\hypertarget{proof-<ID>}{}

\subsubsection[<Short title for TOC>]{Proof --- <ID>}

\bigskip

\noindent
\textbf{Source.}
\srccite{<BibKey>}{Chapter~N, \SN, Exercise~N}.

\vspace{0.75em}

\noindent
\textbf{Goal.}
<One or two sentences stating exactly what is to be proved.>

\vspace{0.75em}

\noindent
\textbf{Logical form.}
<The statement written out with all quantifiers explicit, in symbolic notation.>

\vspace{0.75em}

\noindent
\textbf{Proof strategy.}
<Two to four sentences: which technique (direct, contradiction, induction on what,
construction, Zorn, etc.) and why it works here. Name the key lemmas or facts to invoke.>

\vspace{0.75em}

\noindent
\textbf{Proof.}
\begin{proof}
<Proof body. Use % comments for steps not yet filled in.>
\end{proof}

\vspace{0.75em}

\noindent
\textbf{Remarks.}
\begin{itemize}
  \item \textbf{<Tag>.} <Remark content.>
\end{itemize}
```

The `\subsubsection` short title (bracket argument) appears in the PDF table of contents and should be a compact noun phrase: `Uniqueness of Greatest Element`, `Principle of Transfinite Induction`, `Every Partial Order Extends to a Linear Order`.

**Optional extra fields** — insert between Goal and Logical form when useful:

```latex
\noindent
\textbf{Definition.}
<A key definition from the source that the reader needs to have in view.>

\vspace{0.75em}

\noindent
\textbf{Key background.}
<A theorem or result from earlier in the notes that this proof depends on,
with a \hyperref link if available.>

\vspace{0.75em}

\noindent
\textbf{Note.}
<Cross-reference to a related proof or notes section.>
```

### 3b. Linear Algebra Format (Volume III / linear-algebra only)

Used by: all files in `volume-iii/algebra/linear-algebra/proofs/axler/`.

```latex
% --------------------------------------
% Proof: <ID>
% --------------------------------------
\clearpage
\phantomsection
\hypertarget{proof-<ID>}{}
\section*{Proof — <ID>}

\noindent
\hyperlink{ws-<ID>}{\textbf{← Back to worksheet}}

\vspace{0.5em}
\noindent
\textbf{Source.}
\srccite{<BibKey>}{Chapter~N, Section~NC, Exercise~N}.

\vspace{0.75em}
\noindent
\textbf{Goal.}
<Statement of what is to be proved.>

\vspace{0.75em}
\noindent
\begin{proof}
<Proof body.>
\end{proof}

\vspace{2cm}
\noindent
\hyperlink{ws-<ID>}{\textbf{← Back to worksheet}}
```

Note: this format uses `\section*` (not `\subsubsection`), has back-links at top and bottom, and does not include Logical form or Proof strategy fields in its current usage — though you may add them.

---

## 4. Register in the Proofs Index

Open `<chapter>/proofs/index.tex`. Add an `\input` line for the new proof in source-chapter-section order (keep each source's proofs grouped together). Place it before the `% Worksheets` comment block.

```latex
\input{<full-path-from-root>/<ID>}
```

**Example** (SRF chapter):
```latex
\input{volume-i/sets-relations-functions/proofs/dean/SRF-DEA-C08-E29}
```

If this is the first proof from a new source, also add a worksheet `\input` in the worksheets block at the bottom of the index:
```latex
\input{<chapter>/proofs/worksheets/<source>}
```

---

## 5. Register in the Worksheet

Open (or create) `<chapter>/proofs/worksheets/<source>.tex`.

### 5a. If the worksheet file already exists

Add a new table row in chapter-section-exercise order:

```latex
\phantomsection
\hypertarget{ws-<ID>}{}
\hyperlink{proof-<ID>}{\texttt{<ID>}}
&
<Ch. reference> --- <Short descriptor: what the exercise asks you to prove.>
\\ \hline
```

### 5b. If this is the first proof from a new source

Create the file from scratch:

```latex
\subsection{<Author Last Name>}

\noindent\textbf{Source.} \cite{<BibKey>}

\vspace{0.75em}
\begin{center}
\begin{tabular}{|p{5.0cm}|p{9.0cm}|}
\hline
\textbf{Problem ID} & \textbf{Exercise (descriptor)} \\
\hline
\phantomsection
\hypertarget{ws-<ID>}{}
\hyperlink{proof-<ID>}{\texttt{<ID>}}
&
<Ch. reference> --- <Short descriptor.>
\\ \hline
\end{tabular}
\end{center}
```

Then add `\input{<chapter>/proofs/worksheets/<source>}` to the worksheets block in the chapter's `proofs/index.tex`.

---

## 6. Add a Bibliography Entry (new sources only)

If the source does not already have a bib key in `bibliography/analysis.bib`, append an entry. Match the entry type to the source:

```bibtex
% Textbook:
@book{<Key>,
  author    = {Last, First},
  title     = {Full Title},
  edition   = {N},
  publisher = {Publisher Name},
  address   = {City},
  year      = {YYYY},
  isbn      = {978-...},
}

% Lecture notes / unpublished:
@unpublished{<Key>,
  author = {Last, First},
  title  = {Title of Notes},
  note   = {Lecture Notes, Institution Name},
  year   = {YYYY},
}

% Chapter in an edited volume:
@incollection{<Key>,
  author    = {Last, First},
  title     = {Chapter Title},
  booktitle = {Book Title},
  editor    = {Editor, Name},
  pages     = {N--M},
  publisher = {Publisher},
  year      = {YYYY},
}
```

Key naming convention: `AuthorShortTitleYear` — e.g. `Dean2015OrderTheory`, `Mendelson2008NumberSystems`, `Marker2000ModelTheory`.

---

## 7. Adding New Notes Files (when proof work reveals a gap)

If the proof requires a definition or theorem that is not yet in the notes, add a notes file first:

1. Identify the correct subfolder under `<chapter>/notes/`. Subfolders group related topics (e.g. `notes/order/`, `notes/zorn/`). Create a new subfolder if a topic has 3+ files or is conceptually distinct.
2. Create the notes file following the standard tcolorbox conventions in `DESIGN.md`.
3. Create or update the subfolder's `index.tex` to `\input` the new file.
4. Update the chapter's `notes/index.tex` to `\input` the subfolder index (in dependency order — prerequisites before dependents).

---

## 8. Checklist

Before finishing, confirm:

- [ ] Proof file created at the correct path with the correct ID
- [ ] ID used consistently: `\hypertarget{proof-<ID>}`, `\subsubsection{Proof --- <ID>}`, `\hyperlink{proof-<ID>}` in worksheet
- [ ] `\input` added to `proofs/index.tex` in the right position
- [ ] Worksheet row added with matching `\hypertarget{ws-<ID>}` and `\hyperlink{proof-<ID>}`
- [ ] If new source: worksheet file created, `\input{worksheets/<source>}` added to index, bib entry added to `bibliography/analysis.bib`
- [ ] If new notes subfolder: subfolder `index.tex` created, parent `notes/index.tex` updated

---

## 9. Remark Tags Reference

Use these in the `\textbf{<Tag>.}` pattern in the Remarks block:

| Tag | Use |
|-----|-----|
| `Note` | Cross-reference to related proof or notes definition |
| `Connection to …` | How this result relates to a named theorem or concept elsewhere in the project |
| `Mistakes to avoid` | A common error when applying or proving this result |
| `Capstone` | This result is the payoff of a longer development — name what it unlocks |
| `Proof strategy (alternative)` | A different valid approach worth knowing |
| `Source note` | How numbering or naming differs across the sources in the project |
| `Correction to naive statement` | When the exercise as stated needs precision before it is true |
