# Theorem Rules

Use this module for theorem-like statements: theorem, lemma, proposition,
corollary, axiom, assumption, and consequence.

Core constraints:

- theorem-like environments are atomic;
- theorem-like bodies use standard mathematical notation only;
- predicate names are forbidden in formal bodies and standard/negated/
  contrapositive quantified statements;
- theorem-like statements have labels with the correct prefixes;
- theorem-like statements carry logical blocks in DESIGN order;
- theorem-like statements in notes include proof navigation links where
  required.

Machine-readable rule objects live in `design-rules.json` with
`module: "theorems"`.
