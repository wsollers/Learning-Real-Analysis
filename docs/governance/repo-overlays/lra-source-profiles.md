# lra-source-profiles Overlay

This overlay applies to `lra-source-profiles`.

## Owned Concerns

`lra-source-profiles` owns tooling and metadata for:

- dynamic source profiles for Learning Real Analysis,
- candidate mathematical source classification,
- proposed category placement,
- volume/chapter source indexes,
- reusable named source profiles,
- active source selections,
- stable active-profile exports for project attachment slots,
- cached Markdown extracts derived from reviewed source metadata,
- conservative PDF import, scan, move, and review workflows,
- source manifest validation and profile audit workflows.

## Non-Owned Concerns

`lra-source-profiles` does not own:

- final LRA note content,
- final volume-owned bibliography shards,
- canonical predicate / notation / relation YAML,
- shared LaTeX infrastructure,
- theorem explorer internals or generated graph data,
- global governance standards,
- Lean formalization rules,
- NURBS / Vulkan / simulation rules,
- numerical-analysis benchmark rules,
- PDF-extractor implementation rules.

## Project Overlay Abilities

Agents working in this repo may use its project-local tools to:

- scan configured reading roots and copy PDFs into local working folders,
- classify candidate sources into proposed categories,
- maintain `volumes/<volume>/<chapter>/source-index.yaml` metadata,
- write approved selections to `active-sources.yaml`,
- maintain `misplaced-sources.yaml` and the misplaced review queue,
- generate and validate source indexes,
- export active profiles into stable `active-profile/sourceNN.md` slots,
- export named profiles into the same stable attachment-slot shape,
- import new PDFs with optional known Google Drive URLs,
- generate or refresh per-source Markdown cache files.

These abilities are staging and review abilities. They do not authorize direct
writes into volume notes, final bibliography shards, canonical YAML, or theorem
explorer generated data.

## Safety Rules

Original PDFs outside the repository, including configured reading roots such
as `D:\readings`, must not be modified.

Do not delete original files, overwrite existing source files silently, or
invent bibliographic data. Uncertain metadata must remain marked for review
with confidence and notes.

Local copied PDFs and generated scratch artifacts should remain untracked unless
a task explicitly promotes a specific artifact to reviewed source material.

When moving sources between categories or chapters, preserve source IDs, hash
metadata, review notes, and duplicate-status information unless an explicit
cleanup task says otherwise.

## Integration Boundary

Outputs from `lra-source-profiles` may inform authoring, bibliography, and
source-review work in other repos, but integration must happen through the
owning repository's normal review path.

If source-profile work reveals that final note content, bibliography entries,
canonical YAML, or extraction records need changes, report the owning repo and
required follow-up rather than applying those changes from this repo.
