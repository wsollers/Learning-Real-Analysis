# Canonical YAML

Source: `DESIGN.md` section 3 and `REPOSITORY_STRUCTURE.md`.

The source-of-truth YAML files remain in `Learning-Real-Analysis`:

- `predicates.yaml`
- `notation.yaml`
- `relations.yaml`

These files are not duplicated into volume repos. Automated authoring,
auditing, and extraction tools must read them from the monorepo or receive an
explicit monorepo root path.

No agent may invent predicate, relation, or notation names locally in content
files. Missing canonical vocabulary must be reported as a governance or YAML
update need.
