# Rule Modules

`DESIGN.md` remains the authoritative house constitution.  This directory
is a machine- and workflow-friendly decomposition of that file.

The goal is to keep rule application narrow:

- definition generation should load definition, notation, and logical-block
  rules;
- theorem generation should load theorem, proof-link, notation, and
  logical-block rules;
- proof generation should load proof rules;
- repository audits should load structure, figure, label, and extraction
  rules.

The machine-readable registry is:

```text
rules/design-rules.json
```

Each object records:

- `id`: stable rule identifier for tools
- `source`: where the rule came from in `DESIGN.md`
- `module`: human rule module
- `scope`: when the rule applies
- `applies_to`: workflow tags
- `status`: implementation status in the current planner
- `summary`: concise rule statement
- `checks`: suggested deterministic checks
- `generation_contract`: constraints for AI generation prompts

The markdown files in this directory are curated views for humans.  They
should not replace `DESIGN.md`; they should make it easier to use the
right part of the constitution at the right time.
