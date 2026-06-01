# Phase 4B Numerical Analysis Migration Scaffolding

## Executive summary

Phase 4B prepares the migration of numerical analysis from Volume V to Volume VII. The source content remains in `volume-v/numerical-analysis`; this phase creates only a destination scaffold in Volume VII and records the label, reference, citation, and path audit needed for the actual move.

Recommendation: go for a Phase 4C move, but keep it as a single whole-cluster move with a temporary router at the old Volume V path. Do not split individual numerical-analysis notes during the first move.

## Source cluster inventory

Source cluster: `volume-v/numerical-analysis`

File count: 12 files.

Files:

- `volume-v/numerical-analysis/chapter.yaml`
- `volume-v/numerical-analysis/index.tex`
- `volume-v/numerical-analysis/notes/index.tex`
- `volume-v/numerical-analysis/notes/ieee-floating-point/index.tex`
- `volume-v/numerical-analysis/notes/ieee-floating-point/notes-ieee-floting-point.tex`
- `volume-v/numerical-analysis/notes/interval-arithmetic/index.tex`
- `volume-v/numerical-analysis/notes/interval-arithmetic/notes-interval-arithmetic-rn.tex`
- `volume-v/numerical-analysis/notes/interval-arithmetic/notes-interval-arithmetic.tex`
- `volume-v/numerical-analysis/proofs/index.tex`
- `volume-v/numerical-analysis/proofs/exercises/capstone-numerical-analysis.tex`
- `volume-v/numerical-analysis/proofs/exercises/index.tex`
- `volume-v/numerical-analysis/proofs/notes/index.tex`

## Labels defined in the source cluster

The source cluster defines 13 labels:

- `def:interval-add`
- `def:interval-sub`
- `def:interval-mult`
- `def:interval-contain`
- `sec:interval-arithmetic-rn`
- `def:box`
- `def:box-contain`
- `def:box-add`
- `def:box-sub`
- `def:box-smul`
- `def:box-hadamard`
- `rem:norm-bounds`
- `prf:capstone-numerical-analysis`

There are 10 definition labels, 1 section label, 1 remark label, and 1 proof label.

## References, citations, and inputs found in the source cluster

The source cluster contains no `\ref`, `\cref`, `\Cref`, or `\cite` commands.

It contains internal `\hyperref[...]` links to labels defined in the same cluster:

- `def:interval-add`
- `def:interval-sub`
- `def:interval-mult`
- `def:interval-contain`
- `def:box`
- `def:box-contain`
- `def:box-add`
- `def:box-sub`
- `def:box-smul`
- `def:box-hadamard`
- `rem:norm-bounds`

It contains root-relative inputs hard-coded to the current source path:

- `\input{volume-v/numerical-analysis/notes/index}`
- `\input{volume-v/numerical-analysis/proofs/notes/index}`
- `\input{volume-v/numerical-analysis/proofs/exercises/index}`
- `\input{volume-v/numerical-analysis/notes/ieee-floating-point/index}`
- `\input{volume-v/numerical-analysis/notes/interval-arithmetic/index}`
- `\input{volume-v/numerical-analysis/notes/ieee-floating-point/notes-ieee-floting-point}`
- `\input{volume-v/numerical-analysis/notes/interval-arithmetic/notes-interval-arithmetic}`
- `\input{volume-v/numerical-analysis/notes/interval-arithmetic/notes-interval-arithmetic-rn}`
- `\input{volume-v/numerical-analysis/proofs/exercises/capstone-numerical-analysis}`

## External references into the source cluster

Path references outside the source cluster:

- `volume-v/index.tex` inputs `volume-v/numerical-analysis/index`.
- `docs/plans/phase-4a-content-migration-map.md` names the cluster as the recommended first move target.

No external references to labels defined inside the source cluster were found in TeX, Markdown, or YAML files. The only references to those labels are internal `\hyperref[...]` links from the local interval-arithmetic summary tables to local definitions and remarks.

## Relative-path risks

There are no local relative inputs such as `\input{notes/index}` in the source cluster. However, the source cluster is not path-neutral: it uses root-relative inputs hard-coded to `volume-v/numerical-analysis`.

During Phase 4C, those input paths must be rewritten to `volume-vii/numerical-analysis` as part of the move. The `chapter.yaml` fields must also change from:

- `volume: volume-v`
- `path: volume-v/numerical-analysis`

to:

- `volume: volume-vii`
- `path: volume-vii/numerical-analysis`

The file `notes-ieee-floting-point.tex` appears to contain a typo in `floting`. Phase 4C should preserve that filename unless a separate rename is explicitly approved, because renaming it would add avoidable risk.

## Destination scaffold created

The following destination scaffold was created:

- `volume-vii/numerical-analysis/index.tex`
- `volume-vii/numerical-analysis/chapter.yaml`
- `volume-vii/numerical-analysis/notes/index.tex`
- `volume-vii/numerical-analysis/proofs/index.tex`
- `volume-vii/numerical-analysis/proofs/notes/index.tex`
- `volume-vii/numerical-analysis/proofs/exercises/index.tex`

`volume-vii/index.tex` now inputs `volume-vii/numerical-analysis/index` after the planned-volume status box.

The scaffold intentionally does not duplicate the source mathematical content.

## Recommended router strategy

In Phase 4C, move the full cluster to `volume-vii/numerical-analysis`, then replace the old `volume-v/numerical-analysis/index.tex` with a temporary router:

```tex
% Temporary router. Canonical content moved to Volume VII in Phase 4C.
\input{volume-vii/numerical-analysis/index}
```

The old `volume-v/index.tex` can continue to input `volume-v/numerical-analysis/index` during the router window. A later cleanup phase should remove the old router and update Volume V to its topology and geometry content after downstream split repositories and builds are stable.

Avoid creating duplicate full content under both paths. During the router window, only the old router entry point should remain under `volume-v/numerical-analysis`.

## Exact proposed Phase 4C move plan

1. Confirm the working tree is clean except known optional untracked images.
2. Re-run the label/reference inventory for `volume-v/numerical-analysis`.
3. Replace the Phase 4B destination scaffold with the moved source cluster.
4. Rewrite all moved `\input{volume-v/numerical-analysis/...}` paths to `\input{volume-vii/numerical-analysis/...}`.
5. Update moved `chapter.yaml` metadata to `volume: volume-vii` and `path: volume-vii/numerical-analysis`.
6. Preserve all labels exactly:
   - `def:interval-add`
   - `def:interval-sub`
   - `def:interval-mult`
   - `def:interval-contain`
   - `sec:interval-arithmetic-rn`
   - `def:box`
   - `def:box-contain`
   - `def:box-add`
   - `def:box-sub`
   - `def:box-smul`
   - `def:box-hadamard`
   - `rem:norm-bounds`
   - `prf:capstone-numerical-analysis`
7. Replace the old source entry point with a temporary router at `volume-v/numerical-analysis/index.tex`.
8. Remove or leave empty old subdirectories only if the repository policy for router stubs allows it; otherwise keep no duplicate full content.
9. Build `volume-vii-main.tex`.
10. Build `volume-v-main.tex` to verify the temporary router still works.
11. Confirm root `main.tex` was not changed.
12. Confirm no labels, citations, theorem identifiers, or proof identifiers changed.

## Go/no-go recommendation

Go for Phase 4C as a whole-cluster move with a temporary router.

Rationale:

- The source cluster is small.
- No external label references were found.
- No citations were found.
- Existing label references are internal `\hyperref[...]` links.
- The main risk is mechanical path rewriting, which is manageable if the move is done atomically and labels are preserved.

Do not split the cluster during Phase 4C. The first move should be a path migration only.
