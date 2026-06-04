# Exercise Vault Standards

These standards apply to chapter-level exercise vaults in LRA volume content.

## Purpose

The exercise vault stores exercises, computations, proof drills, reading
checks, diagrams, formalization tasks, implementation tasks, and
handwritten/photo-based homework artifacts.

The exercise vault is separate from the theorem proof vault. Theorem,
proposition, lemma, and corollary proofs remain under the owning chapter's
`proofs/` tree. Exercise records live under the owning chapter's top-level
`exercises/` tree.

## Directory Shape

Each chapter that owns exercise-vault records may use:

```text
chapter/
  exercises/
    index.tex
    exercise-ledger.yaml
    reports/
      index.md
      progress.md
      rework-queue.md
    sources/
      photos/
    <topic>/
      <subtopic>/
        index.tex
        <exercise-set-id>.tex
```

Topic and subtopic folders are mutable routing details. Stable IDs in
`exercise-ledger.yaml` are canonical.

## Identity Rule

Stable IDs are identities. Paths are not identities.

Every exercise set must have a stable `exercise_set_id`. Every item in a set
must have a stable item ID. Refactors may move files, split topics, or rename
folders only if the ledger preserves the same IDs and updates the routing
paths.

## Ledger Rule

Metadata lives primarily in `exercise-ledger.yaml`. TeX files contain the
mathematical artifact: exercise text, solution blocks, verification blocks,
and optional rework notes.

The ledger should record:

```yaml
exercise_sets:
  - exercise_set_id:
    mode:
    topic:
    skill_tags:
    source_image:
    status:
    path:
    items:
      - item_id:
        status:
        skill_tags:
        exercise:
        result:
        verification:
          status:
          notes:
        rework_notes:
```

`source_image` is relative to `exercises/sources/photos/` when present. Use
`null` only when no image source exists.

## TeX Exercise Set Layout

An exercise set file may group many related exercise items. It should contain:

- `exercise_set_id`;
- `mode`;
- `topic`;
- `source_image`;
- `status`;
- exercise items with stable item IDs;
- solution blocks;
- verification blocks;
- optional rework notes.

The header may repeat ledger metadata as comments for local readability, but
the ledger remains the metadata authority.

## Modes

Allowed modes are:

- `computation`;
- `proof`;
- `reading`;
- `diagram`;
- `formalization`;
- `implementation`.

## Skills

Allowed skill tags are:

- `fluency`;
- `proof_construction`;
- `proof_verification`;
- `concept_application`;
- `translation`;
- `counterexample_search`;
- `structural_analysis`;
- `rework`.

## Statuses

Allowed statuses are:

- `generated`;
- `active`;
- `attempted`;
- `complete`;
- `corrected`;
- `rework`;
- `memorialized`.

## Source Photos

Source photos are copied into `exercises/sources/photos/` and referenced from
the ledger. Do not use source-photo paths as exercise identities.

Raw mobile images should be sanitized before entering git when metadata may
contain private information. If sanitization cannot be verified, keep the raw
source outside tracked content and record the blocker.

## Reports

Reports are generated from `exercise-ledger.yaml`. At minimum, keep:

- `reports/index.md`;
- `reports/progress.md`;
- `reports/rework-queue.md`.

Reports are derived artifacts. During refactors, regenerate them from the
ledger after path updates.
