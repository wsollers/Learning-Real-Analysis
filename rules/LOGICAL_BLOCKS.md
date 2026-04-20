# Logical Block Rules

Use this module for Standard quantified statement, predicate reading,
negation, failure-mode, contrapositive, and Interpretation blocks.

Required order after formal environments:

1. Standard quantified statement
2. Definition predicate reading, when applicable
3. Negated quantified statement, when illuminating
4. Negation predicate reading, if negation was generated
5. Failure modes, when applicable
6. Failure mode decomposition, if failure modes were generated
7. Contrapositive quantified statement, when illuminating
8. Interpretation

Role discipline:

- standard, negated, and contrapositive quantified statements use standard
  notation only;
- predicate readings use canonical predicates only;
- Interpretation is prose by default;
- long multi-clause displays use `aligned`.

Machine-readable rule objects live in `design-rules.json` with
`module: "logical_blocks"`.
