# Knowledge Extraction Workflow

Knowledge extraction turns stable LaTeX source into graph and explorer
artifacts.

## Inputs

Use the integrated source tree in `Learning-Real-Analysis` unless a task
explicitly targets a repo-local audit. Canonical YAML remains in
`Learning-Real-Analysis`:

- `predicates.yaml`
- `notation.yaml`
- `relations.yaml`

Split repos may be scanned, but tools that need canonical YAML should receive
the monorepo root explicitly.

## Expected Source Shape

Extraction assumes:

- one mathematical object per theorem-like environment,
- stable labels with approved prefixes,
- dependency blocks after interpretation material,
- dependency items using `\hyperref[label]{Readable Name}`,
- proof labels using `prf:` only for proof locations and navigation,
- no invented predicates,
- no predicate leakage into theorem or definition bodies.

## Review

Generated graph, JSON, or explorer artifacts should be reviewed against the
source labels and dependency blocks. If extraction exposes missing labels,
missing dependencies, or ambiguous predicate needs, report them as source or
governance issues rather than inventing replacements.
