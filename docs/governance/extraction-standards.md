# Extraction Standards

Source sections: `DESIGN.md` sections 2.4, 8, 10, 13, and the knowledge
explorer pipeline notes in `REPOSITORY_STRUCTURE.md`.

## Extraction Contract

Extraction depends on stable labels, one theorem-like object per environment,
dependency remarks, and canonical logical blocks. Authoring changes must
preserve those structures unless the extraction pipeline is explicitly updated.

Author-facing dependency rules live in `dependency-standards.md`. This document
records the extraction contract and implementation-facing expectations.

## Atomic Extraction Identity

Extraction shall enforce the atomic artifact invariants from
`atomic-artifact-standards.md`.

Every mathematical concept shall correspond to exactly one definition
environment, one label, one knowledge-graph node, and one extraction record.
Extraction tools shall not merge multiple independent mathematical concepts
from a bundled definition into a single graph node.

When source content contains a multi-concept definition, the correct migration
is to split the source into atomic definitions, create stable labels for each
concept, update references, and regenerate extraction records. Creating one
combined extraction record for multiple concepts is prohibited.

Every nontrivial TikZ figure shall be represented by a dedicated figure source
file. Figure source files contain only the `tikzpicture`; captions, labels,
and figure environments are extracted from the inclusion point.

## Dependency Blocks

Dependency blocks must be readable in the PDF and extractable by tooling.

Dependency blocks must use the shared LaTeX `dependencies` environment, not a
raw `remark*` environment with a `Dependencies` title. The shared environment
preserves consistent heading/body alignment across volume builds while keeping
dependency items extractable.

The preferred dependency block form is:

```latex
\begin{dependencies}
\begin{itemize}
  \item \hyperref[label]{Readable Name}
\end{itemize}
\end{dependencies}
```

The preferred dependency item form is:

```latex
\item \hyperref[label]{Readable Name}
```

where `label` is a mathematical statement label with one of these prefixes:

- `def:`
- `ax:`
- `thm:`
- `lem:`
- `prop:`
- `cor:`

The extractor should treat the label inside `\hyperref[...]` as the graph-edge
target.

Every visible dependency item creates an edge from the owning artifact to the
target label. The edge is a dependency-route edge for the learning graph, not
necessarily a strict proof-theoretic prerequisite.

When no explicit edge kind is encoded in the LaTeX source, extract the edge as
`kind: dependency`. When later tooling or a canonical route registry supplies
edge kinds, the allowed dependency route kinds are:

- `prerequisite`
- `structural-existence`
- `structural-pairing`
- `proof-use`

For example, in a dependency block attached to `def:supremum`, the edge to
`def:upper-bound` is a prerequisite route and the edge to
`ax:real-completeness` is a structural-existence route. The visible PDF block may
show both simply as dependencies; the Knowledge Explorer should not assume that
both edges have the same logical meaning.

## Proof Vault Links

Proof-vault backlinks must use:

```latex
\ProofVaultURL{https://github.com/wsollers/lra-proof-vault/tree/master/path/to/sanitized-record}
```

The extractor should associate the URL argument with the owning proof label.
Do not infer proof-vault links from raw `\href` commands or ordinary prose
URLs.

Do not use `prf:` labels as dependency targets. A `prf:` label identifies a
proof file or proof location, not a mathematical dependency.

Dependency blocks should not contain prose-only dependencies such as "uses
completeness" when a formal label exists. Use a linked statement label instead.

If the target has not yet been formalized, write a TODO dependency note and do
not invent a label.

## Logical Blocks

Logical blocks must be ordered and named consistently. Predicate-reading blocks
may use canonical predicates only where their layer permits them.

Decoration block shape and order are governed by
`decoration-box-standards.md`.

## Labels

Formal environments require stable labels with approved prefixes. Labels should
be ASCII, descriptive, and aligned with filenames and proof labels.

Definition labels shall be atomic: one concept, one definition, one label.
Shared labels for bundled independent concepts are prohibited.

## Extraction Notation Normalization

Preserve mathematical meaning; normalize mathematical notation.

When converting copied text, OCR text, or image-derived mathematical content
into LRA-ready candidate LaTeX, extraction tools shall normalize notation into
house style while preserving the source meaning. The standard normalization
table lives in `docs/governance/notation-standards.md` under
`Standard Notation Normalizations`.

The extractor should not reproduce OCR notation mechanically. It should rewrite
plain text, Unicode symbols, inconsistent typography, and OCR artifacts into
standard mathematical LaTeX used by the project, unless the task explicitly
requests source-faithful transcription.

Before returning candidate LaTeX, check:

- Are number systems written in house notation?
- Are membership and subset symbols normalized?
- Is multiplication notation consistent?
- Are interval families written as `\mathcal{I}(\mathbb{R})` when appropriate?
- Are endpoint symbols normalized?
- Are OCR artifacts such as `(1,2]` corrected when the source clearly shows
  `[1,2]`?
- Has the extractor preserved meaning rather than merely copying source
  typography?

## Examples And Non-Examples Metadata

`Examples` and `Non-Examples` remark blocks are explanatory metadata attached
to the preceding definition node. They do not create knowledge-graph nodes and
must not be treated as formal dependency targets.

When extraction supports them, examples and non-examples may be indexed as
metadata on the owning definition record, for example:

```yaml
node: Ring
examples:
  - "\\mathbb{Z}"
  - "\\mathbb{Z}[x]"
  - "M_n(\\mathbb{R})"
non_examples:
  - object: "\\mathbb{N}"
    failure: "fails additive inverses"
```

Non-example metadata should preserve the failed condition when the source text
identifies it.

## Exposition Metadata

`Exposition` remark blocks are extractable explanatory metadata. They preserve
mathematical narrative without turning ordinary prose into a formal node.

The canonical source form is:

```latex
\begin{remark*}[Exposition]
...
\end{remark*}
```

Extraction tools should attach exposition artifacts in this order:

1. To the most recent preceding definition, theorem, axiom, proposition,
   lemma, corollary, or example in the same section.
2. If no such item exists, to the current section.
3. If the source explicitly tags an attachment target, to the referenced
   label.

Exposition artifacts should not become primary knowledge-graph nodes by
default. They may appear in hover cards, side panels, or detail panes.

Preferred extracted shape:

```yaml
expositions:
  - id: exposition:compactness-payoff
    attached_to: thm:extreme-value-theorem
    attached_to_kind: theorem
    heading: Exposition
    source_file: volume-iii/analysis/continuity/notes/index.tex
    source_line_start: 42
    source_line_end: 48
    body: |
      Compactness is the hypothesis that prevents values from escaping.
```

## Theorem Explorer

The theorem explorer implementation is owned by `lra-knowledge-explorer`.
`Learning-Real-Analysis` remains the integration point for LaTeX source and
rebuild dispatch.
