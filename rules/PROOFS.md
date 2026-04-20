# Proof Rules

Use this module for proof-file generation and proof architecture audits.

Core constraints:

- notes contain theorem statements, proof files contain proofs;
- no inline proofs in notes unless explicitly designated as examples;
- generated proof files use the canonical proof-file stack;
- proof restatements in proof files are unnumbered;
- labels do not appear inside proof-file restatement environments;
- proof files include Return navigation;
- theorem-like statements in notes include proof navigation;
- proof stubs must be exactly `TODO` until replaced by user-approved
  proofs.

Machine-readable rule objects live in `design-rules.json` with
`module: "proofs"`.
