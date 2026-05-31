# Decoration Audit Standards

Decoration is the surrounding house-style metadata and explanatory structure
attached to theorem-like LaTeX blocks. The audit tool inventories likely
decoration compliance; it does not decide mathematical correctness.

## Audited Block Types

The audit checks these environments:

- `definition`
- `axiom`
- `theorem`
- `lemma`
- `proposition`
- `corollary`

The audit may report, but does not enforce, these environments:

- `example`
- `remark`
- `exercise`

## Decoration Categories

For each audited theorem-like block, the audit classifies whether nearby
material contains:

- a stable label with an approved prefix: `def:`, `ax:`, `thm:`, `lem:`,
  `prop:`, or `cor:`;
- a structural box decision: boxed if load-bearing, unboxed if auxiliary, or
  uncertain if context-dependent;
- an interpretation remark, usually expected unless genuinely redundant;
- a quantified statement or formal statement block when applicable;
- a predicate reading block when canonical predicates are applicable;
- a negated quantified statement block when useful or required by local
  standard;
- a dependency remark block;
- dependency items using `\hyperref[label]{Readable Name}` where `label` is a
  mathematical statement label, not a `prf:` label;
- no invented predicates;
- no predicate leakage into theorem or definition bodies;
- proof link or proof availability when the block is a theorem-like result and
  a proof is expected.

## Output Classification

Each block should be classified as one of:

- `compliant`
- `mostly_compliant`
- `non_compliant`
- `needs_human_review`
- `not_applicable`

Each block should also receive a severity:

- `info`
- `warning`
- `error`

## Safety

The audit tool reports likely issues only. It must not auto-edit source files,
generate source patches, run bulk standardization, or sync downstream repos.

