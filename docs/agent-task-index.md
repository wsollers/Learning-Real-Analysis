# Agent Task Index

This index tells agents which governance files to read for common LRA tasks.
It is intentionally a router, not a replacement for the referenced standards.

## Repository Scope Rule

When working from a local multi-repo checkout such as `F:\repos`, prefer the
canonical files in `lra-governance` for shared governance, architecture,
workflow, prompt, and overlay rules.

When an agent is running inside an isolated repository checkout, such as a
GitHub workflow or a single leaf repo clone without adjacent `lra-governance`,
use that repo's synced local copies under `docs/`.

Do not read every governance, workflow, architecture, and prompt file by
default. Select the smallest file set that matches the task.

## Task Routes

| Task | Read first | Read only if needed |
| --- | --- | --- |
| Add a theorem with a proof obligation | `docs/workflows/add-theorem-with-proof-stub.md`, `docs/governance/proof-standards.md` | `docs/governance/dependency-standards.md`, `docs/governance/atomic-artifact-standards.md`, `docs/architecture/volume-layout.md` |
| Populate an existing proof stub | `docs/workflows/populate-proof-stub.md`, `docs/governance/proof-standards.md`, nearby populated proof files | `docs/governance/dependency-standards.md`, `docs/governance/handwritten-proof-vault-standards.md` |
| Validate or migrate proof stubs | `docs/workflows/proof-stub-invariant-migration.md`, `docs/governance/proof-standards.md` | `docs/governance/refactoring-standards.md`, `docs/architecture/volume-layout.md` |
| Generate or validate theorem routes | `docs/workflows/knowledge-extraction.md`, `docs/governance/extraction-standards.md` | `docs/architecture/knowledge-pipeline.md`, `docs/architecture/theorem-explorer-pipeline.md` |
| Refactor volume folders or source layout | `docs/governance/refactoring-standards.md`, `docs/architecture/volume-layout.md` | `docs/workflows/volume-cleanup.md`, `docs/governance/file-splitting-standards.md` |
| Sync generated wrappers or downstream docs | `docs/workflows/generated-wrapper-sync.md`, `docs/governance/agent-instruction-policy.md` | `docs/governance/repo-overlays/README.md`, the one relevant repo overlay |
| Edit shared LaTeX infrastructure | `docs/governance/repo-overlays/lra-common.md`, `docs/architecture/repository-layout.md` | `docs/governance/notation-standards.md`, `docs/governance/decoration-box-standards.md` |
| Work in a leaf volume repo | `docs/governance/repo-overlays/lra-volume.md`, `docs/architecture/volume-architecture.md` | task-specific workflow rows above |
| Work in `Learning-Real-Analysis` | `docs/governance/repo-overlays/learning-real-analysis.md`, `docs/architecture/multi-repo-sync.md` | `docs/architecture/generated-file-policy.md`, `docs/architecture/latex-build-and-rendering.md` |
| Memorialize handwritten proof artifacts | proof-vault `README.md`, proof-vault `docs/workflows/route-refactor-migration.md`, proof-vault `routing/theorem-routes.json` | `docs/governance/handwritten-proof-vault-standards.md` |
| Audit decoration boxes | `docs/workflows/decoration-audit.md`, `docs/governance/decoration-audit-standards.md` | `docs/governance/decoration-box-standards.md` |
| Add bibliography entries | `docs/workflows/bibliography-entry.md` | `docs/governance/authoring-standards.md` |

## Prompt Routes

Only load constitution prompts when the user is explicitly asking for that
prompted generation or audit mode.

| Prompted task | Prompt file |
| --- | --- |
| Generate a proof | `constitution/prompts/generate-proof.md` |
| Generate a theorem statement | `constitution/prompts/generate-statement.md` |
| Generate a stub chapter | `constitution/prompts/generate-stub-chapter.md` |
| Generate a stub volume | `constitution/prompts/generate-stub-volume.md` |
| Generate a capstone | `constitution/prompts/generate-capstone.md` |
| Generate breadcrumbs | `constitution/prompts/generate-breadcrumb.md` |
| Audit a proof | `constitution/prompts/audit-proof.md` |
| Audit a statement | `constitution/prompts/audit-statement.md` |
| Audit a stub | `constitution/prompts/audit-stub.md` |
| Audit chapter symbols | `constitution/prompts/audit-chapter-symbols.md` |
| Fix logical block gaps | `constitution/prompts/fix-logical-block-gaps.md` |
| Plan toolkits | `constitution/prompts/plan-toolkits.md` |

## Loading Discipline

1. Read `AGENTS.md`.
2. Read this task index.
3. Read the task's required files.
4. Inspect nearby source examples before editing.
5. Load optional files only when the required files leave a concrete question
   unresolved.

