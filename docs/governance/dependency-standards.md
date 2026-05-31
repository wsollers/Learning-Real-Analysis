# Dependency Standards

Dependency standards govern dependency information wherever it appears in the
notes, proof files, stubs, structural presentations, and extracted knowledge
artifacts.

Dependencies are not a standalone mathematical artifact type. They are a
shared information set consumed by definitions, axioms, theorem-like
environments, completed proofs, proof stubs, structural presentations, and
Knowledge Explorer records.

## Scope

Use this standard whenever an item records mathematical prerequisites,
references another formal statement, links a proof artifact back to its source,
or exposes graph edges for extraction.

Dependency information may appear with:

- definitions;
- axioms;
- theorems, lemmas, propositions, and corollaries;
- completed proofs;
- proof stubs;
- vocabulary definitions;
- structural definitions;
- model-theoretic presentations;
- algebraic or structural blueprints;
- classification cards.

## Dependency Layer

A dependency block records mathematical reliance, not exposition, history, or
proof navigation.

Dependency blocks answer:

```text
Which already-defined mathematical objects or results does this item use?
```

They do not answer:

```text
Where is the proof?
What source text inspired this item?
What should be read next?
What is the informal motivation?
```

Those belong to proof navigation, source crosswalk remarks, reading order,
toolkits, or interpretation remarks.

## Standard LaTeX Form

Visible dependency blocks must use the shared `dependencies` environment.

Preferred form:

```latex
\begin{dependencies}
\begin{itemize}
  \item \hyperref[label]{Readable Name}
\end{itemize}
\end{dependencies}
```

The dependency target must be a mathematical statement label with an approved
prefix:

- `def:`;
- `ax:`;
- `thm:`;
- `lem:`;
- `prop:`;
- `cor:`.

Do not use `prf:` labels as dependency targets. A `prf:` label identifies a
proof location, not a mathematical prerequisite.

## Silent Foundational Marker

If a note-body statement is foundational within the local scope and has no
local dependencies to display, use:

```latex
\NoLocalDependencies
```

Do not print a visible dependency block that says only "No local dependencies."

The silent marker satisfies dependency recording for local foundational
statement artifacts. It is not prose and should render nothing.

## TODO Dependencies

If a dependency is mathematically real but the target statement has not yet
been formalized, write a TODO dependency note and do not invent a label.

Example:

```latex
\begin{dependencies}
\begin{itemize}
  \item TODO: formalize the relevant compactness statement.
\end{itemize}
\end{dependencies}
```

Replace TODO dependency notes with formal links once the target exists.

## Proof Dependencies

Completed proofs and proof stubs must record dependencies according to their
own proof standards, but the dependency targets still follow this standard.

Proof-vault URLs are not dependency targets. They are proof artifact metadata
and must use the proof-vault macro specified by proof standards and extraction
standards.

## Structural Presentations

Structure blueprints, classification cards, and signature presentations may
carry dependency metadata, but they do not automatically require visible
dependency blocks unless they introduce a formal statement or a reusable
extraction record.

When a structural presentation points to formal prerequisites, use the same
approved label prefixes and readable link text.

## Extraction Contract

Dependency information is extraction-visible. Tooling should treat each
approved `\hyperref[label]{Readable Name}` dependency item as a graph edge from
the owning artifact to the target label.

Extraction implementation details belong in `extraction-standards.md`; this
document is the author-facing dependency rule source.
