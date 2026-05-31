# lra-lean Overlay

Stub overlay for Lean formalization.

Owned concerns:

- Lean-specific proof architecture,
- Mathlib policy,
- Lean module and namespace rules,
- Lean CI and validation,
- sync into the monorepo `lean/` tree.

## Agent Scope

Lean guidance applies only to `lra-lean` and the monorepo `lean/` mirror.
It must not be injected into volume content instructions.

Use the local Lean build and CI expectations for validation. Do not use
LaTeX render checks as substitutes for Lean validation.
