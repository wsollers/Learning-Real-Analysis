# Repository Layout

Source: `REPOSITORY_STRUCTURE.md`.

## Source Of Truth Map

| Repository | Canonical ownership | Sync direction |
| --- | --- | --- |
| `lra-governance` | Governance docs, architecture docs, repo overlays, prompts, schemas, generators, sync policy. | source for generated governance artifacts |
| `Learning-Real-Analysis` | Assembled monorepo, omnibus builds, canonical YAML sources, docker, cross-volume integration. | receives volume/common/governance syncs |
| `lra-common` | Shared LaTeX infrastructure: `common/`, split `bibliography/`, and bibliography helper scripts. | to volume repos and monorepo |
| `lra-volume-i` through `lra-volume-viii` | Volume content under `volume-N/`. | to monorepo `volume-N/` |
| `lra-lean` | Lean 4 formalization workspace. | to monorepo `lean/` |
| `lra-nurbs` | C++ / Vulkan / geometry / simulation workspace. | to monorepo `nurbs_dde/` |
| `lra-knowledge-explorer` | Extraction pipeline and HTML theorem explorer. | receives rebuild dispatch from monorepo |
| `lra-numerical-analysis` | Numerical methods, computational experiments, benchmarks, plots, numerical reports. | independent/specialized |
| `lra-pdf-extractor` | PDF/source ingestion, bibliography extraction and normalization, local-model-assisted candidate extraction/decorating, Python GUI review workflow, staged LaTeX/BibTeX/JSON outputs. | independent tool repo; produces reviewable candidates only |

`lra-pdf-extractor` is an acceleration and staging tool. It does not own
downstream notes, bibliography, canonical YAML, theorem explorer internals, or
governance rules.

## Downstream Governance Copies

Downstream governance files should be generated from `lra-governance`, not
edited as independent sources.
