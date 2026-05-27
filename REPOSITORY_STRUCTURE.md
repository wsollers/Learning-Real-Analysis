# LRA Repository Structure

This file is the canonical map for the Learning Real Analysis multi-repo
workspace. `DESIGN.md` contains the writing and artifact rules; this file
contains the repository ownership and layout rules.

## Source Of Truth Map

| Repository | Canonical ownership | Sync direction |
|---|---|---|
| `lra-governance` | `DESIGN.md`, `REPOSITORY_STRUCTURE.md`, `.gitignore`, `constitution/` | to all repos |
| `Learning-Real-Analysis` | assembled monorepo, omnibus builds, canonical YAML sources, docker, cross-volume tooling | receives volume/common/governance syncs |
| `lra-common` | shared LaTeX infrastructure: `common/`, `bibliography/` | to volume repos and monorepo |
| `lra-volume-i` | Volume I content under `volume-i/` | to monorepo `volume-i/` |
| `lra-volume-ii` | Volume II content under `volume-ii/` | to monorepo `volume-ii/` |
| `lra-volume-iii` | Volume III content under `volume-iii/` | to monorepo `volume-iii/` |
| `lra-volume-iv` | Volume IV content under `volume-iv/` | to monorepo `volume-iv/` |
| `lra-volume-v` | Volume V content under `volume-v/` | to monorepo `volume-v/` |
| `lra-lean` | Lean 4 formalization workspace | independent/specialized |
| `lra-nurbs` | NURBS/DDE C++ engine | independent/specialized |
| `lra-knowledge-explorer` | extraction pipeline and HTML theorem explorer | independent/specialized |

## Governance Repo Layout

```text
lra-governance/
  .github/
    workflows/
      sync-governance.yml
  .gitignore
  DESIGN.md
  REPOSITORY_STRUCTURE.md
  README.md
  constitution/
    master.md
    prompts/
    schema/
    schemas/
    auditor/
```

Governance files are edited here and synced outward. Downstream repos should not
edit their synced copies except as an emergency repair that is immediately
ported back to `lra-governance`.

## Monorepo Layout

```text
Learning-Real-Analysis/
  .gitignore                  synced from lra-governance
  DESIGN.md                   synced from lra-governance
  REPOSITORY_STRUCTURE.md     synced from lra-governance
  constitution/               synced from lra-governance
  main.tex                    omnibus build root
  volume-i-main.tex           per-volume standalone roots
  volume-ii-main.tex
  volume-iii-main.tex
  volume-iv-main.tex
  volume-v-main.tex
  bibliography/               synced from lra-common
  common/                     synced from lra-common
  predicates.yaml             canonical YAML source
  notation.yaml               canonical YAML source
  relations.yaml              canonical YAML source
  volume-i/
  volume-ii/
  volume-iii/
  volume-iv/
  volume-v/
  docker/
  theorem-explorer/
  lean/
  nurbs_dde/
  ontology/
  rules/
```

The monorepo is the assembled master project. It receives volume content from
the volume repos, shared TeX infrastructure from `lra-common`, and governance
from `lra-governance`.

## Common Repo Layout

```text
lra-common/
  .gitignore                  synced from lra-governance
  DESIGN.md                   synced from lra-governance
  REPOSITORY_STRUCTURE.md     synced from lra-governance
  constitution/               synced from lra-governance
  common/
    preamble.tex
    colors.tex
    environments.tex
    macros.tex
    boxes.tex
    exercise-format.tex
    volume-preamble.tex
  bibliography/
    analysis.bib
```

`common/` and `bibliography/` are edited in `lra-common` and propagated outward.

## Volume Repo Layout

Each volume repo is self-contained and Overleaf-ready.

```text
lra-volume-N/
  .gitignore                  synced from lra-governance
  DESIGN.md                   synced from lra-governance
  REPOSITORY_STRUCTURE.md     synced from lra-governance
  constitution/               synced from lra-governance
  main.tex                    Overleaf main document
  .latexmkrc                  local build config
  common/                     synced copy from lra-common
  bibliography/               synced copy from lra-common
  volume-N/
    index.tex
    <chapter>/
      index.tex
      chapter.yaml
      notes/
      proofs/
```

`main.tex` in each volume repo inputs `common/volume-preamble` and
`volume-N/index`. Paths are intentionally the same shape as in the monorepo.

## Canonical Chapter Layout

```text
<chapter>/
  index.tex
  chapter.yaml
  notes/
    index.tex
    <section>/
      notes-<section>.tex
      figure-<n>.tex
  proofs/
    index.tex
    notes/
      index.tex
      prf-<result-id>.tex
    exercises/
      index.tex
      capstone-<chapter>.tex
```

The chapter `index.tex` is the only file that inputs `proofs/index.tex`.
`proofs/notes/index.tex` inputs proof files in dependency order. Exercise and
capstone material lives under `proofs/exercises/`.

## Sync Rules

- Governance files flow from `lra-governance` to every repo.
- Shared LaTeX and bibliography files flow from `lra-common` outward.
- Volume content flows from each `lra-volume-N` repo to the monorepo.
- Governance files are excluded from volume-to-monorepo sync by path scope:
  volume workflows sync only `volume-N/**`.
- Canonical YAML files stay at the monorepo root and are not copied to volume
  repos.
