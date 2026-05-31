# Multi-Repo Sync

Source: `REPOSITORY_STRUCTURE.md` and current GitHub Actions workflows.

## Sync Direction

- `lra-governance` syncs governance artifacts outward.
- `lra-common` syncs `common/` and `bibliography/` outward.
- `lra-volume-*` sync volume content into `Learning-Real-Analysis`.
- `lra-lean` syncs into `Learning-Real-Analysis/lean/`.
- `lra-nurbs` syncs into `Learning-Real-Analysis/nurbs_dde/`.
- `Learning-Real-Analysis` dispatches theorem-explorer rebuilds.
- `lra-pdf-extractor` is an independent tool repo and is not currently a
  source of direct sync into notes.

## Full-Replace Policy

Existing workflows use `rsync --delete` for full-replace sync. Future
governance generation must include dry-run output and drift checks before any
full replacement.

Phase 4 generated wrapper work is preview-only. Full-replace downstream sync is
a later phase. Generated wrapper previews must be reviewed before any write mode
or downstream sync mode exists.

Phase 7 introduces controlled, repo-selected wrapper sync planning. The first
planned pilot target is `lra-numerical-analysis`. All-repo wrapper sync remains
blocked until pilot results are reviewed.

## Emergency Local Edits

Emergency downstream edits are temporary. The fix must be ported back to the
owning source repo before the next sync.

## lra-pdf-extractor

`lra-pdf-extractor` is a tool/staging repo. It may generate candidate LaTeX,
BibTeX, JSON, and review artifacts, but those outputs are not automatically
synced into downstream repos.

Integration into `lra-common`, `Learning-Real-Analysis`, `lra-volume-*`, or
`lra-knowledge-explorer` must occur through reviewable PRs in the owning repo.

Future governance sync may deliver generated agent wrappers to this repo, using
the `lra-pdf-extractor` overlay.
