# LRA Repository Structure

This file is the canonical map for the Learning Real Analysis multi-repo
workspace. `DESIGN.md` contains the writing and artifact rules; this file
contains the repository ownership and layout rules.

## Source Of Truth Map

| Repository | Canonical ownership | Sync direction |
|---|---|---|
| `lra-governance` | `DESIGN.md`, `REPOSITORY_STRUCTURE.md`, `.gitignore`, `constitution/` | to all repos |
| `Learning-Real-Analysis` | assembled monorepo, omnibus builds, canonical YAML sources, docker, cross-volume tooling | receives volume/common/governance syncs; dispatches knowledge rebuild |
| `lra-common` | shared LaTeX infrastructure: `common/`, `bibliography/` | to volume repos and monorepo |
| `lra-volume-i` | Volume I content under `volume-i/` | to monorepo `volume-i/` |
| `lra-volume-ii` | Volume II content under `volume-ii/` | to monorepo `volume-ii/` |
| `lra-volume-iii` | Volume III content under `volume-iii/` | to monorepo `volume-iii/` |
| `lra-volume-iv` | Volume IV content under `volume-iv/` | to monorepo `volume-iv/` |
| `lra-volume-v` | Volume V content under `volume-v/` | to monorepo `volume-v/` |
| `lra-volume-vi` | Volume VI content under `volume-vi/` | to monorepo `volume-vi/` |
| `lra-volume-vii` | Volume VII content under `volume-vii/` | to monorepo `volume-vii/` |
| `lra-volume-viii` | Volume VIII content under `volume-viii/` | to monorepo `volume-viii/` |
| `lra-lean` | Lean 4 formalization workspace | independent/specialized |
| `lra-nurbs` | NURBS/DDE C++ engine | independent/specialized |
| `lra-knowledge-explorer` | extraction pipeline, HTML theorem graph, GitHub Pages site | triggered by monorepo CI dispatch |

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
  .github/
    workflows/
      trigger-knowledge-rebuild.yml   dispatches lra-rebuild to lra-knowledge-explorer
  main.tex                    omnibus build root
  volume-i-main.tex           per-volume standalone roots
  volume-ii-main.tex
  volume-iii-main.tex
  volume-iv-main.tex
  volume-v-main.tex
  volume-vi-main.tex
  volume-vii-main.tex
  volume-viii-main.tex
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
  volume-vi/
  volume-vii/
  volume-viii/
  docker/
  theorem-explorer/
  lean/
  nurbs_dde/
  ontology/
  rules/
```

The monorepo is the assembled master project. It receives volume content from
the volume repos, shared TeX infrastructure from `lra-common`, and governance
from `lra-governance`. It is also the single dispatch point for the knowledge
explorer rebuild (see Knowledge Explorer Pipeline below).

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
- When LaTeX source content lands in the monorepo on `main`, the monorepo CI
  dispatches a rebuild event to `lra-knowledge-explorer` (see below).

---

## Knowledge Explorer Pipeline

The knowledge explorer at `lra-knowledge-explorer` is rebuilt automatically
whenever LaTeX source content is pushed to the monorepo. The full chain is:

```
lra-common push        ─┐
lra-volume-i push       ├─► Learning-Real-Analysis (sync CI)
lra-volume-* push       │     └── trigger-knowledge-rebuild.yml
       ...             ─┘               └── repository_dispatch: lra-rebuild
                                                  └── lra-knowledge-explorer
                                                        ├── checkout LRA
                                                        ├── run extraction pipeline
                                                        ├── commit knowledge.json
                                                        └── deploy to GitHub Pages
```

The live explorer is published at:

```
https://wsollers.github.io/lra-knowledge-explorer/
```

### Why dispatch from the monorepo, not from each leaf repo

All leaf repos (`lra-common`, `lra-volume-*`) already sync their content to
`Learning-Real-Analysis` on push. The monorepo is therefore the correct and
only place to watch for content changes — it is the single integration point.
Putting dispatch triggers in each leaf repo would cause redundant rebuilds
and would require every leaf to hold the `SYNC_PAT` secret.

### Trigger path filter

The monorepo workflow (`.github/workflows/trigger-knowledge-rebuild.yml`)
fires only when these paths change on `main`:

| Path | Reason |
|---|---|
| `volume-i/**` through `volume-viii/**` | Volume LaTeX chapters extracted by the pipeline |
| `common/**` | Shared macro/environment changes can affect extraction output |
| `bibliography/**` | Same reasoning |
| `theorem-explorer/**.py` | Script changes in the pipeline itself warrant a rebuild |

Cosmetic or structural files (`DESIGN.md`, `BACKLOG.md`, `curriculum.html`,
etc.) do not trigger a rebuild.

### Required secret

The workflow requires a repository secret named `SYNC_PAT` — a GitHub PAT
with `repo` scope. This is the same PAT used by the leaf-repo sync workflows.
Add it at:

```
https://github.com/wsollers/Learning-Real-Analysis/settings/secrets/actions
```

### Knowledge Explorer repo layout

```text
lra-knowledge-explorer/
  .github/
    workflows/
      rebuild.yml             triggered by repository_dispatch lra-rebuild
  index.html                  redirect → knowledge-explorer.html
  knowledge-explorer.html     interactive theorem graph (GitHub Pages entry)
  real-analysis-explorer.html alternate UI
  scripts/
    extract_lra_chapter.py    Pass 1: LaTeX → seed JSON
    seed_to_knowledge_json_v3_fixed6.py   Pass 2: seed → explorer JSON
    run_extraction.py         orchestrator
  knowledge.json              generated — overwritten by CI
  graph-edges.json            generated — overwritten by CI
  PIPELINE.md                 pipeline documentation
```

For full pipeline documentation see `PIPELINE.md` in `lra-knowledge-explorer`.

### Manual rebuild

Go to **Actions → Rebuild Knowledge Explorer → Run workflow** in
`lra-knowledge-explorer` to trigger a rebuild without a code push.
