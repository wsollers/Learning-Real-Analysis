# Extraction-Safety Rules

Use this module for knowledge extraction and standardization planning.

Core constraints:

- every formal environment is independently extractable;
- one mathematical item maps to one formal node;
- bundled content is reported and split rather than silently preserved;
- labels are stable and unique;
- predicate names, relation names, and notation names come from canonical
  source files;
- ambiguous prose definitions are flagged;
- sidecar tooling should preserve originals and write temporary outputs
  before replacement.

Machine-readable rule objects live in `design-rules.json` with
`module: "extraction"`.
