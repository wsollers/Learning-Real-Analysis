# Capability Reference Discoverability Audit

Date: 2026-06-11

## Scope

Audited:

- `capabilities/manifest.yaml`
- `capabilities/ENTRYPOINT.md`
- `capabilities/index.md`
- each `capabilities/*/capability.md`

Goal: ensure required governance and architecture documents are directly listed
or reachable through a concise capability-level index.

## Finding

Before this audit, the capability resolver loaded compact capability docs and
repo overlays, but most deeper governance documents were only discoverable by
manual repository search. That kept the default context small, but it left
important references such as repository layout, volume layout, proof standards,
decoration standards, dependency standards, notation standards, and generated
file ownership outside the explicit capability path.

## Change Made

Added `capabilities/reference-index.md` as the concise escalation map. It maps
each capability family to the smallest relevant governance and architecture
documents:

- volume statement authoring;
- volume definition authoring;
- chapter and section scaffolding;
- Lean work;
- C++ numerical work;
- cross-repo and generated-file ownership.

Updated `capabilities/manifest.yaml` so every capability includes
`capabilities/reference-index.md` in its `reads` list.

Updated `capabilities/ENTRYPOINT.md`, `capabilities/index.md`, and each
capability doc to point to the reference index when the overlay and nearby
examples are insufficient.

## Coverage Result

| Capability | Reference status |
|---|---|
| `author-statement` | Covered through `Volume Statement Authoring` references. |
| `author-definition` | Covered through `Volume Definition Authoring` references. |
| `author-stub-chapter` | Covered through `Chapter And Section Scaffolding` references. |
| `author-stub-section` | Covered through `Chapter And Section Scaffolding` references. |
| `author-lean-theorem` | Covered through `Lean Work` references. |
| `cpp-build-task` | Covered through `C++ Numerical Work` and cross-repo references. |

## Verification

- `python capabilities/test_resolve.py`: 11/11 passed.
- `python -m py_compile capabilities/resolve.py`: passed.
- Reference path existence check: 24/24 referenced docs exist.

## Residual Risk

The reference index makes docs discoverable; it does not make every legacy
auditor rule validator-owned. Semantic auditor gaps remain tracked in
`docs/reports/validator-auditor-gap-analysis.md`.
