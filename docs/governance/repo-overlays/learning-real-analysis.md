# Learning-Real-Analysis Overlay

Stub overlay for the monorepo integration hub.

Owned concerns:

- assembled monorepo integration,
- omnibus builds,
- canonical predicate / notation / relation YAML files,
- cross-volume extraction integration,
- sync receiver behavior.

Do not touch the intentionally untracked `scripts/` directory as part of this
governance migration.

## Agent Scope

Agents working here may coordinate across integrated content, canonical YAML,
and extraction dispatch, but must not treat downstream synced copies as their
own source of truth.

Canonical YAML edits are allowed only when the task explicitly targets
`predicates.yaml`, `notation.yaml`, or `relations.yaml`.
