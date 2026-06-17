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

## Volume II Verification Map

For Volume II formalization work, each declaration that mirrors a volume
artifact should record a stable mapping back to the LaTeX label. Prefer a
small, grep-friendly metadata comment near the declaration or module section
that includes:

- the Volume II label,
- the Lean module,
- the declaration name,
- the verification status.

The status must distinguish an accepted statement with unfinished proof work
from a checked declaration. Report `checked` only when the declaration is
accepted by the local Lean build without placeholders for that declaration.
This metadata is the source that downstream explorer extraction may use to
populate verification fields.
