# Phase 4A Content Migration Map

## Executive summary

Phase 4A is a planning and audit phase only. No content is moved in this phase.

The eight-volume architecture is now structurally active, so the next migration work should narrow each volume to its canonical role without breaking labels, cross-references, split-repository builds, or knowledge extraction. The main pressure points are:

- Volume III currently contains analysis, algebra, geometry, topology, discrete calculus, quantum calculus, and elementary number theory.
- Volume IV is now titled as algebra and abstract structures, but its current content is mostly calculus, computational linear algebra, geometric modeling, ordinary differential equations, and Fourier analysis.
- Volume V is now titled as topology and geometry, but currently contains numerical analysis.
- Volume I contains both introductory logic/proof material and advanced foundations material that belongs, eventually, in Volume VIII.

The first implementation phase should not perform a broad split. It should either create destination stubs and router files, or move one small, self-contained cluster with clear label preservation. The recommended first actual move is the existing `volume-v/numerical-analysis` cluster into Volume VII, after a router strategy has been prepared.

## Current content inventory by volume

### Volume I: Logic, Sets, and Proof

Current active clusters:

- `volume-i/propositional-logic`
- `volume-i/predicate-logic`
- `volume-i/sets-relations-functions`
- `volume-i/axiom-systems`
- `volume-i/proof-techniques`
- `volume-i/algebras-of-sets`
- `volume-i/model-theory`
- `volume-i/type-theory`
- `volume-i/lambda-calculus`

Volume I should keep the introductory logic, proof, set, relation, function, and basic axiom-system tooling. Advanced foundations material is a candidate for Volume VIII, but it should move late because it likely anchors many labels and conceptual dependencies.

### Volume II: Origins of the Numbers

Volume II is already aligned with the target architecture. Its number-system construction material should remain in Volume II unless a later audit identifies small local cleanups.

### Volume III: Analysis

Current top-level clusters:

- `volume-iii/analysis`
- `volume-iii/algebra`
- `volume-iii/discrete-calculus`
- `volume-iii/geometry`
- `volume-iii/number-theory`
- `volume-iii/quantum-calculus`
- `volume-iii/topology`

Current analysis subclusters include real analysis, bounding, sequences, elementary functions, functions, continuity, differentiation, Riemann integration, measure theory, series, and function sequences. These should remain in Volume III.

The algebra, topology, and geometry clusters are the main candidates for migration. Discrete calculus, quantum calculus, and elementary number theory need later classification; they should remain in place until their dependencies and intended audience are clearer.

### Volume IV: Algebra and Abstract Structures

Current top-level clusters:

- `volume-iv/calculus`
- `volume-iv/computational-linear-algebra`
- `volume-iv/fourier-analysis`
- `volume-iv/geometric-modeling`
- `volume-iv/ordinary-differential-equations`

Volume IV should eventually receive algebra and abstract-structure material from current Volume III. Most existing Volume IV content should be reclassified: computational linear algebra and computational geometric modeling belong in Volume VI, numerical linear algebra and approximation material belong in Volume VII, and analytic calculus/ODE/Fourier material may belong in Volume III, VI, or VII depending on presentation.

### Volume V: Topology and Geometry

Current active cluster:

- `volume-v/numerical-analysis`

This cluster is a strong candidate for Volume VII. Volume V should eventually receive topology and geometry material from current Volume III.

### Volume VI: Computational Mathematics

Current status:

- Structural stub only.

Volume VI should receive computational mathematics clusters, especially computational linear algebra, computational geometric modeling, and computational or algorithmic ODE material.

### Volume VII: Numerical and Approximation Mathematics

Current status:

- Structural stub only.

Volume VII should receive numerical analysis, approximation, numerical linear algebra, interval arithmetic, floating-point analysis, and numerical ODE material.

### Volume VIII: Mathematical Logic and Foundations

Current status:

- Structural stub only.

Volume VIII should eventually receive advanced foundations material from Volume I, including model theory, type theory, lambda calculus, and advanced axiom-system material.

## Proposed final destination by chapter cluster

| Current cluster | Proposed destination | Migration note |
| --- | --- | --- |
| `volume-i/propositional-logic` | Volume I | Keep introductory proof and logic material in place. |
| `volume-i/predicate-logic` | Volume I | Keep first-order vocabulary and proof-facing material in place. |
| `volume-i/sets-relations-functions` | Volume I | Keep foundational set, relation, and function tooling in place. |
| `volume-i/proof-techniques` | Volume I | Keep in Volume I as core proof infrastructure. |
| Basic `volume-i/axiom-systems` material | Volume I | Keep introductory signature/language/axiom-system tooling in Volume I. |
| Advanced `volume-i/axiom-systems` material | Volume VIII | Move or mirror later after labels and dependencies are audited. |
| `volume-i/model-theory` | Volume VIII | Candidate for late migration because it is advanced foundations material. |
| `volume-i/type-theory` | Volume VIII | Candidate for late migration. |
| `volume-i/lambda-calculus` | Volume VIII | Candidate for late migration. |
| `volume-i/algebras-of-sets` | Volume I or Volume IV | Keep in Volume I initially; consider Volume IV mirror only if it becomes algebra-facing. |
| `volume-iii/analysis/*` | Volume III | Keep as the Analysis core. |
| `volume-iii/algebra/abstract-algebra` | Volume IV | Move with algebra batch. |
| `volume-iii/algebra/algebraic-structures` | Volume IV | Move with algebra batch. |
| `volume-iii/algebra/linear-algebra` | Volume IV | Move with algebra batch; coordinate with computational linear algebra in Volume VI. |
| `volume-iii/algebra/order-and-lattice` | Volume IV | Move with algebra batch. |
| `volume-iii/algebra/set-algebra` | Volume IV | Move with algebra batch, after resolving relationship to Volume I set algebras. |
| `volume-iii/algebra/set-algebras` | Volume IV | Move with algebra batch, after resolving naming duplication. |
| `volume-iii/algebra/category-theory` | Volume IV | Move with higher-risk abstract-structures batch. |
| `volume-iii/algebra/algebraic-geometry` | Volume IV or Volume V | Prefer Volume IV if algebra-facing; revisit if the presentation is geometry-facing. |
| `volume-iii/topology/point-set-topology` | Volume V | Move after topology cross-references are audited. |
| `volume-iii/geometry/*` | Volume V | Move in small subcluster batches. |
| `volume-iii/discrete-calculus` | Volume III, VI, or VII | Defer classification. It may be analysis, computational, or approximation-facing. |
| `volume-iii/quantum-calculus` | Volume III or VII | Defer classification until dependencies and intended use are clearer. |
| `volume-iii/number-theory/elementary-number-theory` | Volume II or Volume III | Defer. It may belong with number origins, but moving it is not urgent. |
| `volume-iv/computational-linear-algebra` | Volume VI and Volume VII | Computational algorithms to Volume VI; numerical approximation material to Volume VII. |
| `volume-iv/geometric-modeling` | Volume VI and Volume V | Computational modeling to Volume VI; geometric theory to Volume V if separated. |
| `volume-iv/ordinary-differential-equations` | Volume III, VI, and VII | Split only after identifying analytic, computational, and numerical portions. |
| `volume-iv/fourier-analysis` | Volume III or VII | Defer until its role as analysis or approximation material is clear. |
| `volume-iv/calculus` | Volume III | Candidate for Analysis unless it is computational or numerical in presentation. |
| `volume-v/numerical-analysis` | Volume VII | Recommended first actual content move after router preparation. |

## Migration priority order

1. Create destination stubs and router conventions for the first migration targets without moving mathematical content.
2. Move or route `volume-v/numerical-analysis` into `volume-vii/numerical-analysis`.
3. Create Volume V landing stubs for topology and geometry, then move one small geometry subcluster if its references are self-contained.
4. Move Volume III algebra into Volume IV in batches, starting with the most self-contained structural algebra material.
5. Move point-set topology into Volume V only after topology labels and dependencies are audited.
6. Reclassify current Volume IV content into analysis, computational mathematics, and numerical mathematics.
7. Move advanced Volume I foundations material into Volume VIII after router and extraction behavior are settled.

## Risk classification

| Migration item | Risk | Reason |
| --- | --- | --- |
| Destination stubs for Volumes IV-VIII | Low | Stub-only, no cross-references. |
| `volume-v/numerical-analysis` to Volume VII | Medium | Small and topically clear, but contains labels and may be referenced elsewhere. |
| `volume-iii/analysis/*` staying in Volume III | Medium | No move required, but this remains extraction-active content. |
| `volume-iii/geometry/*` to Volume V | Medium | Natural destination, but individual subclusters may contain internal labels and examples. |
| `volume-iii/algebra/*` to Volume IV | High | Many labels, proof files, and likely shared dependencies. |
| `volume-iii/topology/point-set-topology` to Volume V | High | Existing references to dense-set and total-boundedness results suggest nontrivial cross-reference dependencies. |
| `volume-iv/computational-linear-algebra` to Volume VI/VII | High | May need a conceptual split between computational and numerical material. |
| `volume-iv/geometric-modeling` to Volume VI/V | High | Likely straddles computational implementation and geometric theory. |
| `volume-iv/ordinary-differential-equations` to Volume III/VI/VII | High | Analytic, computational, and numerical portions should not be split mechanically. |
| `volume-i/model-theory`, `type-theory`, and `lambda-calculus` to Volume VIII | High | Advanced foundations material likely depends on Volume I notation and may affect extraction scope. |

## Recommended move batches

### Batch 1: Router and destination scaffolding

Create destination landing stubs and router conventions for:

- `volume-vii/numerical-analysis`
- `volume-iv/algebra`
- `volume-v/topology`
- `volume-v/<geometry-topic>`
- `volume-vi/computational-linear-algebra`
- `volume-viii/foundations` or the project-standard equivalent foundations cluster

This batch should not duplicate full content. It should establish how moved chapters will be referenced, how old paths will route temporarily, and how split repositories will build during migration.

### Batch 2: First low-to-medium risk content move

Move `volume-v/numerical-analysis` to `volume-vii/numerical-analysis`, leaving a temporary router at the old path. This is the clearest first actual move because the current source volume is otherwise intended to become topology and geometry.

### Batch 3: Geometry pilot

Move one geometry subcluster from `volume-iii/geometry` to its topic directory under `volume-v/` after confirming it has no broad cross-volume dependencies. A good pilot candidate should be chosen by label/reference audit, not by title alone.

### Batch 4: Algebra migration

Move Volume III algebra to Volume IV in subcluster batches. Start with self-contained algebraic structures, order/lattice, and linear algebra material, then handle category theory and algebraic geometry after dependency review.

### Batch 5: Computational and numerical split

Separate current Volume IV material into computational, numerical, and analytic portions. This should happen after Volume VI and VII have working landing structures and after numerical analysis has moved.

### Batch 6: Advanced foundations migration

Move or mirror advanced foundations material from Volume I into Volume VIII. This should be a late phase because it affects the conceptual spine of Volume I and may require knowledge-explorer changes.

## Required redirect/router strategy

Old paths should remain as temporary routers until all split repositories, root builds, extraction jobs, and external references have been reconciled.

Router files should:

- Use root-relative `\input{volume-*/...}` paths.
- Preserve old entry points during the migration window.
- Avoid duplicating full mathematical content in both the old and new locations.
- Include a short migration comment naming the canonical destination and planned removal phase.
- Be removed only after the corresponding split repository and monorepo builds agree on the canonical path.

Before moving any cluster, its destination index should be made root-relative or otherwise checked so that old routers and new direct inputs both compile.

## Label and cross-reference preservation strategy

Labels, references, citations, theorem identifiers, and proof identifiers must be preserved exactly during migration.

For every actual move:

1. Run a pre-move label/reference inventory with `rg "\\label|\\ref|\\cref|\\Cref|\\hyperref|\\cite"`.
2. Move whole clusters before splitting individual notes or proofs.
3. Keep all `\label{...}` names unchanged.
4. Keep all citation keys unchanged.
5. Add router files at old paths before removing old index entry points.
6. Build the affected standalone volume and the root document when practical.
7. Compare changed files to confirm that only paths and router inputs changed.

If a cluster has many references into another volume, create a dependency note before moving it. Do not introduce label aliases unless governance explicitly requires them.

## Overleaf file-limit risk notes

The migration is motivated in part by Overleaf file limits. During transition, temporary routers are safer than duplicating full content, but routers still count as files.

To reduce file-limit pressure:

- Avoid copying full chapter trees into both old and new volumes.
- Prefer one short router file per moved old entry point.
- Remove temporary routers in a later cleanup phase after sync workflows and split builds are stable.
- Do not move archives during the early migration phases.
- Keep split repositories focused on their canonical volume content and shared support files.

## Knowledge-explorer and extraction implications

The current knowledge rebuild workflow watches Volume II and Volume III. That scope may be intentional extraction targeting or a deferred decision, but it matters for migration:

- Moving algebra, topology, or geometry out of Volume III may remove those chapters from extraction unless the workflow is updated.
- Keeping old routers in Volume III may cause extraction to see routed content under old paths, depending on how extraction follows inputs.
- Duplicating content in both old and new paths could cause duplicate extraction.
- Moving advanced foundations from Volume I to Volume VIII may require adding Volume VIII to extraction scope.

Recommendation: before moving any extraction-active Volume III cluster, define the canonical extraction path and update extraction documentation or workflow scope in a dedicated phase.

## Exact next Codex prompt for Phase 4B

```text
Phase 4B: Create migration router scaffolding for the first content move.

Repository context:
You are working in the Learning-Real-Analysis monorepo.

Use:
docs/plans/phase-4a-content-migration-map.md
docs/architecture/volume-architecture.md
docs/architecture/volume-layout.md
docs/architecture/frontmatter-and-frontispiece-standard.md

Goal:
Prepare the first low-risk migration path for moving numerical analysis from Volume V to Volume VII. This phase should create destination scaffolding and temporary router conventions only unless the audit proves the move is self-contained and safe.

Scope:
Allowed:
- Inspect volume-v/numerical-analysis.
- Inspect volume-vii/index.tex and existing stub conventions.
- Create volume-vii/numerical-analysis stub files if needed.
- Update volume-vii/index.tex to include the new stub only if the standalone Volume VII build remains safe.
- Add a temporary router plan for the old volume-v/numerical-analysis path.
- Add or update a short planning note for the Volume V to Volume VII numerical-analysis migration.

Not allowed:
- Do not move existing numerical-analysis content yet unless the audit shows it is self-contained and you report the exact label/reference impact first.
- Do not rename folders.
- Do not alter labels, proof labels, citations, or cross-references.
- Do not edit workflows or build scripts.
- Do not stage optional images.
- Do not delete archives.

Validation:
- Confirm no mathematical content moved unless explicitly justified by the audit.
- Confirm any new files are stubs or planning/router files only.
- Confirm Volume VII standalone build still works if volume-vii/index.tex is changed.
- Confirm root main.tex still includes Volumes I-VIII in order.
- Confirm optional images remain untracked.

Commit message:
Prepare numerical analysis migration scaffolding
```

## Phase 4A validation checklist

- No content files should be moved in this phase.
- No TeX source files should be changed in this phase.
- No workflows or build scripts should be changed in this phase.
- No image files should be added, removed, staged, or modified in this phase.
- No labels, references, citations, theorem labels, or proof labels should be changed in this phase.
