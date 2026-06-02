# Constitution Master Index

## Purpose

This constitution governs the generation and auditing of all formal mathematical
content in the Learning Real Analysis project. It is organized into four
concerns that must never be mixed:

- **Schema** — what a valid artifact looks like
- **Prompts** — what to send to the model for generation or audit
- **Schemas** — what the model must return
- **This file** — orientation and loading order only

---

## Repository Map (as of 2026)

The project is split across multiple repos. The constitution is owned by
`lra-governance`, synced into every repo, and governs all repos. The detailed
ownership and filesystem map lives in `REPOSITORY_STRUCTURE.md`.

| Repository | Role |
|---|---|
| `lra-governance` | Governance: `DESIGN.md`, `REPOSITORY_STRUCTURE.md`, `.gitignore`, `constitution/` |
| `Learning-Real-Analysis` | Monorepo: full build, canonical YAMLs, docker, assembled volumes |
| `lra-common` | Shared LaTeX infrastructure: `common/`, `bibliography/` — synced to volumes |
| `lra-volume-i` | Volume I content — Overleaf target |
| `lra-volume-ii` | Volume II content — Overleaf target |
| `lra-volume-iii` | Volume III content — Overleaf target |
| `lra-volume-iv` | Volume IV content — Overleaf target |
| `lra-volume-v` | Volume V content — Overleaf target |
| `lra-volume-vi` | Volume VI content — Overleaf target |
| `lra-volume-vii` | Volume VII content — Overleaf target |
| `lra-volume-viii` | Volume VIII content — Overleaf target |
| `lra-lean` | Lean 4 proof formalization |
| `lra-nurbs` | NURBS/DDE C++ engine |
| `lra-knowledge-explorer` | Theorem extraction pipeline + HTML graph |

### Auditor root discovery

The auditor (`constitution/auditor/config.py`) can discover governance files in
the current repo by looking for `constitution/master.md`. Auditor operations
that need canonical YAML files should use the monorepo root by reading the
`REPO_ROOT` environment variable or by receiving `--repoDir`.

When running against a volume repo, set:

```bash
export REPO_ROOT=/path/to/Learning-Real-Analysis
# or pass --repoDir /path/to/Learning-Real-Analysis to the CLI
```

The canonical source files (`predicates.yaml`, `notation.yaml`, `relations.yaml`) live
at the monorepo root and are never duplicated in volume repos.

### common/ ownership

`common/` is owned by `lra-common`. Volume repos receive synced copies via
GitHub Actions (`.github/workflows/sync-to-volumes.yml` in `lra-common`).
Do not edit `common/` files in volume repos directly.

### governance ownership

`DESIGN.md`, `REPOSITORY_STRUCTURE.md`, `.gitignore`, and `constitution/` are
owned by `lra-governance`. Downstream copies are local working context only.

---

## File Map

### Schema (data files — loaded by scripts directly)

| File | Purpose |
|------|---------|
| `schema/block-registry.yaml` | Every possible logical block with trigger conditions and parent dependencies |
| `schema/artifact-matrix.yaml` | R/C/D/N requirement matrix indexed by artifact type |
| `schema/file-schema.yaml` | Canonical filesystem structure for volumes, chapters, proof directories |

### Prompts (injected into API calls by the Python auditor/generator)

| File | Artifact | Operation |
|------|----------|-----------|
| `prompts/audit-statement.md` | def / thm / lem / prop / cor / ax | Audit |
| `prompts/audit-proof.md` | proof file | Audit |
| `prompts/audit-stub.md` | chapter stub / volume stub | Audit |
| `prompts/audit-chapter-symbols.md` | full chapter | Symbol / notation / predicate audit |
| `prompts/generate-statement.md` | def / thm / lem / prop / cor / ax | Generate |
| `prompts/generate-proof.md` | proof file | Generate |
| `prompts/generate-stub-chapter.md` | chapter stub | Generate |
| `prompts/generate-stub-volume.md` | volume stub | Generate |
| `prompts/generate-breadcrumb.md` | breadcrumb box | Generate |
| `prompts/generate-capstone.md` | capstone exercise | Generate |

### Schemas (JSON response schemas — enforced by the Python auditor)

| File | Purpose |
|------|---------|
| `schemas/audit-report.json` | Shared response schema for all audit API calls |

---

## Loading Order for the Python Script

### Audit call (statement)
1. Load `schema/block-registry.yaml`
2. Load `schema/artifact-matrix.yaml`
3. Identify artifact type from LaTeX environment name
4. Extract requirement row from matrix
5. Concatenate: `prompts/audit-statement.md` + matrix row + block registry
6. Send to API
7. Validate response against `schemas/audit-report.json`

### Audit call (proof)
1. Load `prompts/audit-proof.md`
2. Send to API
3. Validate response against `schemas/audit-report.json`

### Audit call (stub)
1. Load `schema/file-schema.yaml`
2. Load `prompts/audit-stub.md`
3. Send to API
4. Validate response against `schemas/audit-report.json`

### Audit call (chapter symbols)
1. Load `prompts/audit-chapter-symbols.md`
2. Load canonical source files: `predicates.yaml`, `notation.yaml`, `relations.yaml`
3. Send chapter content + canonical files to API
4. Receive markdown audit report — **no YAML is written by the model**
5. On explicit user approval: issue separate add request; model returns YAML block only

### Generate call (statement)
1. Load `schema/block-registry.yaml`
2. Load `schema/artifact-matrix.yaml`
3. Load `prompts/generate-statement.md`
4. Identify artifact type
5. Send to API

### Generate call (proof)
1. Load `prompts/generate-proof.md`
2. Send to API

### Generate call (stub)
1. Load `schema/file-schema.yaml`
2. Load `prompts/generate-stub-chapter.md` or `generate-stub-volume.md`
3. Send to API

---

## Canonical Source Files (outside this constitution)

These files live at the **monorepo root** (`Learning-Real-Analysis/`) and are the
single source of truth for formal mathematical names. The constitution never
duplicates their content. They are never copied to volume repos.

```text
Learning-Real-Analysis/
  predicates.yaml
  notation.yaml
  relations.yaml
```

They are read-only to all automated processes. They are modified only by
explicit user instruction, by hand, after reviewing a markdown audit report.

---

## Invariants

- Generation and audit are never mixed in a single prompt.
- Canonical source files are never written by any automated process.
- Block structure for definitions and theorems is identical; artifact type
  determines only the requirement level of each block, not its existence.
- Atomic mathematical items are never bundled merely for convenience.
  Definitions, theorems, lemmas, propositions, corollaries, and axioms each
  occupy their own formal environment block with their own label.
- Statement-box colors are centralized in `common/colors.tex`. Result-like
  boxes use one blue family with decreasing visual weight from theorem to
  proposition to lemma to corollary. Definitions and axioms use separate
  palettes. Local files must not define or override statement-box colors.
- Source crosswalk remarks are expository metadata. Use
  `remark*` titled `Historical note` for direct provenance and `remark*`
  titled `Comparison with Feferman` for structural comparisons. They appear
  after `Interpretation` and before `Dependencies`, and never inside formal
  mathematical, quantified, predicate, or failure-mode blocks.
- Proof files are audited and generated by separate prompts because their
  layer structure is different in kind from the logical block sequence.
- Breadcrumbs belong in the relevant `index.tex` wrapper, not in the main
  note body file that carries the mathematical exposition and theorem
  content.

---

## Breadcrumb Placement Policy

Breadcrumbs are wrapper-level navigation, not note-body mathematics.

- A breadcrumb box belongs in the local `index.tex` file for the chapter,
  section cluster, or subsection cluster it introduces.
- In an `index.tex` wrapper, the breadcrumb should appear before the
  `\input{...}` chain for the note content it navigates.
- Breadcrumbs should not be interspersed with exposition, definitions,
  theorems, remarks, examples, or consequences inside the main note body
  file.
- The main note body file should begin directly with sectioning and
  mathematical content, not with navigation furniture.
- Proof files do not receive breadcrumbs unless there is an explicit
  repository-level wrapper rule for proof navigation.

---

## Axiom Atomicity Policy

Axioms follow the same atomicity discipline as definitions and theorem-like
items.

- One axiom environment contains exactly one independently nameable axiom.
- Distinct axioms must not be bundled into a single axiom block merely because
  they belong to the same axiomatic system.
- An axiomatic family may be introduced by topic-level exposition, but the
  individual axioms must still appear as separate labeled axiom environments.
- If a chapter presents a named axiom system such as the Peano axioms, the
  system name may appear at the topic level, while the formal mathematical
  items inside the topic remain atomic.

---

## Dependency Rendering Policy

Dependency tracking is structurally important, but empty local dependency
notices should not clutter note prose.

- If a statement-level item has substantive local dependencies, record them in
  a visible `\begin{remark*}[Dependencies] ... \end{remark*}` block.
- If a statement-level item is foundational within the current local note
  scope and has no local dependencies to display, do not print a visible
  dependencies remark that says `No local dependencies.`.
- In that foundational case, use the silent marker `\NoLocalDependencies`
  instead.
- `\NoLocalDependencies` is a structural repository marker only. It renders
  nothing in the compiled notes.
- The silent marker is for note-body statement artifacts. It does not change
  the existing practice for proof-file dependency remarks.
- Generation and audit rules must treat `\NoLocalDependencies` as satisfying
  the dependency requirement for foundational local items.

---

## Topic-Level Structural Policy

The repository supports an optional local structural container called a
`topicbox`.

- A `topicbox` is a pedagogical container inside a subsection.
- It is not a numbered mathematical claim.
- It is not a replacement for `\section`, `\subsection`, or
  `\subsubsection`.
- It does not participate in theorem numbering, proof linking, dependency
  labeling, or extraction identity. Those continue to belong to the
  theorem-like environments contained inside it.

The intended hierarchy is:

```text
Chapter
  Section
    Subsection
      Topic
        exposition
        definitions
        theorems
        remarks
        examples
        consequences
```

Here "Topic" means a local logical cluster inside a subsection, not a TOC
level.

### Topic Rules

1. Use `\section` and `\subsection` for the actual chapter hierarchy.
2. Use `topicbox` only inside a subsection.
3. A `topicbox` groups exactly one coherent concept cluster.
4. A `topicbox` may contain exposition, remarks, examples, definitions,
   theorems, propositions, lemmas, corollaries, and consequences.
5. A `topicbox` must not replace atomic theorem-like environments.
6. Every definition/theorem-like item inside a `topicbox` must still use its
   own separate environment.
7. Do not combine multiple definitions or theorems into one theorem-like
   environment merely because they live in the same topic.
8. `topicbox` containers must not be nested.
9. Avoid one-item `topicbox` containers unless the concept genuinely needs
   local framing or contrast.
10. Topicboxes are not used in proof files. Proof files keep the existing
    professional-standard / detailed-learning structure without topic
    containers.
11. The topic title is the concept title itself. Do not prefix topic titles
    with decorative labels such as `Topic:`.
12. When a subsection begins immediately with a `topicbox`, do not add a
    separate introductory prose block that merely repeats the exposition that
    belongs inside the topic. The topic exposition is the primary local
    framing device.

### Required Topic Exposition

If a subsection uses a `topicbox`, that topic must begin with a required
`exposition` environment.

- The exposition is mandatory, not optional.
- The exposition explains the mathematical role of the concept cluster: what
  structure, principle, formulation, or distinction is being isolated there.
- The exposition should describe what the topic establishes, axiomatizes,
  characterizes, or prepares for mathematically, not merely announce that the
  material has been grouped together.
- The exposition environment is intentionally not boxed.
- The exposition may be short, but it must perform real mathematical
  orienting work rather than decorative throat-clearing or transition prose.
- Do not duplicate that same orienting work in a separate pre-topic blurb in
  the subsection body unless there is genuinely broader subsection-level
  context that covers multiple topics at once.
- Prefer mathematically substantive opening sentences such as:
  "These axioms characterize ...",
  "This theorem identifies ...",
  "This topic isolates the distinction between ...",
  "This formulation makes precise ...".
- Avoid weak meta-introductions such as:
  "This topic gathers ...",
  "The grouping matters because ...",
  or similar prose that mainly comments on the organization instead of the
  mathematics.

Preferred local order inside a `topicbox`:

1. `exposition`
2. definitions
3. primary theorems
4. structural remark blocks
5. interpretation remarks
6. examples and non-examples
7. consequences

When a subsection contains multiple related formulations of one concept,
generation shall use separate `topicbox` containers over creating extra
subsections or flattening all formulations into one uninterrupted block.

### Atomic Definition Invariant

Every mathematical concept introduced into the repository shall be introduced
in its own definition environment and shall possess its own unique label.

Canonical principle:

```text
One concept -> one definition -> one label.
```

Grouping multiple independent mathematical concepts into a single definition
environment is prohibited. Each concept shall correspond to exactly one
definition environment, one label, one knowledge-graph node, and one
extraction record.

When generated or audited content contains multiple independently nameable
concepts in one proposed definition, generation shall stop or the audit shall
require a split into atomic definitions before the content is accepted.

### Atomic Figure Invariant

Every nontrivial TikZ figure shall exist as an independent figure source file.

Canonical principle:

```text
One figure -> one figure file.
```

Embedded nontrivial `tikzpicture` environments are prohibited in mathematical
note bodies, proof bodies, exercise bodies, exposition blocks, and statement
files. A dedicated figure source file shall contain only:

```latex
\begin{tikzpicture}
...
\end{tikzpicture}
```

The figure environment, caption, label, placement, and surrounding prose belong
at the inclusion point. Trivial inline visual marks are exempt only when they
have no independent mathematical identity, no caption, no label, no graph role,
and no reuse value.
