# Authoring Standards

Source sections: `DESIGN.md` sections 1, 4, 5, 6, 6.1, 7, 12, and 15.

## Purpose

The notes are a long-term mathematical reference. They preserve definitions,
theorems, proof structures, dependencies, and canonical notation in a form that
remains readable, rigorous, and stable across years of revision.

## Voice

Use a precise reference voice. Do not write as a course transcript, workbook,
chatty explanation, or motivational aside. Exposition should state what a
definition or result does mathematically and how it fits the local structure.

## Boxes

Boxes are structural, not decorative.

Box a definition, axiom, or theorem only when it introduces a structural,
load-bearing concept or result. A boxed item should be central to the local
section, reused later, important for future learning, or expected to carry
dependency weight in the knowledge graph.

Do not box merely because an item is a first appearance.

Every mathematical concept shall be introduced in its own definition
environment and shall possess its own unique label. Grouping multiple
independent mathematical concepts into a single definition environment is
prohibited. This rule is architectural, not stylistic: one concept maps to one
definition, one label, one knowledge-graph node, and one extraction record.

Box-worthy examples include structural concepts and results such as
`Supremum`, `Least Upper Bound Property`, `Sequential Limit`, `Cauchy Sequence`,
`Continuity`, `Derivative`, `Partition`, and `Riemann Integrability`.

Minor auxiliary notions, routine variants, examples, remarks, lemmas,
propositions, corollaries, computational rules, and one-off conveniences are
normally unboxed unless the section explicitly treats them as structural and
load-bearing.

Chapter entries use the required breadcrumb and roadmap structure. The chapter
`index.tex` router must use a non-starred `\chapter{...}` heading and contain
exactly one breadcrumb box before routed content inputs.

Each active notes section router, `notes/<section>/index.tex`, begins with a
non-starred `\section{...}` so the chapter table of contents exposes the
section spine. Sections must not use `\section*`.

Immediately after the section heading, the section router contains exactly one
gray Toolkit box. The Toolkit orients the section's vocabulary and formal
payload; it is not repeated in subsection/topic body files.

Immediately after the Toolkit, the section router contains one short
`remark*` block titled `Exposition`. This section-level exposition introduces
the mathematical role of the section in a precise reference voice. It should
be brief, normally one paragraph and no more than about 100 words. The first
routed `\input` follows this exposition.

Subsection/topic body files should use starred subsection headings by default,
such as `\subsection*{...}`, so the table of contents remains a chapter-section
spine rather than a full topic inventory. Subsection body files should not open
with generic orienting exposition; the section router owns that orientation.

## Figures

Every nontrivial TikZ figure shall exist as an independent figure source file.
Embedded nontrivial `tikzpicture` environments in note bodies, proof bodies,
exercise bodies, exposition blocks, and statement files are prohibited.

Figure source files contain only the `tikzpicture` environment. Captions,
labels, placement, and explanatory prose belong at the inclusion point.

## LaTeX Structural Integrity

Generated LaTeX must be structurally balanced before acceptance. Every `\[`
must have a matching `\]`, every `\begin{...}` must have a matching
`\end{...}`, and `\cite` or `\label` must not appear inside display math unless
intentionally part of the mathematical display. Close display math before
citations, labels, environment endings, or prose continuation.

## Chapter Entries

Chapter openings use the canonical entry pattern from `DESIGN.md`: breadcrumb,
status when needed, roadmap, and chapter structure. Breadcrumbs are structural
dependency statements, not motivational prose.

## Layered Exposition

Each layer has one job:

- formal environments state the mathematics,
- interpretation remarks explain meaning,
- logical blocks expose formal structure,
- dependency blocks support extraction,
- prose connects local context.

Do not over-symbolize exposition. Use prose where prose is the correct layer.

## Statement Boxes

Boxes are structural emphasis, not the default presentation of every formal
environment. A definition, theorem, lemma, proposition, or corollary may be
boxed when it is load-bearing for the section or chapter, named and repeatedly
cited, or otherwise the structurally dominant result being introduced. Do not
box merely because an item is a first appearance or because it has a label.

Unboxed formal environments remain fully valid repository artifacts. They still
need atomic scope, stable labels, required logical/dependency blocks, and the
same extraction discipline as boxed statements. When a statement is boxed, use
the matching shared semantic wrapper; never hand-roll local `tcolorbox` styling.

Generated note prose must live inside its owning LaTeX layer. Do not leave
ordinary explanatory prose directly under a section or subsection heading. Use
formal environments for mathematical objects, `remark*` blocks for exposition
and interpretation, `example*` or `remark*` example metadata for examples and
non-examples, and the shared `dependencies` environment for dependency lists.

Section-level exposition belongs in the section router immediately after the
Toolkit. Item-adjacent exposition remains available in body files when it is
mathematically substantive and attached to a definition, theorem, example, or
local concept cluster.

## Multi-Definition Source Passages

When converting a source passage that contains several related definitions,
operations, cases, or constructions, do not bundle the independent concepts
into one formal definition environment. Classify the passage as a related
definition cluster when it has signals such as:

- "one of the following";
- "the four operations";
- "for each of";
- "this leads to";
- "the endpoint formulas are";
- a set-builder definition followed by multiple displayed equations;
- repeated formulas with the same left-hand object and different operators or
  cases;
- prose saying one displayed equation defines several operators or cases.

For a related definition cluster, begin with a short `remark*` block titled
`Exposition` that explains the shared conceptual family, then give a general
definition when the source gives a common set-based or abstract definition,
then give each independently nameable operation, case, or construction its own
definition environment and stable label.

Do not replace those atomic definitions with a single parameterized umbrella
definition such as "let `\circ` range over the operations" when the source gives
named operations, cases, or endpoint/formula rules separately. The shared
schema may be explained in the exposition block, but each named operation or
case still receives its own definition environment.

When an example is supported by the source or follows directly without adding
new mathematical claims, include a short example after the relevant definition
if it materially improves understanding. Do not add examples merely to satisfy
a pattern, and do not invent examples that introduce unsupported hypotheses,
notation, or claims.

Suppress accidental OCR/prose fragments as definition titles. Titles such as
"And B", "The following", "This leads to", "With", "For", "Therefore", or any
phrase that is not a meaningful mathematical noun phrase must be normalized or
discarded.

Labels for generated definitions must be derived from normalized semantic
titles, not raw OCR spans. Use stable lowercase kebab-case labels such as
`def:interval-addition`, not labels that include page numbers, prose fragments,
or truncated OCR text such as
`def:2-3-real-interval-arithmetic-let-denote-one-of-the-four-arithmet`.

## Examples And Non-Examples

Examples and non-examples are concept-boundary tools. Formal definitions state
what a concept is; examples and non-examples help readers recognize where the
concept applies and where it fails.

Definitions may be followed by optional explanatory remark blocks:

```latex
\begin{remark*}[Examples]
...
\end{remark*}

\begin{remark*}[Non-Examples]
...
\end{remark*}
```

Include these blocks when they materially improve concept-boundary
recognition. They are especially valuable for major algebraic or structural
objects, subtle predicates, and frequently confused concepts.

Examples and non-examples are usually unnecessary for simple auxiliary
definitions, notation declarations, obvious derived concepts, or definitions
whose examples immediately appear in nearby theorems.

Non-examples should identify the precise failed condition whenever practical.
Do not merely state that an object is not an example when the failed axiom,
condition, or hypothesis can be named.

## Exposition Remark Blocks

Use `remark*` blocks titled `Exposition` when a topic benefits from
conceptual explanation, motivation, intuition, historical context, structural
commentary, or connections to nearby topics:

```latex
\begin{remark*}[Exposition]
...
\end{remark*}
```

Exposition blocks are for mathematical explanation that is neither a formal
definition nor a theorem. They may explain why a definition matters, how a
result fits the local structure, what earlier material it connects to, or what
future material it prepares.

Do not use exposition blocks merely to restate a displayed formula or repeat
an interpretation block. Use `Interpretation` when translating one specific
formal definition or theorem into ordinary mathematical language. Use
predicate-reading blocks only when unpacking logical form.

Exposition remark blocks use the normal unboxed `remark*` style. Do not
introduce a new colored box for exposition.

This is distinct from the topic-level `exposition` environment used inside
topic boxes. The environment `exposition` orients a topic cluster; the
`remark*` block titled `Exposition` makes item-adjacent conceptual narrative
extractable.
