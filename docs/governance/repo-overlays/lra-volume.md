# lra-volume Overlay

Stub overlay for `lra-volume-i` through `lra-volume-viii`.
Named volume overlays, such as `lra-volume-ii.md`, may add narrow
cross-repository metadata contracts while keeping implementation ownership in
the specialist repo.

Owned concerns:

- volume content only,
- Overleaf-ready volume roots,
- local copies of synced `common/`,
- volume-to-monorepo content sync.

This overlay may contain negative guard rails that say specialist rules do not
apply to volume repos. It must not contain positive Lean-specific, C++ /
Vulkan / simulation, numerical-analysis / benchmark / plotting, or PDF
extraction workflow rules.

## Agent Scope

Volume agents may edit only the owning `volume-N/` content unless a task
explicitly says otherwise. They should not edit synced `common/`,
`bibliography/`, generated governance wrappers, or canonical YAML.

Volume tasks should preserve Overleaf readiness and monorepo sync shape.

## Stub Chapters

Volume chapter stubs follow the global `stub-chapter-standards.md` standard.
After stub generation, run the local volume build command when available; for
standard LRA volume roots, try `latexmk -lualatex main.tex` unless local
instructions say otherwise.

## Stub Sections

Volume section stubs follow the global `stub-section-standards.md` standard.
Section-stub tasks must update the owning chapter's notes and proofs routers
using the local routing convention, then run the local volume build command
when available.

## Artifact Payload Generation

For large chapter artifact generation, use the deterministic payload workflow
in `docs/workflows/artifact-payload-generation.md`. Machine-ingested JSON or
JSONL payloads are the source of truth; Codex should rehydrate payloads through
the importer and generator, preserve pedagogical order, keep notation before
first use, and run deterministic local audits by default. AI-backed audits
using `-ai codex` are opt-in only.
