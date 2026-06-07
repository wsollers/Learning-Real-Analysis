# Agent Task Index

Repository scope: `Learning-Real-Analysis` is read-only by default unless the
user explicitly asks to modify it; workflows sync changes from the leaf
`lra-*` repositories into that aggregate monorepo.

Governance scope: `lra-governance` is the master governance and script
repository; shared scripts, rules, prompts, schemas, and workflows belong here,
and commit syncs copy those rules into the leaf repositories.

This index tells agents which authority to load for common LRA tasks. It is a
router, not a replacement for the referenced standards.

## Repository Scope Rule

When working from a local multi-repo checkout such as `F:\repos`, prefer the
canonical files in `lra-governance` for shared governance, architecture,
workflow, prompt, schema, tool, and overlay rules.

When an agent is running inside an isolated repository checkout, such as a
GitHub workflow or a single leaf repo clone without adjacent `lra-governance`,
use that repo's synced local copies under `docs/`.

Do not read every governance, workflow, architecture, schema, and prompt file by
default. Select the smallest file set that matches the task.

External repository references are marked with `[external:<repo>]`. They are
task inputs, not local `lra-governance` files.

## Loading Discipline

1. Read `AGENTS.md`.
2. Read this task index.
3. Read only the task's required docs, schema/data files, and tool help.
4. Inspect nearby source examples before editing.
5. Load optional files only when required files leave a concrete question
   unresolved.
6. Load constitution prompts only when the user explicitly asks for that
   prompted generation or audit mode.
7. Full-corpus governance reading is reserved for explicit governance audit or
   consolidation tasks.

## Schema Authority

Machine-checkable layout and artifact rules should live in structured schema
or data files. Prose docs should point to them instead of restating them.

- Statement and decoration block identity:
  `constitution/schema/block-registry.yaml`
- Required block matrix by artifact type:
  `constitution/schema/artifact-matrix.yaml`
- Volume, chapter, topic, proof folder, proof file, stub, capstone, and
  breadcrumb layout:
  `constitution/schema/file-schema.yaml`
- Audit response shape:
  `constitution/schemas/audit-report.json`

## Task Routes

| Task | Required docs | Required schema/data | Required tools | Optional docs | Output artifacts | Validation checks |
| --- | --- | --- | --- | --- | --- | --- |
| Generate chapter artifacts from payload | `docs/workflows/artifact-payload-generation.md`, `docs/governance/atomic-artifact-standards.md`, `docs/governance/notation-standards.md`, `docs/governance/dependency-standards.md` | ordered JSON/JSONL payload, appendable artifact registries, canonical predicates/notation/relations YAML | `tools/import_artifact_payload.py`, `tools/chapter_artifact.py`, deterministic local audit commands | `docs/workflows/proof-layout-audit.md` | payload file, appendable YAML registry, generated notation page, generated LaTeX blocks, proof stubs, chapter manifest, audit report | importer dry-run/write, artifact validate, true-up, box-color audit, proof-layout audit, latexmk build, local registry/symbol check when available without AI |
| Add theorem with proof stub | `docs/workflows/add-theorem-with-proof-stub.md`, `docs/governance/proof-standards.md`, `docs/governance/dependency-standards.md` | `constitution/schema/file-schema.yaml`, `constitution/schema/artifact-matrix.yaml` | leaf `scripts/validate_leaf_proofs.py`, `tools/governance/audit_proof_layout.py` | `docs/governance/atomic-artifact-standards.md`, `docs/architecture/volume-layout.md` | theorem/proposition/lemma/corollary source, canonical proof stub | leaf proof validator, proof layout audit, volume build |
| Add definition | `docs/governance/authoring-standards.md`, `docs/governance/atomic-artifact-standards.md`, `docs/governance/notation-standards.md`, `docs/governance/dependency-standards.md` | `constitution/schema/artifact-matrix.yaml`, `constitution/schema/block-registry.yaml` | volume build or local extractor when available | `docs/governance/model-standards.md` | definition source and any notation/predicate updates | YAML parse when data changes, volume build, extractor audit when available |
| Add proof or populate existing proof stub | `docs/workflows/populate-proof-stub.md`, `docs/governance/proof-standards.md`, `docs/governance/dependency-standards.md`, nearby populated proof files | `constitution/schema/file-schema.yaml` | leaf `scripts/validate_leaf_proofs.py`, `tools/governance/audit_proof_layout.py` | `docs/governance/handwritten-proof-vault-standards.md` | existing proof file populated in place | leaf proof validator, proof layout audit, volume build |
| Validate or migrate proof stubs | `docs/workflows/proof-stub-invariant-migration.md`, `docs/workflows/proof-layout-audit.md`, `docs/governance/proof-standards.md` | `constitution/schema/file-schema.yaml` | `tools/governance/audit_proof_layout.py`, leaf `scripts/validate_leaf_proofs.py` | `docs/governance/refactoring-standards.md`, `docs/architecture/volume-layout.md` | compliance report or targeted stub edits | proof audit with `--strict` or explicit `--refactor-mode`, leaf validator |
| Audit proof layout | `docs/workflows/proof-layout-audit.md`, `docs/governance/proof-standards.md` | `constitution/schema/file-schema.yaml` | `tools/governance/audit_proof_layout.py` | nearby proof examples | compliant/non-compliant proof list | `python tools\governance\audit_proof_layout.py --root <target> --format json` |
| Generate or validate theorem routes | `docs/workflows/knowledge-extraction.md`, `docs/governance/extraction-standards.md`, `docs/governance/dependency-standards.md` | `constitution/schema/file-schema.yaml` plus route schema in the target leaf repo | leaf `scripts/generate_theorem_routes.py`, leaf `scripts/validate_leaf_proofs.py` | `docs/architecture/knowledge-pipeline.md`, `docs/architecture/theorem-explorer-pipeline.md` | theorem route JSON/YAML and route diff artifacts | route generator, route validate-only, leaf proof validator, build wrapper |
| Refactor chapter | `docs/governance/refactoring-standards.md`, `docs/architecture/volume-layout.md` | `constitution/schema/file-schema.yaml` | `tools/governance/audit_volume_layout.py`, `tools/governance/audit_proof_layout.py` | `docs/workflows/volume-cleanup.md`, `docs/governance/file-splitting-standards.md` | moved source/proof files, updated indexes, regenerated route metadata, route diff | volume layout audit, proof layout audit, build, theorem-route regeneration/validation when paths move |
| Refactor volume folders or source layout | `docs/governance/refactoring-standards.md`, `docs/architecture/volume-layout.md`, `docs/workflows/volume-layout-audit.md` | `constitution/schema/file-schema.yaml` | `tools/governance/audit_volume_layout.py`, `tools/governance/audit_proof_layout.py` | `docs/architecture/multi-repo-sync.md`, `docs/governance/file-splitting-standards.md` | topic-mirrored notes/proofs layout, updated routers, regenerated route metadata, migration report | volume layout audit with `--refactor-mode` during migration and `--strict` after; theorem-route regeneration/validation when paths move |
| Create stub chapter | `docs/governance/stub-chapter-standards.md`, `docs/architecture/volume-layout.md` | `constitution/schema/file-schema.yaml` | `tools/governance/audit_volume_layout.py` | `docs/workflows/volume-cleanup.md` | chapter skeleton, chapter metadata, notes/proofs routers | volume layout audit, build or documented no-build reason |
| Create stub section | `docs/governance/stub-section-standards.md`, `docs/architecture/volume-layout.md` | `constitution/schema/file-schema.yaml` | `tools/governance/audit_volume_layout.py` | `docs/governance/stub-chapter-standards.md` | paired `notes/{topic}/` and `proofs/{topic}/` folders and routers | volume layout audit, build |
| Memorialize exercise artifacts | `docs/workflows/exercise-vault-memorialization.md`, `docs/governance/exercise-vault-standards.md` | `exercise-ledger.yaml` in the owning chapter | YAML parser, route existence checks, build command | `docs/governance/refactoring-standards.md`, `docs/architecture/volume-layout.md` | copied source photo when present, TeX exercise set, updated ledger, regenerated reports, exercise index routing | ledger parse, duplicate-ID check, route check, build |
| Extract knowledge | `docs/workflows/knowledge-extraction.md`, `docs/governance/extraction-standards.md`, `docs/governance/dependency-standards.md` | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml`, target repo route schema | target extractor scripts | `docs/architecture/knowledge-pipeline.md`, `docs/architecture/canonical-yaml.md` | knowledge artifacts and route artifacts | extractor validation, schema parse, route validation |
| Update predicates, relations, or notation | `docs/governance/notation-standards.md`, `docs/architecture/canonical-yaml.md` | canonical YAML files in the owning repo | YAML parser, symbol audit when available | `docs/governance/dependency-standards.md`, `constitution/prompts/audit-chapter-symbols.md` | canonical YAML changes or audit report | YAML parse, extractor/build if affected |
| Sync generated wrappers or downstream docs | `docs/workflows/generated-wrapper-sync.md`, `docs/governance/agent-instruction-policy.md` | wrapper manifest/config files when present | `tools/governance/generate_agent_wrappers.py`, `tools/governance/report_wrapper_drift.py`, `tools/governance/sync_agent_wrappers.py`, `tools/governance/validate_repo_rules.py` | `docs/governance/repo-overlays/README.md`, exactly one relevant overlay | generated wrapper previews or synced downstream docs | drift report, repo-rule validation, dry-run before write |
| Edit shared LaTeX infrastructure | `docs/governance/repo-overlays/lra-common.md`, `docs/architecture/repository-layout.md` | affected macro/schema docs if any | target repo build/tests | `docs/governance/notation-standards.md`, `docs/governance/decoration-box-standards.md` | macro/package changes in `lra-common` | compile target volumes or affected smoke build |
| Work in `lra-source-profiles` | `docs/governance/repo-overlays/lra-source-profiles.md`, `[external:lra-source-profiles] README.md` | source manifests, `volumes/<volume>/<chapter>/source-index.yaml`, `active-sources.yaml`, named-profile indexes, active-profile index | `[external:lra-source-profiles] scripts/validate_source_indexes.py`, relevant local source-profile scripts | `docs/architecture/repository-layout.md`, `docs/governance/repo-overlays/lra-pdf-extractor.md` only when comparing ingestion boundaries | source profile metadata, active-profile exports, category placements, review queues, markdown cache | source index validation, YAML parse, no destructive PDF moves, no direct downstream note/bibliography/YAML edits |
| Work in a leaf volume repo | `docs/governance/repo-overlays/lra-volume.md`, `docs/architecture/volume-architecture.md` | task-specific schema rows above | task-specific validators above | relevant workflow row for the task | leaf source, proof, route, or build artifacts | leaf build wrapper and task-specific validators |
| Work in `Learning-Real-Analysis` | `docs/governance/repo-overlays/learning-real-analysis.md`, `docs/architecture/multi-repo-sync.md` | integration repo manifests/artifacts only | integration build/sync checks | `docs/architecture/generated-file-policy.md`, `docs/architecture/latex-build-and-rendering.md` | integration-only changes unless explicitly authorized | verify canonical source repo ownership before editing content |
| Memorialize handwritten proof artifacts | `[external:lra-proof-vault] README.md`, `[external:lra-proof-vault] routing/theorem-routes.json`, `[external:lra-proof-vault] docs/workflows/route-refactor-migration.md` | route snapshot in `[external:lra-proof-vault] routing/` | `[external:lra-proof-vault] scripts/memorialize_attempt.py`, `[external:lra-proof-vault] scripts/validate_vault.py` | `docs/governance/handwritten-proof-vault-standards.md` | copied attempt file and vault metadata | proof-vault validator, exact route match or explicit user choice |
| Audit decoration boxes | `docs/workflows/decoration-audit.md`, `docs/governance/decoration-audit-standards.md` | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml` | `tools/governance/audit_latex_decoration.py` when available | `docs/governance/decoration-box-standards.md` | decoration inventory/audit report | decoration audit, statement audit when available |
| Add bibliography entries | `docs/workflows/bibliography-entry.md` | bibliography layout docs/data in target repo | bibliography build/check scripts when present | `docs/governance/authoring-standards.md` | `.bib` entries or split bibliography files | BibTeX/biber parse or target build |
| Audit governance | `docs/workflows/governance-audit.md`, `docs/reports/governance-bloat-audit.md`, this index | `constitution/schema/*.yaml`, `constitution/schemas/*.json` | grep/path/schema sanity scripts | full corpus only as explicit audit exception | audit report or consolidation plan | schema parse, task-index path check, repeated-heading/phrase scan |
| Shrink constitution or move architecture facts | `docs/reports/governance-bloat-audit.md`, `constitution/master.md`, `docs/architecture/repository-layout.md`, `docs/architecture/multi-repo-sync.md` | relevant schema files only where pointers need updates | grep/path sanity checks | affected governance docs | mechanical move plan or small pointer edits | no authority loss, no new duplicate documents |
| Reduce workflow duplication | `docs/reports/governance-bloat-audit.md`, target workflow, canonical governance standard | relevant schema authority for the repeated rule | grep repeated phrases/headings | nearby workflow docs for consistency | shortened workflow with pointers | confirm workflow still has steps, outputs, validation |
| Make prompts consume schema | `docs/reports/governance-bloat-audit.md`, target prompt file | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml`, `constitution/schema/file-schema.yaml` as applicable | prompt/schema reference sanity checks | prompt-builder docs if present | prompt with schema pointers or generated schema excerpt plan | prompt still names required inputs and output contract |

## Prompt Routes

Only load constitution prompts when the user is explicitly asking for that
prompted generation or audit mode.

| Prompted task | Prompt file | Schema/data inputs | Deterministic tools/checks |
| --- | --- | --- | --- |
| Generate a proof | `constitution/prompts/generate-proof.md` | `constitution/schema/file-schema.yaml`, `docs/governance/dependency-standards.md` | proof layout audit after writing |
| Generate a theorem statement | `constitution/prompts/generate-statement.md` | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml`, `docs/governance/dependency-standards.md` | statement/block audit when available |
| Generate a stub chapter | `constitution/prompts/generate-stub-chapter.md` | `constitution/schema/file-schema.yaml` | volume layout audit |
| Generate a stub volume | `constitution/prompts/generate-stub-volume.md` | `constitution/schema/file-schema.yaml` | volume layout audit |
| Generate a capstone | `constitution/prompts/generate-capstone.md` | `constitution/schema/file-schema.yaml`, `constitution/schema/artifact-matrix.yaml` | volume build or chapter audit |
| Generate breadcrumbs | `constitution/prompts/generate-breadcrumb.md` | `constitution/schema/file-schema.yaml` | path/layout sanity check |
| Audit a proof | `constitution/prompts/audit-proof.md` | `constitution/schema/file-schema.yaml`, `constitution/schemas/audit-report.json` | proof layout audit |
| Audit a statement | `constitution/prompts/audit-statement.md` | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml`, `constitution/schemas/audit-report.json` | statement/block audit when available |
| Audit a stub | `constitution/prompts/audit-stub.md` | `constitution/schema/file-schema.yaml`, `constitution/schemas/audit-report.json` | volume layout audit |
| Audit chapter symbols | `constitution/prompts/audit-chapter-symbols.md` | canonical notation/predicate/relation YAML in the owning repo | YAML parse, symbol audit when available |
| Fix logical block gaps | `constitution/prompts/fix-logical-block-gaps.md` | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml` | statement/block audit when available |
| Plan toolkits | `constitution/prompts/plan-toolkits.md` | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml` | toolkit/decoration audit when available |
