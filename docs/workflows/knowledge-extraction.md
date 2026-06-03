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

Extraction assumes source follows `docs/governance/extraction-standards.md`,
`docs/governance/dependency-standards.md`, and the structured rules in
`constitution/schema/block-registry.yaml`,
`constitution/schema/artifact-matrix.yaml`, and
`constitution/schema/file-schema.yaml`.

## Review

Generated graph, JSON, or explorer artifacts should be reviewed against the
source labels and dependency blocks. If extraction exposes missing labels,
missing dependencies, or ambiguous predicate needs, report them as source or
governance issues rather than inventing replacements.
