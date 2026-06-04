# Exercise Vault Memorialization Workflow

Use this workflow for the command:

```text
Memorialize this exercise. <topic>
```

This workflow is for exercises, computations, proof drills, and homework
artifacts. It is not the theorem proof-vault workflow.

## Command Semantics

`<topic>` is a mutable routing topic relative to the chapter's
`exercises/` tree, such as `derivatives`. It is not the exercise identity.

When source images are present, copy sanitized source images into:

```text
chapter/exercises/sources/photos/
```

Then reference the copied filename from `exercise-ledger.yaml`.

## Required Steps

1. Copy the uploaded or source image into `exercises/sources/photos/`.
2. Create or update the topic folder under `exercises/`.
3. Create or update the topic `index.tex`.
4. Create a set file or append to an existing set file.
5. Transcribe the exercise and solution into the TeX set file.
6. Add verification and rework notes.
7. Update `exercise-ledger.yaml`.
8. Regenerate `reports/index.md`, `reports/progress.md`, and
   `reports/rework-queue.md` from the ledger.
9. Hook the topic into `exercises/index.tex`.
10. Run available validation and build checks.

## Refactor Safety

Before moving or renaming any exercise path, read `exercise-ledger.yaml` and
preserve all `exercise_set_id` and item IDs. Only update routing fields such
as `topic`, `path`, and source-photo references when the files actually move.

Do not use folder names, filenames, page order, or source-image filenames as
canonical identities.

## Exercise Set Files

Exercise set files may contain many related items. Keep each item identifiable
inside the file by its stable item ID. Include solution, verification, and
optional rework blocks close to the corresponding item.

The ledger remains the metadata authority, while the TeX file remains the
mathematical artifact.

## Validation

Use the strongest available local checks. Typical checks include:

- YAML parse validation for `exercise-ledger.yaml`;
- duplicate-ID checks for exercise set IDs and item IDs;
- route checks that every ledger `path` exists;
- LaTeX build checks for the owning volume or monorepo.

If a check is unavailable, report that explicitly.
