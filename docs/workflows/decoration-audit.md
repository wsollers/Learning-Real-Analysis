# Decoration Audit Workflow

Decoration audit is inventory-only. It scans theorem-like LaTeX blocks and
reports likely compliance issues without rewriting source files.

## Inputs

The audit tool may scan `lra-volume-*` repos read-only. It writes reports only
to the requested output directory and must not create patches or modify
mathematical content.

## Checks

The audit classifies labels, decoration remarks, dependency blocks, proof
links, predicate leakage, oversized environments, and one-object-per-environment
risks according to `docs/governance/decoration-audit-standards.md`.

Box status is reported as detected, not detected, or uncertain. Final
box-worthiness remains a human authoring decision because structural importance
is context-dependent.

## Optional Local Model Use

Ollama or other local model output may help classify ambiguous cases, but it is
advisory only. The model must not rewrite blocks, invent labels, invent
predicates, infer missing dependencies without visible labels, or apply source
changes.

## Output

Audit reports should estimate cleanup scope by volume, file, classification,
severity, and issue code. They are planning artifacts, not automatic
standardization instructions.
