# Extraction Standards

Source sections: `DESIGN.md` sections 2.4, 8, 10, 13, and the knowledge
explorer pipeline notes in `REPOSITORY_STRUCTURE.md`.

## Extraction Contract

Extraction depends on stable labels, one theorem-like object per environment,
dependency remarks, and canonical logical blocks. Authoring changes must
preserve those structures unless the extraction pipeline is explicitly updated.

Author-facing dependency rules live in `dependency-standards.md`. This document
records the extraction contract and implementation-facing expectations.

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

## Theorem Explorer

The theorem explorer implementation is owned by `lra-knowledge-explorer`.
`Learning-Real-Analysis` remains the integration point for LaTeX source and
rebuild dispatch.
