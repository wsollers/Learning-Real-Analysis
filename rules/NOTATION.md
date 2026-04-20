# Notation Rules

Use this module whenever generating or auditing formal mathematical
content.

Core constraints:

- formal bodies use standard mathematical notation;
- predicate names are confined to predicate-reading or explicit
  predicate-analysis roles;
- canonical predicate, relation, and notation names come from
  `predicates.yaml`, `relations.yaml`, and `notation.yaml`;
- quantified variables in logical blocks need explicit domains unless
  fixed by the immediately preceding statement;
- long logical displays use aligned formatting.

Machine-readable rule objects live in `design-rules.json` with
`module: "notation"`.
