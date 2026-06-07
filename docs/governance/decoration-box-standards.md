# Decoration Box Standards

Decoration box standards govern the non-statement information blocks that
surround definitions, theorem-like environments, and related formal artifacts.

These blocks are part of the standard information set for an artifact. They are
not decorative flourish. They provide formal restatements, predicate readings,
failure-mode analysis, interpretation, exposition, examples, non-examples, source
crosswalks, dependencies, and proof navigation where the artifact type
requires them.

## Rendering Rule

Decoration blocks are structural metadata blocks. Unless an artifact-specific
standard explicitly states otherwise, decoration blocks render as unboxed
`remark*` environments rather than standalone theorem-style boxes.

This applies to standard quantified statements, predicate readings, negated
quantified statements, negation predicate readings, failure-mode blocks,
interpretation blocks, exposition blocks, examples, non-examples, source
crosswalks, and dependency blocks.

The owning definition, theorem, lemma, proposition, corollary, or axiom remains
the primary formal artifact. Decoration blocks are attached metadata and do not
create an independent theorem-like visual hierarchy.

## Scope

Use this standard for standardized surrounding blocks attached to:

- definitions;
- axioms;
- theorems;
- lemmas;
- propositions;
- corollaries;
- vocabulary definitions when applicable;
- structural definitions when applicable.

This standard does not decide whether an artifact requires each block. The
artifact-specific standard decides that. This document standardizes the shape,
meaning, and order of blocks once they are required or intentionally included.

## Decoration Blocks

The standardized decoration blocks are:

- Standard quantified statement;
- Definition predicate reading;
- Predicate reading;
- Negated quantified statement;
- Negation predicate reading;
- Failure modes;
- Failure mode decomposition;
- Contrapositive quantified statement;
- Contrapositive predicate reading;
- Interpretation;
- Historical note;
- Comparison with Feferman;
- Exposition;
- Examples;
- Non-Examples;
- Dependencies;
- Proof navigation link.

Artifact-specific standards may omit blocks that are not meaningful for that
artifact type.

## Ordering Rule

When a block is present, use this order:

1. Formal environment and label
2. Proof navigation link, when required inside the formal environment
3. Standard quantified statement
4. Definition predicate reading or Predicate reading
5. Negated quantified statement
6. Negation predicate reading
7. Failure modes
8. Failure mode decomposition
9. Contrapositive quantified statement
10. Contrapositive predicate reading
11. Interpretation
12. Historical note or Comparison with Feferman
13. Exposition
14. Examples
15. Non-Examples
16. Dependencies or `\NoLocalDependencies`

Do not reorder blocks for aesthetics. Omit only blocks that the governing
artifact standard does not require.

## Standard Quantified Statement

Use a `remark*` block titled `Standard quantified statement`.

The block contains standard mathematical notation only. Do not use canonical
predicate names or extraction predicate forms here.

Preserve all hypotheses and ambient variables. For structured objects, state
the ambient setting explicitly rather than using malformed quantified syntax.

## Predicate Readings

Use `Definition predicate reading` for definitions and `Predicate reading` for
theorem-like environments.

Predicate readings must use canonical predicate names when they exist. If no
canonical predicate exists, do not invent one merely to fill the block.

Use `\operatorname{...}` predicate notation unless a shared macro is explicitly
defined.

Vocabulary definitions and structural definitions normally omit predicate
readings unless the predicate is canonical and useful.

## Negation Blocks

Negated quantified statements and negation predicate readings are proof-use
blocks. Include them when the negated form is a standard proof tool or when the
artifact-specific standard requires them.

Push negations inward and preserve the same ambient hypotheses and free
variables as the positive statement.

Do not include negation blocks merely because a formal negation can be written.

## Failure Modes

Use failure-mode blocks when a definition or result has meaningful ways to
fail and the distinction helps later reasoning.

Failure modes are prose. Failure mode decomposition is the formal or displayed
decomposition of those branches.

Do not use failure-mode blocks as informal interpretation. If the goal is to
explain meaning, use `Interpretation`.

## Contrapositive Blocks

Contrapositive blocks apply only to theorem-like environments with a genuine
hypothesis-conclusion structure.

Do not generate contrapositive blocks for definitions or axioms.

Include contrapositives only when they are standard proof tools for the result,
not merely because they can be formed mechanically.

## Interpretation

Use a `remark*` block titled `Interpretation`.

Interpretation is prose only. It explains mathematical meaning, structural
role, standard failure picture, and local significance.

Interpretation blocks remain encouraged across artifact types, including
vocabulary and structural definitions.

## Exposition

Use a `remark*` block titled `Exposition`.

Exposition is broader mathematical narrative: motivation, intuition,
conceptual framing, structural commentary, historical or methodological
context, and relationships to nearby topics. It is not a formal definition,
theorem, predicate reading, or dependency list.

Use `Interpretation` when translating one specific formal item into ordinary
mathematical language. Use predicate-reading blocks when unpacking logical
form. Use `Exposition` for topic-level explanation or broader conceptual
framing.

Exposition blocks use the normal unboxed `remark*` style. They are
extractable explanatory metadata attached to the nearest relevant formal item
or section; they do not create separate knowledge-graph nodes by default.

Do not confuse `remark*` titled `Exposition` with the topic-level
`exposition` environment used inside topic boxes.

## Source Crosswalks

Use `Historical note` for direct provenance and `Comparison with Feferman` for
structural comparison with Feferman's presentation.

Source crosswalks appear after Interpretation and before Exposition,
Examples, Non-Examples, and Dependencies. They
must not appear inside formal environments, quantified statements, predicate
readings, negation blocks, or failure-mode decompositions.

## Examples And Non-Examples

Definitions may include optional concept-boundary blocks titled `Examples` and
`Non-Examples`.

Include them when they materially improve recognition of what the definition
does and does not cover. They are especially useful for major algebraic
structures, subtle predicates, and frequently confused pairs of concepts.

Non-examples should identify the failed axiom, condition, or hypothesis
whenever practical. These blocks are explanatory metadata attached to the
owning definition; they do not create separate knowledge-graph nodes.

## Dependencies

Dependency blocks are governed by `dependency-standards.md`. This standard
only fixes their position in the decoration order.

Use the shared `dependencies` environment for visible dependency blocks and
`\NoLocalDependencies` for foundational local note-body statements with no
local dependencies to display.

## Audit Boundary

Decoration audits check whether required blocks are present, ordered, and
well-formed. They do not decide mathematical correctness.

Auditors must apply the artifact-specific standard first. For example,
Vocabulary Definitions and Structural Definitions should not be flagged merely
because predicate-oriented blocks were omitted under their governing standard.
