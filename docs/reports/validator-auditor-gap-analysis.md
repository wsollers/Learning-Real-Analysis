# Validator/Auditor Gap Analysis

Date: 2026-06-12

## Scope

This report compares the legacy auditor prompt contracts in
`constitution/prompts/audit-*.md` with the current validation surface in:

- `constitution/auditor/audits/*.py`
- `constitution/auditor/validators/*.py`
- `tools/governance/*.py`
- `tools/governance/rules/**`
- `capabilities/*/verify.py`

The purpose is to decide what still blocks capability installation and legacy
auditor retirement.

## Executive Summary

The deterministic validators now cover much of the structural surface that the
legacy auditors described, especially chapter routing, proof layout, decoration
presence/order, label discipline, dependency links, box discipline, figure
placement, generated-block hygiene, canonical volume shape, print-edition
routing, router-only topic indexes, and canonical capstone/exercises routing.

Since the first draft of this report, `tools/governance/validate_volume.py` has
become the primary volume-level runner. It runs `volume_shape` first and fails
fast when the canonical tree is not present; only then does it run the small
composable validators. The parity fixture harness now includes
`validate_volume.py` coverage in addition to legacy layout/proof/decoration
checks.

Parity is not complete. The remaining gaps are concentrated in semantic or
judgment-heavy checks:

- mathematical correctness of negations, contrapositives, and quantified forms;
- atomicity of definitions and theorem-like statements;
- canonical notation consistency beyond simple predicate/name scans;
- full symbol-audit reporting against predicates, notation, and relations;
- audit-report output parity for statement audits;
- volume-stub parity;
- proof body quality checks beyond layer/layout enforcement.

Recommendation: install the capability path as the primary workflow, but keep
legacy auditors archived as fallback until the gaps below are resolved or
explicitly retired.

## Current Surfaces

### Legacy Auditors

| Legacy auditor | Contract type | Output |
|---|---:|---|
| `audit-statement.md` | LLM structural and semantic statement audit | JSON audit report |
| `audit-proof.md` | LLM proof-file layer and proof quality audit | JSON audit report |
| `audit-stub.md` | LLM chapter/volume stub audit | JSON audit report |
| `audit-chapter-symbols.md` | LLM chapter symbol consistency audit | Markdown report |

### Current Validators And Auditors

| Surface | Nature | Notes |
|---|---:|---|
| `constitution/auditor/audits/statement.py` | LLM-backed | Still calls `audit_statement` prompt. Not a deterministic replacement. |
| `constitution/auditor/audits/proof.py` | Hybrid | TODO stubs are deterministic; full proof audit still calls prompt. |
| `constitution/auditor/audits/stub.py` | LLM-backed | Still calls `audit_stub` prompt. |
| `constitution/auditor/audits/symbols.py` | LLM-backed | Still calls `audit_symbols` prompt. |
| `constitution/auditor/validators/generated_block.py` | Deterministic | Good generated statement block preflight. Narrower than statement audit. |
| `constitution/auditor/validators/ontology.py` | Deterministic | Validates canonical ontology files, not chapter usage parity. |
| `tools/governance/validate_decoration.py` | Deterministic | Thin harness over decoration rules. Strong structural replacement for many statement checks. |
| `tools/governance/audit_proof_layout.py` | Deterministic | Strong proof layout validator. |
| `tools/governance/validate_volume.py` | Deterministic | Primary composable volume validator with shape fail-fast. |
| `tools/governance/validators/*.py` | Deterministic | Small validator modules for routing, proof contracts, capstones, dependencies, labels, formal decoration, and structure. |
| `tools/governance/validate_chapter_house_rules.py` | Deterministic legacy | Broad strict chapter validator. Useful as a comparison source; no longer the target architecture. |
| `tools/governance/audit_volume_layout.py` | Deterministic legacy | Volume/chapter routing and layout validation. Useful as comparison/fallback until parity retirement. |
| `tools/governance/test_parity_fixtures.py` | Deterministic test | Now includes `validate_volume.py` expected code coverage for the broken parity fixture. |
| `capabilities/author-definition/verify.py` | Deterministic | Bound file verifier for generated definitions. Narrow. |
| `capabilities/author-statement/proof_stub.py` | Deterministic generator | Generates proof stubs; not itself an auditor. |

## Gap Matrix

### Statement Auditor Versus Validators

| Legacy check | Current deterministic coverage | Gap |
|---|---|---|
| Environment label present, prefix, lowercase slug | Covered by `validate_decoration.py`, `validate_chapter_house_rules.py`, and generated-block validator. | Low. |
| Box presence and house colors | Covered by generated-block validator, `validate_chapter_house_rules.py`, and box-color audit. | Low for generated/current forms. Legacy wrapper variants should remain tested. |
| Proof link from theorem-like statement | Covered by `validate_chapter_house_rules.py` and decoration rules. | Low. |
| Required decoration blocks present | Covered for key blocks by `validate_decoration.py`, `validate_chapter_house_rules.py`, and `author-definition/verify.py`. | Medium. Coverage is not full artifact-matrix parity for every optional/conditional/dependent block. |
| Decoration block order | Covered by `validate_chapter_house_rules.py`; partly by decoration rules. | Low/medium. Needs parity fixtures for all block-order cases. |
| Forbidden decoration blocks by artifact type | Covered by `validate_chapter_house_rules.py`. | Low. |
| Dependent block parent/child rules | Covered by `validate_chapter_house_rules.py`. | Low/medium. Needs broader tests for each dependent pair. |
| Dependencies block or `\NoLocalDependencies` | Covered by generated-block validator and chapter house rules. | Low. |
| Dependency targets are formal labels, not proof labels | Covered by generated-block validator and chapter house rules. | Low. |
| Source crosswalk citation presence | Covered by chapter house rules. | Medium. Validator checks citation presence, not provenance semantics. |
| Examples/non-examples do not introduce labels/formal statements | Covered by chapter house rules. | Low/medium. |
| Predicate names not in formal bodies | Covered for `\operatorname` leakage and some known predicate forms. | Medium. It does not fully prove all predicate-language leakage. |
| Predicate names registered in canonical source | Partly covered by `author-definition/verify.py` and ontology validator. | High. Chapter-wide usage parity still depends on symbol auditor prompt. |
| Notation matches `notation.yaml` | Not fully covered. | High. Existing validators catch some style/shape issues, not full notation registry consistency. |
| Relation names match `relations.yaml` | Not fully covered. | High. Ontology validates registry integrity, not chapter usage. |
| Correct negated quantified statement | Not deterministically covered. | High. Requires semantic logic or explicit structured source fields. |
| Correct contrapositive | Not deterministically covered. | High. Requires semantic logic or structured hypothesis/conclusion representation. |
| Quantifier variables all fixed/explicit | Not deterministically covered except some style triggers. | High. |
| Atomicity of formal item | Not deterministically covered. | High. This is repository-identity critical and still judgment-heavy. |
| Figure atomicity / embedded TikZ | Covered by chapter house rules for inline/nontrivial TikZ placement. | Low/medium. "Nontrivial" remains heuristic. |
| JSON audit-report schema output | Covered only for prompt-backed auditor path. | Medium. Deterministic validators emit their own records, not the legacy audit JSON shape. |

### Proof Auditor Versus Validators

| Legacy check | Current deterministic coverage | Gap |
|---|---|---|
| Layer order | Covered by `audit_proof_layout.py` and chapter house rules. | Low. |
| `\newpage`, `\phantomsection`, proof label, `\LRAProofFor` | Covered. | Low. |
| Label root and filename match | Covered. | Low. |
| Return navigation | Covered. | Low. |
| Proof-vault URL placement and raw image rejection | Covered by `audit_proof_layout.py`. | Low. |
| Restatement has no label and uses starred theorem-like env | Covered. | Low. |
| Professional and detailed proof layers present | Covered. | Low. |
| Proof structure remark present | Covered. | Low. |
| Dependencies block present and formal targets only | Covered. | Low. |
| Proof topic/index reachability | Covered by `audit_proof_layout.py`, `audit_volume_layout.py`, and chapter house rules. | Low. |
| Stub squareness | Covered by `audit_proof_layout.py`, decoration rules, and proof-stub generator. | Low. |
| No proof-structuring/flash macros | Covered by chapter house rules for known prohibited macros. | Medium. Custom macro universe is not exhaustively classified. |
| No topicbox/exposition in proof files | Mostly covered by top-level environment discipline. | Medium. Needs explicit parity fixture. |
| Professional proof is compact and rigorous | Not covered. | High. This is semantic/quality judgment. |
| Detailed proof steps are genuine logical milestones | Not covered. | High. |
| House notation in proof bodies | Not fully covered. | High. |
| Full JSON audit-report schema output | Prompt-backed proof auditor still provides this; deterministic layout validator uses its own JSON shape. | Medium. |

### Stub Auditor Versus Validators

| Legacy check | Current deterministic coverage | Gap |
|---|---|---|
| Chapter required paths | Covered by `validate_chapter_house_rules.py` and `audit_volume_layout.py`. | Low. |
| Chapter router heading/label/breadcrumb/input order | Covered by `validate_chapter_house_rules.py` and volume layout audit. | Low. |
| Notes/proofs topic routing | Covered. | Low. |
| Capstone standard location/routing | Covered. | Low. |
| Folder/file naming discipline | Covered for many chapter files. | Low/medium. |
| Chapter registry membership | Covered. | Low. |
| Volume stub `index.tex` scope/orientation | Partial. | Medium/high. Volume-level prose/orientation requirements are not clearly enforced. |
| Volume `chapter.yaml` dependency order | Partial. | High. Presence/shape can be checked; dependency order is judgment unless registry encodes ordering authority. |
| JSON audit-report schema output | Not deterministic. | Medium. |

### Chapter Symbol Auditor Versus Validators

| Legacy check | Current deterministic coverage | Gap |
|---|---|---|
| Registry YAML parses and internal references are valid | Covered by ontology validator. | Low. |
| Predicate `\operatorname{...}` leakage into formal bodies | Partly covered by generated-block validator and chapter house rules. | Medium. |
| Missing predicate names in chapter usage | Partly covered for generated definitions. | High. No full deterministic chapter scanner against predicates.yaml parity. |
| Predicate arity/form consistency | Not fully covered. | High. |
| Missing notation items | Not fully covered. | High. |
| Inconsistent notation conventions | Not fully covered. | High. |
| Missing relation names | Not fully covered. | High. |
| Inconsistent relation usage | Not fully covered. | High. |
| Unused registry entries by chapter | Not covered deterministically. | Medium. Informational only, but useful. |
| Markdown symbol-audit output | Still prompt-backed. | Medium. |

## Cross-Cutting Gaps

### 1. Prompt-Backed Auditors Still Exist

The `constitution/auditor/audits` modules still call the legacy prompt contracts
for statement, full proof, stub, and symbol audits. That means the project has
not yet replaced the auditors with validators. It has added deterministic
validators beside them.

### 2. Output Shape Is Not Unified

Legacy auditors return `constitution/schemas/audit-report.json` shaped reports.
The deterministic validators return validator-specific JSON or terminal output.
Before retiring auditors, decide whether parity requires:

- preserving the old audit-report schema;
- accepting validator-native reports;
- adding a small adapter that converts validator records into audit-report
  records for compatibility.

### 3. Semantic Checks Need A Data Model

The hardest statement checks cannot be made robust with regex alone:

- correct negation;
- correct contrapositive;
- all variables fixed or quantified;
- atomicity;
- notation equivalence;
- predicate arity equivalence.

To retire the prompt auditor for these, the source needs structured fields or a
small intermediate representation. Otherwise these should be explicitly marked
"human/LLM review retained" rather than claimed as validator parity.

### 4. Parity Fixtures Are Too Small

`tools/governance/fixtures/parity/manifest.json` now locks a small but more
representative set of issue codes, including unified `validate_volume.py`
coverage:

- unified volume validation: chapter router shape, router-only topic indexes,
  missing dependency declarations, interpretation warnings, wrong label prefix,
  unknown/forbidden decoration blocks, missing dependent decoration parents,
  source crosswalk citation hygiene, formal claims inside expository blocks,
  decoration order, label-inside-restatement, topicbox/exposition in proof
  files, inline TikZ placement, duplicate labels, boxed nonformal content, proof
  routing/reachability, stub discipline, proof dependency targets, and proof
  exercises routing;
- decoration: wrong label prefix, missing interpretation, missing dependencies,
  missing standard quantified statement, missing proof navigation, proof stub
  structure not blank;
- proof layout: partial stub, proof dependency target, proof reachability;
- volume layout: unrouted proofs topic.

This is a useful start, but still not enough to justify deletion of the legacy
semantic auditors.

## Retirement Readiness

### Safe To Treat As Validator-Owned Now

These rule families can be treated as validator-owned, with legacy auditors only
as optional fallback:

- proof file layer presence/order;
- proof label/root/filename association;
- proof routing/index reachability;
- chapter notes/proofs topic layout;
- chapter router order;
- formal label prefix and slug hygiene;
- required basic decoration presence;
- dependency block presence and target-prefix discipline;
- box color/style discipline;
- generated-block basic hygiene;
- inline/nontrivial TikZ placement checks.

### Not Safe To Remove Yet

Keep the legacy prompt/auditor fallback for:

- statement semantic correctness;
- atomicity detection;
- negation and contrapositive correctness;
- full notation/predicate/relation chapter usage audit;
- proof body quality and logical milestone review;
- volume-stub dependency-order and orientation review;
- old audit-report JSON compatibility, if any downstream tooling consumes it.

## Recommended Closeout Plan

1. Continue adding parity manifest rows for every legacy auditor requirement
   that is validator-owned.
2. Continue adding negative fixtures for lower-priority deterministic checks as
   they are declared validator-owned.
3. Build or explicitly defer a deterministic chapter symbol scanner.
4. Decide output compatibility: validator-native JSON or audit-report adapter.
5. Mark semantic checks as either structured-data future work or retained
   human/LLM review.
6. Only then move `constitution/prompts/audit-*.md` to legacy/archive or remove
   them.

## Bottom Line

Validator coverage is strong enough to install capabilities and route normal
work through the deterministic path. It is not strong enough to delete the
legacy auditors without either losing semantic review coverage or formally
declaring those semantic checks out of scope.
