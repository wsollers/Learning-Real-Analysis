# LRA Governance Design Index

This file is a router. It is not the full governance manual.

Canonical rule sources live under:

- `docs/governance/`
- `docs/architecture/`
- `docs/governance/repo-overlays/`
- `docs/workflows/`

Generated agent wrappers are derived from these modular sources.

Do not add large policy blocks directly to `DESIGN.md`. Add or update the
appropriate modular document instead.

## Canonical Source Directories

| Directory | Purpose |
| --- | --- |
| `docs/governance/` | Authoring, dependency, decoration-box, model presentation, proof, extraction, notation, generated-agent, task-scope, refactoring, and validation standards. |
| `docs/governance/repo-overlays/` | Additive repository and repository-family overlays. |
| `docs/architecture/` | Ownership, repository layout, generated-file policy, sync architecture, build/render architecture, canonical YAML, and knowledge pipeline architecture. |
| `docs/workflows/` | Operational workflows for extraction, cleanup, wrapper sync, and audit/report phases. |
| `docs/archive/` | Retired or obsolete design assumptions retained for historical reference when needed. |

## Task Router

| Task or question | Canonical document |
| --- | --- |
| Authoring theorem, definition, axiom, or chapter content | `docs/governance/authoring-standards.md` |
| Box decisions and load-bearing structural items | `docs/governance/authoring-standards.md` |
| Shared dependency information for definitions, theorem-like environments, proofs, stubs, and structural records | `docs/governance/dependency-standards.md` |
| Standardized surrounding logical and explanatory blocks for formal artifacts | `docs/governance/decoration-box-standards.md` |
| Structural presentations for signatures, languages, models, classifications, and blueprints | `docs/governance/model-standards.md` |
| Handwritten proof vault policy and memorialized proof artifacts | `docs/governance/handwritten-proof-vault-standards.md` |
| Exercise vault policy and memorialized exercise artifacts | `docs/governance/exercise-vault-standards.md` |
| Formal notation, predicate readings, and no predicate leakage | `docs/governance/notation-standards.md` |
| Proof file architecture, proof labels, and theorem/proof navigation | `docs/governance/proof-standards.md` |
| Atomic definition and atomic figure invariants | `docs/governance/atomic-artifact-standards.md` |
| Extraction implementation for dependency labels, proof-vault URLs, and theorem explorer records | `docs/governance/extraction-standards.md` |
| File splitting, one-artifact-per-file rules, and structural stability | `docs/governance/file-splitting-standards.md` |
| Refactoring safety and high-fidelity cleanup | `docs/governance/refactoring-standards.md` |
| Stub chapter generation during governance rebuilds | `docs/governance/stub-chapter-standards.md` |
| Stub section generation during governance rebuilds | `docs/governance/stub-section-standards.md` |
| Decoration audit expectations | `docs/governance/decoration-audit-standards.md` |
| Agent instruction generation and provider wrappers | `docs/governance/agent-instruction-policy.md` |
| Task limits, stop conditions, and protected local paths | `docs/governance/task-scope-limits.md` |
| Repository ownership and source-of-truth map | `docs/architecture/repository-layout.md` |
| Volume repository layout and Overleaf shape | `docs/architecture/volume-layout.md` |
| Shared LaTeX infrastructure ownership | `docs/architecture/repository-layout.md` and `docs/governance/repo-overlays/lra-common.md` |
| Canonical YAML ownership | `docs/architecture/canonical-yaml.md` |
| Knowledge graph and theorem explorer pipeline | `docs/architecture/knowledge-pipeline.md` and `docs/architecture/theorem-explorer-pipeline.md` |
| LaTeX build and rendering expectations | `docs/architecture/latex-build-and-rendering.md` |
| Numerical-analysis software workbench tasks | `docs/governance/repo-overlays/lra-numerical-analysis.md` |
| Multi-repo sync boundaries | `docs/architecture/multi-repo-sync.md` |
| Generated file headers, drift, and full-replace policy | `docs/architecture/generated-file-policy.md` |
| Knowledge extraction workflow | `docs/workflows/knowledge-extraction.md` |
| Bibliography entry workflow | `docs/workflows/bibliography-entry.md` |
| Exercise memorialization workflow | `docs/workflows/exercise-vault-memorialization.md` |
| Volume cleanup workflow | `docs/workflows/volume-cleanup.md` |
| Generated wrapper sync workflow | `docs/workflows/generated-wrapper-sync.md` |
| Decoration audit workflow | `docs/workflows/decoration-audit.md` |

## Repository Overlay Router

Use exactly one repo overlay or repo-family overlay when generating or
interpreting repo-local agent rules.

| Repository | Overlay |
| --- | --- |
| `Learning-Real-Analysis` | `docs/governance/repo-overlays/learning-real-analysis.md` |
| `lra-common` | `docs/governance/repo-overlays/lra-common.md` |
| `lra-volume-i` through `lra-volume-viii` | `docs/governance/repo-overlays/lra-volume.md` |
| `lra-lean` | `docs/governance/repo-overlays/lra-lean.md` |
| `lra-nurbs` | `docs/governance/repo-overlays/lra-nurbs.md` |
| `lra-knowledge-explorer` | `docs/governance/repo-overlays/lra-knowledge-explorer.md` |
| `lra-numerical-analysis` | `docs/governance/repo-overlays/lra-numerical-analysis.md` |
| `lra-pdf-extractor` | `docs/governance/repo-overlays/lra-pdf-extractor.md` |

Repo overlays are additive. They refine the global rules for owned work, but
they do not fork or weaken global governance.

## Non-Negotiable Anchors

- Governance docs in `lra-governance` are canonical.
- Generated downstream files are not canonical.
- Downstream generated wrapper edits are emergency-only and must be ported
  upstream before the next sync.
- Generated wrapper sync is controlled, repo-selected, drift-checked, and
  full-replace only after review.
- Volume repos must not receive positive Lean, NURBS/Vulkan,
  numerical-analysis, or PDF-extractor specialist rules.
- `Learning-Real-Analysis/scripts/` is protected and out of scope for
  governance migration.
- Canonical YAML remains owned by `Learning-Real-Analysis`.
- Shared LaTeX infrastructure remains owned by `lra-common`.
- Local model and Ollama output is advisory only.
- Do not invent predicates, labels, dependencies, or mathematical content.

## Maintenance Rule

When a new governance rule is added:

1. Put the rule in the smallest applicable modular document.
2. Update the relevant README or router if discovery would otherwise be hard.
3. Update repo overlays only when the rule changes repo-specific behavior.
4. Regenerate previews or run validation only when generated wrappers are in
   scope.

Large design narratives, obsolete assumptions, and historical notes belong in
`docs/archive/` or in a migration report, not in this router.
