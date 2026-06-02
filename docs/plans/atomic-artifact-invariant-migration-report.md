# Atomic Artifact Invariant Migration Report

## Summary

This report records the governance refactor that promotes atomic definitions
and atomic figures to repository invariants.

New canonical invariants:

- one concept -> one definition -> one label;
- one nontrivial TikZ figure -> one figure source file.

This phase updates governance, constitution, schema, extraction rules, and
generation/audit prompts. It does not split mathematical content or extract
figures from volume files.

## Modified Governance Locations

- `DESIGN.md`
- `constitution/master.md`
- `constitution/schema/file-schema.yaml`
- `constitution/prompts/audit-statement.md`
- `constitution/prompts/generate-statement.md`
- `constitution/prompts/generate-stub-chapter.md`
- `docs/architecture/latex-build-and-rendering.md`
- `docs/governance/README.md`
- `docs/governance/atomic-artifact-standards.md`
- `docs/governance/authoring-standards.md`
- `docs/governance/extraction-standards.md`
- `docs/governance/file-splitting-standards.md`
- `docs/governance/model-standards.md`

## Audit Method

A lightweight repository audit was run over the monorepo and split volume
repositories. The audit identified:

- embedded `tikzpicture` candidates outside canonical figure source files;
- definition environments containing lists or multiple definition-like items.

These are candidates for manual review, not automatic conclusions. Some
matches are archives, macro definitions, or deliberately local trivial visuals.

## Embedded TikZ Candidates Requiring Review

Shared macro files such as `common/boxes.tex` were excluded from the follow-up
chapter list because they define infrastructure rather than mathematical
figures.

Chapters and areas requiring review:

- `volume-i/sets-relations-functions`
- `volume-i/predicate-logic`
- `volume-iii/discrete-calculus`
- `volume-iii/analysis`
- `volume-v/topology`
- `volume-vii/numerical-analysis`

Specific monorepo candidate files:

- `front-matter.tex`
- `volume-vii/numerical-analysis/notes/ieee-floating-point/notes-ieee-floting-point.tex`
- `volume-v/topology/point-set-topology/notes/notes-topology.tex`
- `volume-iii/discrete-calculus/difference-operators/linearity.tex`
- `volume-iii/discrete-calculus/expansions/newtons-series.tex`
- `volume-iii/discrete-calculus/motivation/motivation.tex`
- `volume-iii/discrete-calculus/telescoping-sums/telescoping-identity.tex`
- `volume-iii/discrete-calculus/foundations/notes/binomial-theorem.tex`
- `volume-iii/analysis/differentiation/notes/secant-tangent/figure1.tex`
- `volume-iii/analysis/bounding/notes/bounds-extremals/notes-supremum.tex`
- `volume-i/sets-relations-functions/notes/sets/notes-set-operations.tex`
- `volume-i/predicate-logic/notes/reference/notes-summary-tables.tex`
- `volume-i/predicate-logic/notes/translation/notes-square-of-opposition.tex`

## Bundled Definition Candidates Requiring Review

Archive matches are intentionally listed separately because archive material
should not be changed without an explicit archive-cleanup task.

Active chapters and areas requiring review:

- `volume-ii/peano-systems`
- `volume-ii/rationals`
- `volume-iii/analysis`
- `volume-iv/algebra`
- `volume-vii/numerical-analysis`
- `volume-viii/algebras-of-sets`
- `volume-viii/lambda-calculus`

Specific active candidate files:

- `volume-viii/lambda-calculus/notes/introduction/notes-introduction.tex`
- `volume-viii/algebras-of-sets/notes/rings-of-sets/notes-rings-of-sets.tex`
- `volume-vii/numerical-analysis/notes/ieee-floating-point/notes-ieee-floting-point.tex`
- `volume-iv/algebra/order-and-lattice/lattice-order/notes/ordered-sets/notes-ordered-sets.tex`
- `volume-iv/algebra/linear-algebra/notes/vector-spaces/notes-vector-spaces.tex`
- `volume-iv/algebra/algebraic-structures/notes/formal-model-theoretic-structures.tex`
- `volume-iv/algebra/algebraic-structures/notes/fields/notes-fields-definition.tex`
- `volume-iv/algebra/algebraic-structures/notes/groups/notes-groups-definition.tex`
- `volume-iv/algebra/algebraic-structures/notes/rfo/notes-alg-functions.tex`
- `volume-iv/algebra/algebraic-structures/notes/rings/notes-rings-definition.tex`
- `volume-iv/algebra/algebraic-geometry/notes/notes-polynomial-rings.tex`
- `volume-iii/analysis/elementary-functions/notes/trigonometric/notes-trigonometric.tex`
- `volume-iii/analysis/continuity/notes/point-continuity/notes-discontinuity.tex`
- `volume-ii/rationals/notes/defining-order-on-q/notes-defining-order-on-q.tex`
- `volume-ii/rationals/notes/field-structure-of-q/notes-field-structure-of-q.tex`
- `volume-ii/rationals/notes/nonlinear-approximation-in-q/notes-problems-in-q.tex`
- `volume-ii/peano-systems/notes/defining-peano-systems/notes-defining-peano-systems.tex`

Archive-only candidates:

- `volume-i/propositional-logic/archive/pre-2026-governance-rebuild/notes/notes-equivalences.tex`
- `volume-i/propositional-logic/archive/pre-2026-governance-rebuild/notes/notes-normal-forms.tex`
- `volume-i/propositional-logic/archive/pre-2026-governance-rebuild/notes/notes-semantics.tex`
- `volume-i/propositional-logic/archive/pre-2026-governance-rebuild/notes/notes-syntax.tex`
- `volume-i/archive/moved-to-volume-iii/lattice-order/notes/ordered-sets/notes-ordered-sets.tex`

## Required Follow-Up Migration Rules

For bundled definitions:

1. Split each independent concept into its own definition environment.
2. Preserve any existing canonical labels when the concept identity is already
   clear.
3. Create new stable labels only for genuinely separate concepts that were
   previously hidden inside a bundled environment.
4. Update references and dependency blocks to point at the new atomic labels.
5. Regenerate extraction records.

For embedded TikZ figures:

1. Determine whether the embedded diagram is nontrivial.
2. Extract each nontrivial `tikzpicture` to a dedicated figure source file.
3. Leave the figure environment, caption, and label at the inclusion point.
4. Do not extract trivial inline marks unless they carry independent
   mathematical meaning.

## Manual Review Notes

The audit is intentionally conservative. List-based definitions may include
component fields of a single structural concept; those still require review
because the new invariant asks whether each listed item is independently
nameable and graph-worthy.

The figure audit is also conservative. A `tikzpicture` in a file named
`figure1.tex` may already be close to compliance, but it still requires review
because canonical figure source files shall contain only the `tikzpicture` and
should follow the `figure-<n>.tex` naming convention where applicable.
