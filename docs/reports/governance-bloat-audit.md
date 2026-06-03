# Governance Bloat Audit

Audit date: 2026-06-03

## Scope

This audit inspected the canonical governance repo only. It intentionally read
the full governance corpus as an audit exception:

- `AGENTS.md`
- `constitution/master.md`
- `docs/governance/**`
- `docs/architecture/**`
- `docs/workflows/**`
- `docs/governance/repo-overlays/**`
- `constitution/prompts/**`
- `docs/agent-task-index.md`
- existing structured files under `constitution/schema/` and
  `constitution/schemas/`

No large documentation sections were moved or rewritten in this phase.

## Authority Model

- `AGENTS.md` should remain a short router.
- The constitution answers what is valid.
- Architecture docs answer where things live and how repos, volumes, builds,
  and sync paths are organized.
- Governance docs state rules authored content must obey.
- Workflow docs state how an agent performs a task.
- Repo overlays add local constraints and must not fork global rules.
- The task index routes task types to the smallest sufficient reading set.

## Inventory

### Canonical Files

- `AGENTS.md`: root router for agents in the governance repo.
- `docs/agent-task-index.md`: task-to-document router. This is new local work
  and is not yet part of the upstream canonical branch until committed.
- `constitution/master.md`: currently mixes constitutional law, auditor
  loading details, repository map, canonical YAML ownership, file maps, and
  some content invariants.
- `constitution/schema/block-registry.yaml`: structured block identity,
  triggers, dependency parents, and block notes.
- `constitution/schema/artifact-matrix.yaml`: structured R/C/D/N requirement
  matrix by artifact type.
- `constitution/schema/file-schema.yaml`: structured filesystem, proof file,
  chapter stub, capstone, breadcrumb, and auditor path rules.
- `constitution/schemas/audit-report.json`: JSON response schema for audit
  calls.
- `docs/governance/*.md`: human-facing content standards.
- `docs/architecture/*.md`: repo, volume, sync, build, schema-location, and
  generated-file architecture.
- `docs/workflows/*.md`: task execution recipes.
- `docs/governance/repo-overlays/*.md`: repo-specific operational overlays.
- `constitution/prompts/*.md`: prompt bodies for generation and audit modes.

### Files That Look Redundant By Design

Within this repository, the redundancy is mostly cross-layer, not downstream
copy duplication. Downstream copies in `lra-common` and leaf repos are expected
for isolated agents and GitHub workflows, but those were outside this audit's
edit scope.

### Stale Or Misaligned Areas

- `docs/governance/handwritten-proof-vault-standards.md` still reads like an
  older theorem-map/backlink workflow in places. The proof vault now uses
  route snapshots as the authoritative import mechanism.
- `constitution/master.md` still contains repo map, ownership, root discovery,
  canonical YAML, file map, and loading order material that is more
  architecture/tooling than constitutional law.
- `constitution/schema/file-schema.yaml` duplicates the repo map and auditor
  path facts also present in architecture docs.
- Several workflow docs restate proof-stub invariants already governed by
  `docs/governance/proof-standards.md`.
- Prompt files repeat common prompt scaffolding headings and output-contract
  instructions.

## Corpus Measurements

The audit scanned 63 files in the requested corpus.

Largest files by estimated token weight:

| File | Chars | Words | Est. tokens |
| --- | ---: | ---: | ---: |
| `constitution/prompts/fix-logical-block-gaps.md` | 20,259 | 2,236 | 5,065 |
| `constitution/prompts/generate-statement.md` | 15,804 | 2,115 | 3,951 |
| `constitution/master.md` | 15,225 | 2,112 | 3,806 |
| `docs/governance/model-standards.md` | 12,547 | 1,672 | 3,137 |
| `constitution/prompts/audit-statement.md` | 10,350 | 1,461 | 2,588 |
| `docs/governance/decoration-box-standards.md` | 7,288 | 938 | 1,822 |
| `constitution/prompts/audit-chapter-symbols.md` | 6,459 | 921 | 1,615 |
| `docs/governance/handwritten-proof-vault-standards.md` | 5,813 | 743 | 1,453 |
| `docs/governance/extraction-standards.md` | 5,550 | 711 | 1,388 |
| `docs/governance/stub-chapter-standards.md` | 5,452 | 791 | 1,363 |

Repeated heading sanity check:

- `Role` appears in 11 prompt files.
- `Output Encoding and TeX Notation` appears in 10 prompt files.
- `Input` appears in 9 prompt files.
- `Agent Scope` appears in 7 repo overlay files.
- `Output Format` appears in 6 prompt files.
- `Purpose` appears in 5 files.

These repeated prompt headings are mostly expected, but they are candidates for
a shared prompt preamble or prompt-builder composition if context pressure
becomes severe.

## Duplication Findings

### Proof Stub Invariant

Repeated theme:
Every proof-bearing theorem-like statement needs a proof stub; proof stubs
carry labels, `\LRAProofFor{...}`, theorem return links, restatement, two proof
bodies, proof structure, and dependencies.

Locations:

- `docs/governance/proof-standards.md`
- `docs/workflows/add-theorem-with-proof-stub.md`
- `docs/workflows/populate-proof-stub.md`
- `docs/workflows/proof-stub-invariant-migration.md`
- `constitution/schema/file-schema.yaml`

Canonical home:

- Human rule: `docs/governance/proof-standards.md`
- Structured rule: `constitution/schema/file-schema.yaml`, possibly later
  mirrored or migrated under `docs/schemas/`
- Workflow procedure: workflow docs

Recommended replacement:

Workflow docs should keep steps and validation commands, but replace repeated
full proof-file checklists with:

> Proof-stub structure is governed by `docs/governance/proof-standards.md` and
> the structured proof-file rule in `constitution/schema/file-schema.yaml`.

### Canonical YAML Ownership

Repeated theme:
`predicates.yaml`, `notation.yaml`, and `relations.yaml` live in
`Learning-Real-Analysis` and are not owned by leaf repos.

Locations:

- `constitution/master.md`
- `docs/architecture/canonical-yaml.md`
- `docs/architecture/knowledge-pipeline.md`
- `docs/architecture/latex-build-and-rendering.md`
- `docs/architecture/repository-layout.md`
- `docs/governance/notation-standards.md`
- multiple repo overlays

Canonical home:

- Architecture: `docs/architecture/canonical-yaml.md`
- Operational boundary: the relevant repo overlay

Recommended replacement:

In `constitution/master.md`, keep only a short source-authority principle and
point to `docs/architecture/canonical-yaml.md` for storage/discovery details.

### Repository Map And Sync Ownership

Repeated theme:
Repository ownership, sync direction, monorepo role, `lra-common` ownership,
and volume ownership.

Locations:

- `constitution/master.md`
- `constitution/schema/file-schema.yaml`
- `docs/architecture/repository-layout.md`
- `docs/architecture/multi-repo-sync.md`
- `docs/architecture/latex-build-and-rendering.md`
- `docs/governance/repo-overlays/*.md`

Canonical home:

- Ownership map: `docs/architecture/repository-layout.md`
- Sync behavior: `docs/architecture/multi-repo-sync.md`
- Local operational narrowing: repo overlays

Recommended replacement:

Remove detailed repository tables from `constitution/master.md` in a later
mechanical move and leave:

> Repository ownership and sync topology are architecture facts. See
> `docs/architecture/repository-layout.md` and
> `docs/architecture/multi-repo-sync.md`.

### Generated Wrapper Policy

Repeated theme:
Generated downstream instruction files are derived artifacts, downstream edits
are emergency-only, and repairs must be ported upstream.

Locations:

- `AGENTS.md`
- `docs/governance/agent-instruction-policy.md`
- `docs/architecture/generated-file-policy.md`
- `docs/workflows/generated-wrapper-sync.md`
- `docs/governance/repo-overlays/README.md`

Canonical home:

- Policy: `docs/governance/agent-instruction-policy.md`
- Artifact architecture: `docs/architecture/generated-file-policy.md`
- Operational steps: `docs/workflows/generated-wrapper-sync.md`

Recommended replacement:

Keep `AGENTS.md` short. It should point to task index and generated-wrapper
policy, not restate sync implementation details.

### Decoration And Logical Block Requirements

Repeated theme:
Box eligibility, logical block ordering, toolkit boxes, formal statement
blocks, dependencies, examples/non-examples, interpretation, predicate reading,
and audit boundaries.

Locations:

- `constitution/master.md`
- `constitution/schema/block-registry.yaml`
- `constitution/schema/artifact-matrix.yaml`
- `docs/governance/decoration-box-standards.md`
- `docs/governance/decoration-audit-standards.md`
- `docs/governance/atomic-artifact-standards.md`
- `constitution/prompts/audit-statement.md`
- `constitution/prompts/generate-statement.md`

Canonical home:

- Structured block identity and matrix:
  `constitution/schema/block-registry.yaml` and
  `constitution/schema/artifact-matrix.yaml`
- Human explanation: `docs/governance/decoration-box-standards.md` and
  `docs/governance/atomic-artifact-standards.md`
- Prompt execution: only prompt-specific instructions

Recommended replacement:

Prompts should receive generated excerpts from structured data rather than
hand-copying the same checklist when possible. Markdown standards should point
to the matrix for machine-checkable presence/order rules.

### Stub Chapter And Stub Section Requirements

Repeated theme:
Stub chapter/section marker requirements, allowed placeholder style, build
rules, and reporting requirements.

Locations:

- `docs/governance/stub-chapter-standards.md`
- `docs/governance/stub-section-standards.md`
- `docs/workflows/volume-cleanup.md`
- `constitution/schema/file-schema.yaml`
- `constitution/prompts/audit-stub.md`
- `constitution/prompts/generate-stub-chapter.md`
- `constitution/prompts/generate-stub-volume.md`

Canonical home:

- Structured file requirements: `constitution/schema/file-schema.yaml`
- Human standards: stub governance docs
- Procedure: workflows and prompts

Recommended replacement:

Move reusable required-marker lists into structured data; keep prose for
judgment calls such as when a stub is appropriate.

### Handwritten Proof Vault Workflow

Repeated theme:
Proof vault is not canonical mathematical source; handwritten records attach to
canonical proofs; backlinks and metadata are required.

Locations:

- `docs/governance/handwritten-proof-vault-standards.md`
- `docs/governance/proof-standards.md`
- `docs/governance/extraction-standards.md`
- proof-vault `README.md` and proof-vault workflow docs outside this repo

Canonical home:

- Current operational workflow belongs in `lra-proof-vault`.
- Governance should state only cross-repo invariants and privacy/sanitization
  rules.

Recommended replacement:

Revise `handwritten-proof-vault-standards.md` after route-source workflow
stabilizes. It should refer to route snapshots rather than legacy theorem-map
language.

## Misplaced-Material Move Map

| Current location | Material | Proposed target | Reason | Move class |
| --- | --- | --- | --- | --- |
| `constitution/master.md` | Repository map table | `docs/architecture/repository-layout.md` | Repo ownership is architecture, not validity law. | Mechanical |
| `constitution/master.md` | Auditor root discovery and `REPO_ROOT` usage | `docs/architecture/canonical-yaml.md` or auditor README | Tool path discovery is tooling/architecture. | Mechanical with path review |
| `constitution/master.md` | `common/` ownership | `docs/architecture/repository-layout.md` and `docs/governance/repo-overlays/lra-common.md` | Ownership/sync fact belongs outside constitution. | Mechanical |
| `constitution/master.md` | Governance ownership and generated downstream copies | `docs/governance/agent-instruction-policy.md` | This is wrapper/sync policy. | Mechanical |
| `constitution/master.md` | File map and prompt loading order | `docs/architecture/generated-file-policy.md` or constitution auditor README | Operational loader documentation, not content law. | Needs human review |
| `constitution/master.md` | Canonical YAML storage list | `docs/architecture/canonical-yaml.md` | Storage/discovery fact. | Mechanical |
| `constitution/schema/file-schema.yaml` | Multi-repo layout comments | `docs/architecture/repository-layout.md` | Comments duplicate architecture. Keep only schema fields or a pointer. | Needs human review |
| `constitution/schema/file-schema.yaml` | Auditor path rules | auditor README or `docs/architecture/canonical-yaml.md` | Tool-specific discovery. | Needs human review |
| `docs/governance/handwritten-proof-vault-standards.md` | Commit/push workflow and backlink procedure | proof-vault workflows | Operational procedure belongs in proof-vault workflow docs. | Needs human review |
| `docs/workflows/*.md` | Full proof/stub rule restatements | `docs/governance/proof-standards.md` and schema pointers | Workflows should route, not restate full standards. | Mechanical |

## Schema Extraction Candidates

Existing structured locations are already present under `constitution/schema/`.
Prefer extending or migrating these before creating an unrelated
`data/governance-rules.yaml`.

| Rule | Source doc | Proposed structured location | Status | Validator/audit |
| --- | --- | --- | --- | --- |
| Theorem-like artifact categories (`def`, `thm`, `lem`, `prop`, `cor`, `ax`) | `artifact-matrix.yaml`, `block-registry.yaml`, prompts | Existing `constitution/schema/artifact-matrix.yaml` | enforced/auditable | `constitution/auditor` statement audit |
| Required statement blocks | `block-registry.yaml`, `artifact-matrix.yaml`, `decoration-box-standards.md` | Existing matrix/registry | enforced/auditable | statement audit |
| Label prefixes | `block-registry.yaml`, proof standards, prompts | New `label-rules` section in structured governance data or existing schema | auditable | deterministic label scanner |
| Proof file layer order | `file-schema.yaml`, `proof-standards.md` | Existing `constitution/schema/file-schema.yaml` | auditable/enforced in leaf validators | `scripts/validate_leaf_proofs.py` in leaf repos |
| `\LRAProofFor{...}` proof association | `proof-standards.md`, workflows | Existing proof-file schema or new proof rule data | enforced in leaf validators | `validate_leaf_proofs.py` |
| Proof body section requirements | `proof-standards.md`, `file-schema.yaml` | Existing proof-file schema | enforced in leaf validators | `validate_leaf_proofs.py` |
| Box eligibility rules | `block-registry.yaml`, `decoration-box-standards.md` | Existing block registry trigger fields | auditable | statement/decoration audit |
| One-toolkit-box-per-section | `block-registry.yaml`, `decoration-box-standards.md` | Existing block registry plus toolkit audit config | auditable | `constitution/auditor/audits/toolkits.py` |
| Breadcrumb placement | `constitution/master.md`, `file-schema.yaml` | Existing file schema, possibly split into `breadcrumb-rules` | auditable | stub audit |
| Stub chapter required markers | `stub-chapter-standards.md`, `file-schema.yaml` | Existing file schema | auditable | stub audit |
| Stub section required markers | `stub-section-standards.md` | Add structured section-stub schema | documented/auditable | future stub-section validator |
| Dependency target prefixes | `dependency-standards.md`, `proof-standards.md` | New dependency/link rules section | auditable | generated-block validator |
| Proof-vault backlink placement | `proof-standards.md`, `extraction-standards.md` | Proof-file schema or proof-vault route schema | auditable | leaf/proof-vault validators |

Rules intentionally left prose-only:

- Whether a negated statement is pedagogically useful.
- Whether interpretation is already supplied clearly enough nearby.
- Whether a theorem is structurally dominant enough to deserve a box.
- Mathematical taste judgments about exposition quality.
- Whether a refactor reveals a missing mathematical object that should be
  introduced.

## Task-Index Review

`docs/agent-task-index.md` exists locally and already implements the local
multi-repo versus isolated-checkout rule. It needs expansion to carry output
artifacts and validation checks per route.

Recommended routes:

| Task | Required docs | Optional docs | Output artifacts | Validation |
| --- | --- | --- | --- | --- |
| Add theorem | `docs/workflows/add-theorem-with-proof-stub.md`, `docs/governance/proof-standards.md` | dependency and atomic-artifact standards | statement file, proof stub | `validate_leaf_proofs.py --strict`, volume build |
| Add definition | authoring, atomic-artifact, notation standards | model standards, dependency standards | notes file change | volume build, extractor if available |
| Add proof | `populate-proof-stub.md`, proof standards | dependency standards, nearby proofs | existing proof file changed | `validate_leaf_proofs.py --strict`, build |
| Populate proof stub | same as add proof | proof-vault standards if backlink involved | proof body in place | leaf validator/build |
| Refactor chapter | refactoring standards, volume layout | file-splitting, multi-repo sync | moved/renamed source files, updated indexes | build, extractor, route diff |
| Create stub chapter | stub-chapter standards, file schema | volume layout | chapter skeleton | build or documented no-build reason |
| Create stub section | stub-section standards, file schema | volume layout | section notes/proofs/index wiring | build |
| Extract knowledge | knowledge-extraction workflow, extraction standards | knowledge pipeline | build knowledge artifacts | extractor validation |
| Update predicates/relations/notation | notation standards, canonical-yaml architecture | chapter symbol audit prompt | YAML changes or audit report | schema/YAML parse, symbol audit |
| Memorialize handwritten proof | proof-vault README/workflow, route snapshot | handwritten proof-vault standards | copied attempt, metadata | proof-vault validator |
| Audit governance | governance audit workflow, this report, task index | schema docs | audit report | governance audit script when created |

## Proposed Commit Plan

1. Router/index hardening:
   - `AGENTS.md`
   - `docs/agent-task-index.md`
   - optional `docs/workflows/governance-audit.md`

2. Structured-rule extraction:
   - extend or migrate `constitution/schema/*`
   - add validation of rule IDs, required fields, and path references
   - add short schema README updates

3. Conservative mechanical consolidation:
   - move repository map and canonical YAML storage details out of
     `constitution/master.md`
   - replace workflow checklist duplication with pointers
   - update prompt scaffolding only if composition tooling supports it

4. Proof-vault standards refresh:
   - update handwritten proof-vault governance to route-snapshot language
   - keep privacy/sanitization invariants in governance
   - keep operational memorialization workflow in proof vault

5. Repeatable bloat audit tooling:
   - add a script that measures chars/words/estimated tokens, duplicate
     normalized content, repeated headings, task-index path validity, and
     structured-rule validity.

## High-Priority Consolidation Targets

1. `constitution/master.md`: remove mechanical architecture/tooling sections
   after adding pointers.
2. `docs/governance/handwritten-proof-vault-standards.md`: update to current
   route-snapshot proof-vault design.
3. Proof-stub workflow docs: keep task steps but point detailed invariant
   checks to proof standards/schema.
4. Prompt common scaffolding: consider shared preamble/composition only after
   prompt-builder support exists.
5. Structured rule data: extend existing `constitution/schema/` rather than
   creating a second authority system.

## Unresolved Ambiguity

- Whether `constitution/master.md` should remain the loader-facing index for
  the old Python auditor or be split into a content constitution plus an
  auditor README.
- Whether structured governance rules should remain in `constitution/schema/`
  or migrate/mirror to `docs/schemas/`.
- Whether route-snapshot proof-vault rules belong partly in governance or
  entirely in `lra-proof-vault` with only high-level invariants in governance.
- Whether prompt scaffolding should be deduplicated in source files or left
  duplicated for standalone readability.

## Validation And Sanity Checks

Commands run:

```powershell
rg --files AGENTS.md constitution docs -g "*.md" -g "*.yaml" -g "*.yml" -g "*.json"
```

```powershell
rg -n "markdown|link|schema|audit|validate|yamllint|jsonschema|py_compile|pytest" -g "*.py" -g "*.ps1" -g "*.yml" -g "*.yaml" -g "*.md" scripts tools constitution docs .github -S
```

```powershell
python - <<'PY'
# scanned requested files, counted chars/words/headings,
# reported repeated headings, key phrase repetition, and largest files
PY
```

The search found governance wrapper validation tools and constitution auditor
tools, but no dedicated markdown link checker or governance-bloat audit script.
Full link validation was not run. Formal schema validation against a schema
validator was not run in this phase.

Additional checks run after drafting:

```powershell
python - <<'PY'
# parsed constitution/schema/*.yaml and constitution/schemas/audit-report.json
PY
```

Result: existing YAML and JSON schema files parsed successfully.

```powershell
python - <<'PY'
# extracted markdown/yaml/json references from docs/agent-task-index.md and
# checked whether local lra-governance paths exist
PY
```

Result: 48 references checked; 2 were reported missing because they are
proof-vault external references rather than `lra-governance` paths:

- `docs/workflows/route-refactor-migration.md`
- `routing/theorem-routes.json`

Follow-up: the task index should mark external repo references explicitly so a
future path validator can distinguish local governance paths from cross-repo
task inputs.
