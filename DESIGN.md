# Learning Real Analysis House Constitution
## Working Draft

## Primary Rules

## A - Rule — Layer-Gated Notation Enforcement

Before editing or generating any formal block, classify the target block by role.

Allowed notation by role:

1. Definition/theorem/lemma/proposition/corollary/axiom body:
   standard mathematical notation only.

2. Standard quantified statement:
   standard mathematical notation only.

3. Negated quantified statement:
   standard mathematical notation only.

4. Contrapositive quantified statement:
   standard mathematical notation only.

5. Definition predicate reading:
   canonical predicates allowed and preferred.

6. Negation predicate reading:
   canonical predicates allowed and preferred.

7. Failure mode decomposition:
   canonical predicates allowed only if the block is explicitly serving a predicate-analysis role; otherwise use standard mathematical notation.

8. Interpretation:
   prose only; no formal predicate language unless explicitly discussed as notation.

## B - Rule — No Predicate Leakage

Predicate names from predicates.yaml must never appear in:
- definition bodies
- theorem/lemma/proposition/corollary/axiom bodies
- standard quantified statements
- negated quantified statements
- contrapositive quantified statements

unless the user explicitly asks for a predicate-language version.

## C - Rule — Rewrite Preflight Audit

Before returning a transformed file, check each formal block and report:

- BLOCK TYPE
- NOTATION FAMILY USED
- COMPLIANT / NONCOMPLIANT

If a standard quantified statement contains any \operatorname{...} predicate name from predicates.yaml, the output is invalid.
If a definition/theorem-like body contains any canonical predicate name, the output is invalid.

## 1. Purpose and Identity

These notes are a long-term mathematical reference, not a transcript of a course and not a workbook. They are written to preserve definitions, theorems, proof structures, dependencies, and canonical notation in a form that remains readable, rigorous, and stable across years of revision.

The document has four governing aims:

1. preserve mathematical precision,
2. preserve structural clarity,
3. preserve visual consistency,
4. preserve machine-readable semantic structure through canonical predicate systems

The notes are therefore governed by repository rules, notation rules, box rules, proof rules, logical-form rules, and figure rules.

---

## 2. Canonical Repository Structure

### 2.1 Top-Level Layout

```text
Learning-Real-Analysis/
  main.tex
  bibliography/
    analysis.bib
  common/
    preamble.tex
    colors.tex
    environments.tex
    macros.tex
    boxes.tex
  predicates.yaml
  notation.yaml
  relations.yaml
  proofs-to-do-analysis.md
  volume-i/
  volume-ii/
  volume-iii/
  volume-iv/
```

### 2.2 Volume Scope

- `volume-i/` contains logic, predicate calculus, sets, relations, functions, axiom systems, and proof techniques.
- `volume-ii/` contains natural numbers through complex numbers.
- `volume-iii/` contains analysis, metric spaces, topology, measure theory, algebra, and related proof-primary mathematics.
- `volume-iv/` contains applied and computational mathematics.

### 2.3 Card Extraction Rule

`volume-iv/` is excluded from card extraction by default.

### 2.4 Volume Chapter Registry

Each volume must declare its canonical chapter subjects in dependency order.

#### Rule — Every volume lists its chapter subjects

Each volume section in the constitution includes a chapter registry listing the canonical chapter subjects in dependency order.

#### Purpose

This registry serves four roles:

1. it fixes the intended mathematical spine of the volume,
2. it determines breadcrumb neighbors,
3. it defines valid chapter-generation targets,
4. it prevents drift in folder naming and chapter scope.

#### Rule — Registry order is structural

The listed order is dependency order, not alphabetical order.

#### Rule — Canonical chapter subjects are authoritative

A generated or refactored chapter must use a subject from the registry unless the registry itself is amended first.

#### Rule — Chapter subject names are repository identifiers, not display titles

Each chapter has:

1. a repository subject name used in paths and generation requests,
2. a display title used in the rendered document.

Example:

- repository subject: `limits-of-functions`
- display title: `Limits of Functions`

### 2.5 Canonical Chapter Structure

Every chapter follows this structure:

```text
<chapter>/
  index.tex
  chapter.yaml
  notes/
    index.tex
    <section>/
      notes-<section>.tex
      figure-<n>.tex
  proofs/
    index.tex
    notes/
      index.tex
      <proof-id>.tex
    exercises/
      index.tex
      <proof-id>.tex
      capstone-<chapter>.tex
```

### 2.6 Structural Rules

The root `main.tex` inputs volumes only.

The chapter `index.tex` is the only file that inputs `proofs/index.tex`.

Each `notes/<section>/` directory contains no local `index.tex`; the section file itself is the content unit.

`proofs/notes/index.tex` inputs proof files in dependency order.

`proofs/exercises/index.tex` inputs exercise proofs with the capstone last.

`figure-*.tex` files contain TikZ code only. They contain no document preamble and no inline color definitions.

Reading-list files are not compiled into the main document.

### 2.7 Stub Chapter Generation

A stub chapter is a formally created chapter whose structural files exist even when its notes and proofs are not yet written.

#### Rule — Stub generation is a first-class operation

Given a volume path and chapter subject, the system must be able to generate a stub chapter in canonical form.

Example request:

```text
generate stub chapter volume-iii analysis bounding
```

#### Rule — Stub chapter output

Generating a stub chapter creates the following minimum structure:

```text
<chapter>/
  index.tex
  chapter.yaml
  notes/
    index.tex
  proofs/
    index.tex
    notes/
      index.tex
    exercises/
      index.tex
      capstone-<chapter>.tex
```

If section folders are not yet known, no section subfolders are created.

#### Rule — Stub `index.tex` contents

The chapter `index.tex` for a stub chapter contains, in order:

1. breadcrumb box,
2. status box with `Status: Planned`,
3. structural roadmap,
4. placeholder `\input` chain only if section stubs already exist.

#### Rule — Stub breadcrumb neighbors come from the registry

The prior and next chapter names in the breadcrumb are taken from the chapter registry for that volume.

#### Rule — Stub roadmap is structural, not empty

A stub chapter may be unwritten, but its roadmap must still state:

- what the chapter is expected to formalize,
- what prior chapter it depends on,
- what later chapter it feeds into.

#### Rule — Stub chapter file names are canonical immediately

A stub chapter does not postpone naming discipline. Its folder name, capstone name, and future proof paths must already conform to house naming rules.

#### Rule — Stub generation does not invent mathematics silently

If section decomposition or chapter scope is unclear, the stub may contain placeholders, but it must not invent detailed theorem lists without being asked.

### 2.8 Capstone Rule

Every chapter has exactly one capstone exercise:

```text
proofs/exercises/capstone-<chapter>.tex
```

It appears last in `proofs/exercises/index.tex` and is tracked in `proofs-to-do-analysis.md`.

---

## 3. Canonical Notation Layer

### 3.1 Governing Principle

Notation in the notes is determined by `notation.yaml`, not by source texts. Source notation is never binding.
Rule — Long Logical Display Formatting

### 3.1.1 All multi-clause logical statements must use aligned environments.

Preferred breakpoints:
- implications
- conjunctions
- disjunctions
- 
### 3.2 Canonical Rewrite Rule

Every imported theorem, definition, proof, remark, or exercise is rewritten into house notation using standard mathematical concepts and terminology before it enters the notes.

### 3.2.1 Rule — Predicate Coverage Audit

During processing:

If a quantified logical structure appears that cannot be expressed using existing predicates:

DO NOT invent a predicate inline
ADD an audit entry:
Missing predicate: <description>
Location: <file + label>
Suggested form: <formal structure>

### 3.3 Canonical Role Assignment

Notation is assigned by mathematical role, not by local convenience.

The canonical notation layer is driven by `notation.yaml`.

### 3.4 Quantifier Standardization

Quantified forms must follow the canonical styles in `notation.yaml`. In particular, use forms such as

\[
\forall \varepsilon > 0 \; \exists N \in \mathbb{N} \; \forall n \ge N
\]

and not alternate local variants such as \(n > N\) when the house convention is \(n \ge N\).

### 3.5 Local Deviation Rule

Local deviation from canonical notation is permitted only by getting permission.

### 3.6 Predicate–Prose Agreement Rule

The symbols used in prose, logical blocks, and predicate blocks must agree with the canonical notation layer.

## Rule — Canonical Predicate, Relation, and Notation Sources

All predicate names, relation names, and canonical notation names used in theorem statements, predicate readings, negation readings, failure-mode decompositions, and other formal logical displays must come from the project’s canonical source files. This file is stored in the root of the repository or in project memory in ChatGPT.

The canonical sources are:

- `predicates.yaml`
- `relations.yaml`
- `notation.yaml`

For AI-assisted generation, these source files are the project memory.
Standardizer request files must transmit the applicable predicate catalog
instead of relying on unstated model memory. Predicate-reading blocks may
use a `reading_aliases` entry from `predicates.yaml` only when that alias is
registered on the canonical predicate.

No ad hoc predicate, relation, or notation name may be invented locally inside a notes file, theorem file, proof file, or remark block.

If a formal name is needed and it is not already present in the canonical source files, that is a hard audit failure. The name must first be added to the appropriate canonical source file before it is used anywhere in the notes.

All rendered formal names in logical displays must be written in their canonical form using operator-style typesetting:

```latex
\operatorname{Name}(\cdots)

## 4. Box Vocabulary

### 4.1 Governing Principle

Boxes mark what must be retained. Bare environments carry logical development.

### 4.2 Box Types

There are four box types:

- Definition box
- Axiom box
- Theorem box
- Toolkit box

The color palette is fixed in `common/colors.tex`.

### 4.3 Box Templates

Canonical box templates are fixed and are not modified locally.

### 4.4 Box–Environment Separation Rule

Every box wraps a single bare environment. A box must not contain ad hoc prose pretending to be a formal environment.

### 4.5 Density Rule

Boxes are intentionally scarce. Toolkit boxes do not count toward normal box density. Visual saturation is forbidden.

### 4.6 Visual Rhythm Rule

A section should ordinarily move through toolkit, first boxed definition, plain exposition, bare theorem or proposition, proof, and remarks. Boxing is structural, not decorative.

---

## 5. Chapter Entry Pattern

### 5.1 Required Opening Order

Every chapter `index.tex` opens with these elements in this order:

1. breadcrumb box,
2. structural roadmap.

### 5.2 Breadcrumb Title Rule

The title of the breadcrumb box is the chapter subject, not the word “Breadcrumb”.

### 5.3 Breadcrumb Scope Rule

The breadcrumb shows only the immediate conceptual neighborhood, not the full historical spine.

Example:

\[
\text{Limits of Functions} \;\to\; \text{Continuity} \;\to\; \textbf{Differentiation} \;\to\; \text{Integration}
\]

The full dependency structure belongs in the roadmap and in the graph files, not in the breadcrumb.

### 5.4 Breadcrumb Formatting Rule

Use `$\;\to\;$`. The current chapter is bolded. The breadcrumb must fit inside the box and may be split across at most two lines if needed.

### 5.5 Stub Chapter Rule

Stub chapters still carry a breadcrumb. A stub chapter inserts a **Status: Planned** box immediately after the breadcrumb.

---

Rule — Toolkit-Guided Decomposition of Bundled Concepts

Purpose.
To preserve conceptual coherence for the reader while ensuring that all formal content remains atomic and extractable for the knowledge graph.

1. Atomicity Requirement (Global)

Every definition or theorem-like environment (definition, theorem, lemma, proposition, corollary, axiom) must contain exactly one independently extractable mathematical item.

No environment may bundle multiple predicates, operations, relations, conditions, or named statements.

2. Detection of Bundled Content

An environment is non-compliant if it contains:

Multiple named concepts (e.g., “bounded above / below / bounded”)
Multiple operations or constructions (e.g., all pointwise operations in one definition)
Bullet lists or clauses that define distinct mathematical notions
Multiple statements that could each be given their own label
3. Required Repair Structure (MANDATORY)

When bundled content is encountered, it must be refactored using the following structure:

(a) Toolkit Box (Top of Section)

A single gray Toolkit box must appear at the top of the section representing the conceptual family.

The Toolkit box must:

Name the concept family (e.g., Bounding Toolkit, Limits Toolkit)
List the primary notions or tools in that family
Provide a structural overview (not formal definitions)
(b) Expository Block (Immediately After Toolkit)

A prose block must follow the Toolkit box explaining the concept at a high level.

The exposition must:

Describe the unifying idea of the concept family
Explain relationships informally
Prepare the reader for the formal decomposition

This block must be unboxed (or a remark* if required by house style).

(c) Atomic Formal Blocks (Following Exposition)

All formal content must be split into separate environments.

Each environment must:

Contain exactly one concept or statement
Have its own label
Be independently extractable
Contain no bundled definitions or clauses
4. Ordering Constraint

The required order is:

Toolkit Box
→ Exposition
→ Definition/Theorem Block
→ Definition/Theorem Block
→ ...

No formal block introducing a member of a concept family may appear before its Toolkit and exposition.

5. Non-Permissible Transformations

The following are not allowed:

Splitting a bundled definition into multiple environments without introducing or verifying a Toolkit
Leaving multiple concepts grouped under bullet points inside a single environment
Using prose inside a definition to implicitly define multiple notions
Creating atomic blocks without conceptual framing
6. Conceptual Consistency Requirement

All atomic items produced from a decomposition must correspond to entries in the canonical predicate, relation, or notation systems when applicable.

This ensures:

alignment with predicates.yaml
consistency in naming and extraction
stable knowledge graph nodes
7. Audit Rule

During compliance checks:

If a definition/theorem-like environment contains multiple independently nameable items:
Flag as Bundled Content Violation
Verify presence of appropriate Toolkit box
If absent, require insertion
Require insertion of exposition
Require full decomposition into atomic environments
8. Design Principle (Normative Statement)

Conceptual grouping belongs to Toolkit and exposition.
Formal environments must remain strictly atomic.

This rule is mandatory and applies globally across all volumes and chapters.
Addendum — Bundled Content Detection Criteria

Purpose.
To ensure that bundled mathematical content is detected reliably and uniformly, independent of formatting style, so that all formal environments remain atomic and extractable.

1. Trigger Condition (Formatting-Independent)

A bundled-content violation is triggered whenever a definition or theorem-like environment introduces more than one independently nameable mathematical item, regardless of how the content is formatted.

This applies independently of presentation, including but not limited to:

itemize or enumerate lists
align, aligned, or multi-line equation blocks
comma-separated definitions or assignments
multiple constructions defined in a single display
conditional clauses introducing additional notions
inline chains of definitions (e.g., “define …, …, and …”)
2. Formal Detection Rule

Let an environment E be a definition or theorem-like environment.
If the content of E can be decomposed into two or more statements S
1
	​

,S
2
	​

,…,S
n
	​

 such that each S
i
	​

 could be assigned:

its own name, and
its own label,
then E is non-compliant and must be decomposed.
3. Canonical Examples of Violations

The following patterns are always violations:

(a) Multiple predicates in one definition
$f$ is bounded above, bounded below, and bounded if ...
(b) Multiple operations defined together
(f+g)(x), (f-g)(x), (fg)(x), ...
(c) Bullet-list definitions
\begin{itemize}
\item ...
\item ...
\end{itemize}
(d) Multi-clause logical definitions
We define A if ..., and define B if ...
4. Non-Exceptions

There are no formatting-based exceptions to this rule.

In particular, the following do not exempt an environment from decomposition:

grouping for convenience
brevity of presentation
traditional textbook style
“closely related” concepts
use of shared notation
5. Required Action Upon Detection

When a bundled-content violation is detected:

Identify the concept family governing the items
Locate or create the corresponding Toolkit box
Insert or verify presence of expository block
Decompose the environment into atomic environments:
one item per environment
one label per item
no residual bundling
6. Design Principle (Reinforced)

Formatting must not obscure structure.
If multiple concepts exist, they must be made structurally explicit.

This addendum is mandatory and extends the global atomicity requirement to be strictly formatting-independent.


Rule — Bundled Content Detection Criteria

Purpose.
To ensure that bundled mathematical content is detected reliably and uniformly, independent of formatting style, so that all formal environments remain atomic and extractable.

1. Trigger Condition (Formatting-Independent)

A bundled-content violation is triggered whenever a definition or theorem-like environment introduces more than one independently nameable mathematical item, regardless of how the content is formatted.

This applies independently of presentation, including but not limited to:

itemize or enumerate lists
align, aligned, or other multi-line equation blocks
comma-separated definitions or assignments
multiple constructions defined in a single display
conditional clauses introducing additional notions
inline chains of definitions such as “define …, …, and …”
2. Formal Detection Rule

Let E be a definition or theorem-like environment. If the content of E can be decomposed into two or more statements S
1
	​

,S
2
	​

,…,S
n
	​

 such that each S
i
	​

 could be assigned:

its own name, and
its own label,

then E is non-compliant and must be decomposed.

3. Canonical Examples of Violations

The following patterns are always violations:

(a) Multiple predicates in one definition

$f$ is bounded above, bounded below, and bounded if ...

(b) Multiple operations defined together

(f+g)(x), (f-g)(x), (fg)(x), ...

(c) Bullet-list definitions

\begin{itemize}
\item ...
\item ...
\end{itemize}

(d) Multi-clause logical definitions

We define A if ..., and define B if ...
4. Non-Exceptions

There are no formatting-based exceptions to this rule.

In particular, the following do not exempt an environment from decomposition:

grouping for convenience
brevity of presentation
traditional textbook style
closely related concepts
use of shared notation
5. Required Action Upon Detection

When a bundled-content violation is detected:

identify the concept family governing the items,
locate or create the corresponding Toolkit box,
insert or verify presence of expository block,
decompose the environment into atomic environments:
one item per environment,
one label per item,
no residual bundling.
6. Design Principle

Formatting must not obscure structure.
If multiple concepts exist, they must be made structurally explicit.

Rule — Toolkit Must Enumerate the Atomic Family

When bundled content is decomposed under a Toolkit box, the Toolkit must enumerate exactly the atomic items that follow, either explicitly or by unmistakable grouped reference.

Requirements

The Toolkit must:

name the governing concept family,
list the atomic notions or results that will be formalized below,
provide structural orientation without itself serving as a formal definition or theorem.
Consequence

The Toolkit functions as the conceptual hub for the atomic sequence that follows. It is not decorative and it must correspond to the formal decomposition beneath it.


## 6. Governing Box Rules

### Rule R1 — What Gets a Box

Definitions are always boxed on first appearance only.

Axioms are always boxed.

Theorems are boxed only when they have a proper name, are the primary result of the section, and will be cited later by name.

Each section has exactly one toolkit box at the top.

### Rule R2 — What Never Gets a Box

Lemmas, propositions, corollaries, remarks, examples, proofs, and second appearances never receive boxes.

### Rule R3 — Import Test

A statement receives a box only if it is the statement a reader opening the section must immediately identify and retain.

### Rule R4 — Scarcity

At most one structurally dominant boxed theorem ordinarily appears in a section.

### Rule R5 — Breadcrumb Prose

Breadcrumb prose states dependency, chapter construction, and forward consequence only. It does not motivate emotionally and does not manage reader expectation.

---

## 7. Exposition Voice and Register

### 7.1 Reader Model

The notes are written for one serious reader who has studied logic, set theory, and the construction of \(\mathbb{N}\) through \(\mathbb{R}\), reads primary sources in parallel, and expects the notes to remain self-contained and stable over long reuse.

### 7.2 Voice Rule

First person and second person are absent. The notes are written as an authoritative record.

Forbidden examples:
- “we will show”
- “you should notice”
- “let us recall”

Preferred forms:
- “The argument proceeds…”
- “The critical observation is…”
- “This follows from…”

### 7.3 Explanation Rule

Explain everything non-obvious. Explain nothing obvious.

### 7.4 Required Expository Content

Each expository block should carry, when applicable:

1. the precise mathematical fact,
2. why it is true,
3. why it matters,
4. its standard failure mode,
5. its structural or geometric picture.

### 7.5 Figure Rule

Every figure has a self-contained caption naming all objects, transformations, and the conclusion illustrated.

All figures live in `figure-<n>.tex` and are `\input`'d. TikZ is never embedded inline in notes files.

### 7.6 Remark Register Rule

Remarks are post-mastery mathematical prose. They do not apologize for length, acknowledge difficulty, or coach reaction.

---

## 8. Label Conventions

### 8.1 Mandatory Labels for Formal Environments

Every occurrence of the following environments must carry a label:

- theorem
- lemma
- proposition
- corollary
- definition
- axiom

A theorem-like environment is not considered complete until its label is present.

Unlabeled theorem-like environments are forbidden in notes files.

### 8.2 Prefixes

- theorem: `thm:`
- lemma: `lem:`
- proposition: `prop:`
- corollary: `cor:`
- definition: `def:`
- axiom: `ax:`
- proof file: `prf:`

### 8.3 Placement Rule

The label appears inside the environment immediately after it begins.

Exception: For proof files governed by Rule 9.5.1, the label must appear at the top of the file, outside and prior to any environments, to ensure proper anchor placement for \phantomsection.

### 8.4 Naming Rule

Labels are lowercase, semantic, hyphen-separated, and globally unique.

### 8.5 Proof Alignment Rule

If the statement label is

```latex
\label{prop:convergent-implies-cauchy}
```

then the proof label must be

```latex
\label{prf:convergent-implies-cauchy}
```

and the proof filename must use the same root.

### 8.6 One-Statement–One-Label Rule

Each theorem-like object has exactly one label.

### 8.7 Proof Creation Dependency Rule

No proof file may be created for a statement unless the corresponding statement label already exists in the notes file.

---

## 9. Proof Architecture

### 9.1 Directory Structure

```text
proofs/
  index.tex
  notes/
    index.tex
    <proof-id>.tex
  exercises/
    index.tex
    <proof-id>.tex
    capstone-<chapter>.tex
```

### 9.2 Proof Classification

A notes proof is permanent when it introduces a recurring technique, is load-bearing, or has structure worth revisiting.

Exercise proofs are transient practice and may later be removed.

### 9.3 File Naming Rule

Proof filenames are lowercase, hyphen-separated, ASCII only, and free of LaTeX markup.

### 9.4 New-Page Rule

Every notes proof file begins on a new page.

### Rule 9.5 — Proof File Architecture
Every proof file must follow a standardized structure to ensure consistency across the digital library, proper hyperlinking, and a dual-layered pedagogical approach. The content must be ordered as follows:

Canonical Header:

\newpage

\phantomsection

The proof-level label: \label{prf:theorem-id}

Return Block: A remark* environment containing: \hyperref[thm:theorem-id]{Return to Theorem}

Theorem Restatement: The theorem or proposition statement wrapped in an unnumbered environment (e.g., \begin{theorem*} ... \end{theorem*}).

The Professional Standard Proof: A proof environment containing the compact, rigorous mathematical argument, beginning with the bold header Professional Standard Proof..

The Detailed Learning Proof: A second proof environment beginning with the bold header Detailed Learning Proof. This section uses bold inline step headings (e.g., Step 1.) to decompose the logic for the reader.

Proof Structure Remark: A remark* titled Proof structure summarizing the high-level strategy (e.g., direct, contradiction, induction).

Dependencies Remark: A remark* titled Dependencies containing a list of \hyperref links to all definitions, axioms, or lemmas utilized in the proof.

Canonical Template for Proof Files
Code snippet
\newpage
\phantomsection
\label{prf:theorem-id}

\begin{remark*}[Return]
\hyperref[thm:theorem-id]{Return to Theorem}
\end{remark*}

\begin{theorem*}[Theorem Name]
Statement of the theorem goes here.
\end{theorem*}

\begin{proof}
\textbf{Professional Standard Proof.}~\\
Text of the compact, rigorous proof.
\end{proof}

\begin{proof}
\textbf{Detailed Learning Proof.}~\\

\textbf{Step 1.} First logical milestone.
Detailed explanation...

\textbf{Step 2.} Second logical milestone.
Detailed explanation...
\end{proof}

\begin{remark*}[Proof structure]
Description of the overall strategy.
\end{remark*}

\begin{remark*}[Dependencies]~\\
\begin{itemize}
  \item \hyperref[def:source1]{Definition of X}
  \item \hyperref[thm:source2]{Theorem Y}
\end{itemize}
\end{remark*}

### 9.6 Macro Restriction Rule

No flash macros are used in proof files. No proof-structuring macros are used. Only ordinary environments and hyperlinks are allowed.

### 9.7 Two-Layer Proof Rule

The professional proof is the compact mathematical proof.

The detailed learning proof exists to teach the professional proof.

### 9.8 Step Rule for Learning Proofs

The detailed proof uses inline bold step headings only:

```latex
\textbf{Step 1.} ...
```

No step macros and no separate remark environments organize the steps.

### 9.9 Explanationless Proof Mode

Explanationless proof mode is opt-in only. It occurs only when explicitly requested with phrases such as “proof only”, “no explanation”, “professional proof only”, or “compact proof”.

### 9.10 Discipline Preservation Rule

Even when explanation is omitted, the proof still obeys house notation, label conventions, proof architecture, macro restrictions, and voice rules.



### 9.11 — Proof Files Must Use Unnumbered Theorem Environments

    In proof files (files whose purpose is to contain proofs, e.g. under `proofs/`), any restatement of a theorem must use the unnumbered environment:
    
    \begin{theorem*} ... \end{theorem*}
    
    and must NOT use a numbered `theorem` environment.
    
    #### Requirements
    
    - The proof file version of a theorem is a **restatement**, not a new theorem.
    - Therefore it must:
      - use `\begin{theorem*}` (unnumbered)
      - retain the original theorem name
      - NOT introduce numbering or new theorem identities
    
    #### Label Restriction (Strict)
    
    - Proof files must **never contain `\label{...}` inside theorem environments**.
    - Labels belong only to the canonical theorem in the notes file.
    
    #### Rationale
    
    - Prevents duplicate numbering across notes and proofs.
    - Enforces that the canonical statement lives in the notes, not the proof file.
    - Prevents cross-reference collisions and accidental label shadowing.
    - Keeps proofs as secondary artifacts attached to primary theorem declarations.
    
    #### Example
    
    ❌ Incorrect (creates duplicate numbering and label collision)
    ```latex
    \begin{theorem}[Least Upper Bound Property Implies Existence of Suprema]
    \label{prf:lub-property-implies-existence-of-suprema}
    ...
    \end{theorem}
    ---

## 10. Logical Form Blocks

### 10.1 Atomicity Rule

Each mathematically distinct term, condition, or assertion receives its own environment.

Paired notions such as upper bound and lower bound, bounded above and bounded below, or monotone increasing and monotone decreasing are not bundled into one environment merely because they are naturally related.
### 10.1.1 Mathematical Standards in Definition and Theorem like Environments

Definition, lemma, corollary, theorem, proposition, axiom, should only contain standard 
mathematical notation. 

House predicates are reserved soley for the remark blocks:
1. Definition predicate reading
2. Negation predicate reading
3. Failure mode decomposition
All other blocks use standard notation.

### 10.2 Required Logical Blocks

Every definition, axiom, theorem, lemma, proposition, corollary is followed immediately by these blocks, in order:

1. Standard quantified statement
2. Definition predicate reading
3. Negated quantified statement, when negation mathematically illuminating
4. Negation predicate reading, If a negated quantification was generated
5. Failure modes, when applicable
6. Failure mode decomposition, if failure modes were generated
7. Contrapositive quantified statement when mathematically illuminating
8. Interpretation block - always
9. Dependencies block - always

A predicate-reading block is included only when a canonical predicate exists or clearly deserves to exist. It need not be forced for one-off statements whose quantified form is already structurally transparent.

### 10.3 Definition Notation-Binding Rule

If a definition introduces notation, that notation appears in the definition itself and may then be used in the logical blocks.

### 10.4 Dependencies Block Rule

Every definition, axiom, theorem, lemma, proposition, and corollary carries an attached `remark*` block titled **Dependencies** after its Interpretation block. This block is extraction infrastructure, not exposition.

If the item depends on earlier formal items, the block lists those items using explicit `\hyperref[...]` links to their labels:

```latex
\begin{remark*}[Dependencies]\hfill
\begin{itemize}
  \item \hyperref[def:limit-function]{Limit of a Function}
  \item \hyperref[def:cluster-point-R]{Cluster Point}
\end{itemize}
\end{remark*}
```

If the item is foundational within the current extraction scope, the block states:

```latex
\begin{remark*}[Dependencies]
No local dependencies.
\end{remark*}
```

Dependency labels must point to formal mathematical items such as definitions, axioms, lemmas, propositions, corollaries, and theorems. They must not point to proof labels. The extraction convention is:

```text
dependency label -> current item label
```

For downstream graph extraction, optional attached relationship blocks may also be used after Dependencies:

- `\begin{remark*}[Consequences] ... \end{remark*}` for direct implication or downstream-use links;
- `\begin{remark*}[Equivalent forms] ... \end{remark*}` for equivalence links.

These relationship blocks must also use explicit `\hyperref[...]` labels.

### 10.5 Negation Interpretation Rule

The negated quantified statement block contains the formal negation only. Explanatory prose about the negation belongs in a later failure-mode or interpretation block.

### 10.6 Failure Mode Decomposition Rule

Important negations may be followed by a `remark*` block titled **Failure mode decomposition**. This block may use underbraces or similar visual grouping to display the mathematically meaningful branches of the negation.

### 10.7 Predicate Availability Rule

Predicate blocks must use the canonical predicate library in `predicates.yaml`.

If a new predicate seems necessary, permission must be requested before it is introduced.

### 10.8 No Bundled Predicate Rule

Distinct concepts are not collapsed into one predicate block unless the canonical predicate library already treats them as inseparable.

### 10.8 Applicability Rule

Logical blocks are mandatory for theorem-like environments and definitions. They are not mandatory for remarks, examples, or proofs unless a special local purpose requires them.

### 10.9 Ordering Rule

Logical blocks appear immediately after the environment they formalize, before exposition resumes.

### 10.10 Predicate-Reading Header Rule

When a predicate-reading block is included, its header must reflect its role. Use role-specific headers such as `Definition predicate reading`, `Negation predicate reading`, and `Contrapositive predicate reading` rather than a generic `Predicate reading` label.

### 10.11 Underbrace Predicate Reading Rule

Important predicate readings may use underbrace decomposition when this clarifies the conjunctive or disjunctive structure of the statement.

### 10.12 Contrapositive Selectivity Rule

Contrapositive blocks are included only when mathematically illuminating, not merely because a contrapositive can be written formally.

### 10.12  Rule — Long Logical Display Formatting

All multi-clause logical statements must use aligned environments.

Preferred breakpoints:
- implications
- conjunctions
- disjunctions
---

## 11. Variable and Domain Rules

### 11.1 Domain Declaration Rule

Every quantified variable in logical blocks must carry an explicit domain.

### 11.2 Sequence Ambient Rule

When quantifying over a sequence in a standard quantified statement, the ambient space must be declared, for example:
\[
\forall (x_n) \subseteq \mathbb{R}
\]

### 11.3 Distinguished Variable Rule

If a definition or theorem-like environment begins by introducing specific variables and their domains, then those variables remain fixed in the immediately following logical blocks and need not be re-quantified there.

### 11.4 New Variable Rule

Any variable newly quantified inside a logical block must still declare its domain.

### 11.5 No Free Variables Rule

Variables in logical blocks must be either explicitly quantified or fixed in the immediately preceding statement.

### 11.6 Closed Predicate Rule

When a predicate statement is intended to represent the full theorem or definition, it should be logically closed rather than leaving parameters free.

---

## 12. Explanatory Prose Rules

### 12.1 Explanation Layer Rule

Expository prose is for explanation, not formalization.

All formal mathematical content must already have appeared in the environment itself or in its logical blocks.

### 12.2 No Over-Symbolization Rule

Explanatory sentences must be written in clear English. They must not compress logic into symbolic shorthand.

### 12.3 One Job Per Layer Rule

- environment body: mathematical statement
- quantified block: formal logic
- predicate block: canonical abstraction
- negation/contrapositive/failure-mode blocks: structural failure or implication anatomy
- prose: explanation

Prose explains. It does not introduce, define, or formally document.

### 12.4 Interpretation Block Rule

Every definition and theorem-like environment carries an `\begin{remark*}[Interpretation] ... \end{remark*}` block by default.

### 12.5 Interpretation Omission Rule

The Interpretation block may be omitted when nearby section-level exposition already supplies the interpretation clearly and a local interpretation would be repetitive. This exception especially applies to tightly clustered definitions such as bounds and extremals when a shared structural exposition already performs the interpretive work.

### 12.6 Itemize Layout Rule

When an `itemize` environment begins immediately inside a `remark*` block, insert `\hfill` immediately after the `\begin{remark*}[...]` line to force a clean line break before the list. Do not do this when prose already begins the body of the remark.

---

## 13. Dependency Figures

### 13.1 Purpose

Dependency figures are a standard device for displaying the logical architecture of a subchapter or chapter. They show what depends on what, which statements are foundational, and how the proof spine is organized.

### 13.2 Scope

A subchapter may include a local dependency figure. Each chapter should conclude with a larger synthesis dependency figure summarizing the structure of the chapter as a whole.

### 13.3 Storage Rule

Every dependency figure lives in a dedicated `figure-<n>.tex` file and is `\input`'d into the notes. No inline TikZ dependency graph appears in notes files.

### 13.4 Node Color Rule

Node colors reinforce the box vocabulary:

- axiom nodes use the axiom palette,
- definition nodes use the definition palette,
- theorem-like nodes use the theorem palette,
- neutral support nodes may use a light gray style.

### 13.5 Edge Color Rule

Edge colors are standardized by relation type:

- implies → blue
- uses → yellow
- defines → green

These meanings are repository-wide and must remain consistent across all dependency figures.

### 13.6 Legend Rule

Every dependency figure includes a compact legend identifying node types and edge relation colors.

### 13.7 Title and Caption Rule

Every dependency figure has:

1. a short internal figure title naming the dependency map,
2. a caption explaining what mathematical region the figure covers and what structural conclusion the reader should draw from it.

### 13.8 Caption Quality Rule

Captions must be explanatory, not merely descriptive.

### 13.9 Layout Rule

The layout should make the main proof spine visually clear. Rooted top-down trees, left-to-right definitional flows, and layered synthesis maps are preferred.

### 13.10 Label Rule

Visible node labels use display titles, not repository identifiers or raw label IDs.

### 13.11 Selectivity Rule

Dependency figures show the structural backbone, not every local remark or minor lemma.

### 13.12 Synthesis Compression Rule

Chapter-end synthesis figures may compress local detail into higher-level nodes when needed for readability.

### 13.13 Maintenance Rule

Dependency figures are part of the mathematical infrastructure of the notes and should be updated when the subchapter or chapter structure changes.

### 13.14 Standard Bottom Legend Macro Rule

All dependency figures must place their legend below the main TikZ diagram and above the caption using the shared legend macro defined in `common/boxes.tex`.

### 13.15 Edge Label Default Rule

When the standard bottom legend and standardized edge colors are in use, dependency edges are unlabeled by default. Edge labels may be added only when mixed edge types or unusual local meaning would otherwise be ambiguous.

---

## 14. Default Output Rule for Proof Requests

When a proof is requested with no qualification, the default output is:

- house notation,
- labeled statement,
- professional proof,
- detailed learning proof with bold inline steps,
- explanatory prose,
- no forbidden macros.

Only an explicit request suppresses the explanatory layer.


---

## Rule — Long logical displays must use aligned

When a quantified or predicate display is too long for a single readable line, it must be broken using an `aligned` environment inside display math.

### Preferred breakpoints

Break lines at major logical structure:

- implications  
- conjunctions  
- disjunctions  

### Purpose

This keeps logical blocks readable, prevents overflow into the margins, and makes the structure of the statement visible to the eye.

### Preferred pattern

```latex
\begin{remark*}[Standard quantified statement]
\[
\begin{aligned}
&s \text{ satisfies the epsilon characterization of } \sup A \\
&\quad\Longleftrightarrow\quad
 \bigl(\forall x \in A \;(x \le s)\bigr) \\
&\qquad\qquad\land\;
 \bigl(\forall \varepsilon > 0 \;\exists a \in A \;(s - \varepsilon < a)\bigr).
\end{aligned}
\]
\end{remark*}
```

### Consequence

A long one-line logical display should not be left flat when its structure can be made clear.


---

## Rule — Theorem/Proof Separation

Every theorem-like environment must have its statement defined in the notes layer and its proof defined in a separate proof file.

- Statements live in:
  `notes/...`
- Proofs live in:
  `proofs/notes/...`

No inline proofs inside notes unless explicitly designated as an example.

---

## Rule — Proof Stub Requirement

All generated proofs must initially be stubs.

### Required form

```latex
\begin{proof}
TODO
\end{proof}
```

### Constraints

- No additional text inside the proof
- No strategy, hints, or partial structure
- No comments
- No placeholder mathematics

---

## Rule — Proof Replacement Lifecycle

Proof stubs are placeholders for user-authored proofs.

- User writes proof externally (handwritten or draft)
- Assistant reviews/grades
- Upon acceptance:
  - Replace stub with full formal proof
  - Maintain house style
  - Preserve theorem linkage

---

## Rule — One Proof Per File

Each theorem must have exactly one proof file.

- File naming:
  `prf:<theorem-label>.tex`

---

## Rule — Proof Navigation Linking

Every proof must include navigation linking back to its theorem.

Example:

```latex
\begin{remark*}[Return]
\hyperref[thm:...]{Return to Theorem}
\end{remark*}
```

---

## Rule — No Premature Proof Generation

The assistant must not generate full proofs unless explicitly requested after the user has completed their own proof.

---

## Rule — Proof Status Tracking (Recommended)

Maintain a lightweight status marker for each proof:

- TODO
- IN_PROGRESS
- COMPLETE
- VERIFIED

This may be tracked externally (e.g., in a tracking markdown file).


---

## Rule — Theorem and Proof Navigation Links Are Required

Every theorem-like statement in the notes layer must include an explicit navigation link to its proof file, and every proof file must include an explicit navigation link back to its statement.

### Theorem-side requirement

A theorem-like environment must contain an explicit `\hyperref[...]` link pointing to the corresponding proof label.

Preferred pattern:

```latex
\begin{theorem}[...]
\label{thm:...}

...

\smallskip
\noindent
\hyperref[prf:...]{\textit{Go to proof.}}

\end{theorem}
```

### Proof-side requirement

A proof file must include an explicit return link pointing back to the theorem label.

Preferred pattern:

```latex
\begin{proof}
\label{prf:...}
TODO
\end{proof}

\begin{remark*}[Return]
\hyperref[thm:...]{Return to Theorem}
\end{remark*}
```

### Consequence

Navigation between statement and proof is mandatory and must be present in both directions.

---

## Rule — Predicate Readings Must Use Operator Names

In predicate-reading remarks and other logical displays that present formal predicate language, predicate names must be typeset using textual operator-style names rather than symbolic macros whenever readability would otherwise suffer.

### Required style

Use `\operatorname{...}` for predicate names.

Preferred examples:

```latex
\operatorname{Subset}(A,S)
\operatorname{NonemptySet}(A)
\operatorname{BoundedAbove}(A)
\operatorname{Supremum}(s,A)
```

### Forbidden style when it harms readability

Do not use symbolic predicate macros such as `\Subset` in predicate-reading displays when they collide with parentheses, arguments, or neighboring logical structure.

### Purpose

This keeps predicate-reading remarks visually stable, avoids symbol collisions, and makes the formal language easier to scan.

### Consequence

Predicate-reading remarks should present formal predicates as textual operators unless a symbolic form is clearly readable and intentionally standardized.


###  rule 15. Priority and Stability of Formal Outputs
### 15.1 Atomic File Rule: 
When generating a proof, the assistant must treat the output as the final contents of a single .tex file. It must not include conversational filler, meta-commentary, or introductory prose outside of the LaTeX source unless explicitly asked.
### 15.2 Structural Immutability: 
 The vertical sequence defined in Rule 9.5.1 (Header $\to$ Return $\to$ Label $\to$ Claim $\to$ Professional Proof $\to$ Learning Proof $\to$ Remarks) is non-negotiable. The assistant is prohibited from collapsing these into fewer environments or reordering them to "save space".
 
 ### 15.3 Default High-Fidelity Mode: 
 In the absence of "compact" or "professional-only" keywords, the assistant must automatically generate all 12+ mandatory logical blocks for notes and all 6 layers for proof files.
 
 ### 15.4 Context Continuity: 
 The assistant must refer to notation.yaml and predicates.yaml before every formal display to ensure role-specific variable and operator consistency.
