<!--
GENERATED FILE — DO NOT EDIT BY HAND.

Source repo: wsollers/lra-governance
Source commit: d98bb51fc80e683b38a9d1e76f4a0c91037ede0a
Generated from:
- docs/governance/...
- docs/architecture/...
- docs/governance/repo-overlays/learning-real-analysis.md

Regenerate from lra-governance.
Emergency downstream edits must be ported upstream before the next sync.
-->

# Claude Instructions

@AGENTS.md

If import semantics are unavailable, use the generated `AGENTS.md` content for
the same repository as the canonical local instruction body.

## Global Agent Rules

- Treat generated instruction files as derived artifacts.
- Follow the owning repository boundary for every task.
- Do not include secrets, credentials, tokens, or machine-local private values.
- Do not modify mathematical content during governance or wrapper-generation tasks.
- Do not touch `Learning-Real-Analysis/scripts/`.
- Port emergency downstream instruction repairs back to `lra-governance`.

## Repo Overlay

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

## Provider Notes

Claude should use this wrapper as a pointer to the generated repo instructions.
