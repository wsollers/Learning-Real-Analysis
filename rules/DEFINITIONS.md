# Definition Rules

Use this module for generating or auditing definitions and definition
families.

Core constraints:

- every definition environment is atomic;
- paired notions and concept families are split into separate definitions;
- bundled definitions require Toolkit, exposition, then atomic formal
  blocks;
- definition bodies use standard notation only;
- canonical predicates belong in predicate-reading remarks, not the
  definition body;
- each definition has a `def:` label;
- each definition carries the required logical block stack.

Machine-readable rule objects live in `design-rules.json` with
`module: "definitions"`.
