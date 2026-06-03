# Structured Rule Gap Audit

Audit date: 2026-06-03

## Scope

This audit checked whether machine-checkable governance rules still need a new
structured rule file. It inspected the existing schema authority:

- `constitution/schema/block-registry.yaml`
- `constitution/schema/artifact-matrix.yaml`
- `constitution/schema/file-schema.yaml`

It also reviewed governance and workflow docs for rules that appear
machine-checkable. No new schema system was created.

## Existing Coverage

| Rule area | Existing authority | Coverage |
| --- | --- | --- |
| Required theorem-like blocks | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml` | Covered. Matrix records requirement levels for theorem, lemma, proposition, corollary, axiom, and definition artifacts. |
| Required definition blocks | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml` | Covered. Definition requirements, examples, non-examples, interpretation, dependencies, and box behavior are represented. |
| Permitted theorem-like environment categories | `constitution/schema/artifact-matrix.yaml`, `constitution/schema/block-registry.yaml` | Covered for `def`, `thm`, `lem`, `prop`, `cor`, and `ax`. |
| Required logical/remark block names | `constitution/schema/block-registry.yaml` | Covered. Block IDs, display names, environments, triggers, parents, and notes are structured. |
| Box eligibility rules | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml` | Covered. Conditional box triggers and artifact-level requirement levels are structured. |
| One-toolkit-box-per-section rule | `constitution/schema/block-registry.yaml`, `constitution/schema/artifact-matrix.yaml` | Covered. The toolkit block records the rule and the matrix makes it required. |
| Breadcrumb and roadmap requirements | `constitution/schema/file-schema.yaml` | Covered. Chapter stub order, breadcrumb source, roadmap requirements, and breadcrumb formatting are structured. |
| Proof file layout | `constitution/schema/file-schema.yaml` | Covered. Required layers, layer definitions, macro rules, label rules, and vault URL placement are structured. |
| Proof-to-theorem association | `constitution/schema/file-schema.yaml` | Covered. `\LRAProofFor{...}` is a required proof layer. |
| Proof body section requirements | `constitution/schema/file-schema.yaml` | Covered. Professional standard and detailed learning proof bodies are required layers for full proofs and stubs. |
| Proof file naming and location | `constitution/schema/file-schema.yaml` | Covered. Topic-mirrored location, legacy location, naming rule, topic match rule, and index hook rule are structured. |
| Stub chapter required markers | `constitution/schema/file-schema.yaml` | Covered. Stub files, directories, topic pairs, index order, breadcrumb source, and roadmap content are structured. |
| Stub section/topic markers | `constitution/schema/file-schema.yaml` | Covered. Topic-pair stubs and matched notes/proofs topic directories are structured. |
| Volume/chapter/topic folder layout | `constitution/schema/file-schema.yaml` | Covered. Volume, chapter, topic, router, legacy, and audit-tool rules are structured. |

## Gaps Found

### Generated Agent Wrapper File Set

- Source docs: `docs/governance/agent-instruction-policy.md`,
  `docs/architecture/generated-file-policy.md`,
  `docs/workflows/generated-wrapper-sync.md`
- Gap: The provider wrapper file set is prose-only.
- Why existing schema does not already cover it:
  `constitution/schema/file-schema.yaml` currently focuses on mathematical
  volume/source layout and proof/stub layout. Generated wrapper files are
  governance delivery artifacts, not mathematical source artifacts.
- Proposed existing schema file to extend: `constitution/schema/file-schema.yaml`
  could grow a `generated_agent_wrappers` section if wrapper sync tooling needs
  a structured file list.
- Enforcement status: documented.
- Validator status: planned tools are named in `tools/governance/README.md`;
  no implemented wrapper validator was inspected in this audit.

Recommendation: defer schema edits until wrapper sync tooling needs a concrete
machine-readable file list.

### Generated File Header Fields

- Source docs: `docs/architecture/generated-file-policy.md`,
  `docs/governance/agent-instruction-policy.md`
- Gap: Required generated-file header fields are prose-only.
- Why existing schema does not already cover it: The existing schemas do not
  define generated wrapper metadata.
- Proposed existing schema file to extend: `constitution/schema/file-schema.yaml`
  if wrapper files remain treated as file-layout artifacts; otherwise a future
  wrapper-specific schema may be justified.
- Enforcement status: documented.
- Validator status: planned wrapper drift/report tools, not currently enforced
  here.

Recommendation: defer. This is mechanical, but adding schema before the wrapper
generator exists would create premature authority.

### Repo Overlay Selection Constraints

- Source docs: `docs/governance/agent-instruction-policy.md`,
  `docs/governance/repo-overlays/README.md`
- Gap: "Exactly one repo overlay or repo-family overlay" is prose-only.
- Why existing schema does not already cover it: Existing schema files govern
  mathematical artifacts and file layout, not wrapper composition.
- Proposed existing schema file to extend: none yet. This likely belongs with
  future wrapper generation metadata rather than
  `constitution/schema/block-registry.yaml`,
  `constitution/schema/artifact-matrix.yaml`, or proof/volume layout schema.
- Enforcement status: documented.
- Validator status: planned wrapper validation tooling.

Recommendation: leave prose-only until wrapper generation metadata exists.

### Atomic Definition Splitting

- Source docs: `docs/governance/atomic-artifact-standards.md`,
  `docs/governance/extraction-standards.md`,
  `docs/governance/file-splitting-standards.md`
- Gap: The one-concept-per-definition invariant is documented but not fully
  machine-enforceable.
- Why existing schema does not already cover it: Detecting whether a definition
  contains multiple independent mathematical concepts requires mathematical
  judgment. Label count and environment count are auditable, but concept
  splitting is not purely mechanical.
- Proposed existing schema file to extend: no schema change recommended.
- Enforcement status: auditable.
- Validator status: statement/decoraton audits can flag risks, but a validator
  should not claim full semantic enforcement.

Recommendation: intentionally leave as prose plus audit heuristic.

### Predicate Leakage And Missing Canonical Vocabulary

- Source docs: `docs/governance/notation-standards.md`,
  `docs/governance/extraction-standards.md`
- Gap: Predicate leakage and missing canonical vocabulary are partly
  machine-detectable but depend on canonical YAML and local mathematical
  context.
- Why existing schema does not already cover it: The rule is data-dependent on
  `predicates.yaml`, `notation.yaml`, and `relations.yaml`, which are owned by
  `Learning-Real-Analysis`.
- Proposed existing schema file to extend: no schema change recommended.
- Enforcement status: auditable.
- Validator status: extraction and symbol audits should consume canonical YAML.

Recommendation: leave in governance/extraction standards; do not duplicate the
canonical YAML vocabulary into governance schema.

## Rules Intentionally Left Prose-Only

- Whether a theorem is structurally dominant enough to deserve a box.
- Whether an interpretation block is genuinely redundant with nearby
  exposition.
- Whether a negated or contrapositive form is standardly useful in proofs.
- Whether a definition bundles multiple independent concepts.
- Whether a proof is mathematically correct.
- Whether a topicbox is pedagogically warranted rather than decorative.

These are auditable by humans and sometimes assisted by tools, but they should
not be represented as deterministic validator rules.

## Recommendation

Do not create `data/governance-rules.yaml` at this time.

The current schema authority already covers the main machine-checkable
mathematical, proof, stub, breadcrumb, and volume-layout rules. The remaining
obvious machine-checkable gaps are generated-wrapper metadata and overlay
selection, which should be structured only when the wrapper generator or drift
validator is implemented.
