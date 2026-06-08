# Repository Layout

Source: `REPOSITORY_STRUCTURE.md`.

## Source Of Truth Map

| Repository | Canonical ownership | Sync direction |
| --- | --- | --- |
| `lra-governance` | Governance docs, architecture docs, repo overlays, prompts, schemas, constitution files, generators, sync policy. | source for generated governance artifacts |
| `Learning-Real-Analysis` | Assembled monorepo, omnibus builds, canonical YAML sources, docker, cross-volume integration. | receives volume/common/governance syncs |
| `lra-common` | Shared LaTeX infrastructure: `common/`. | to volume repos and monorepo |
| `lra-volume-i` through `lra-volume-viii` | Volume content under `volume-N/`. | to monorepo `volume-N/` |
| `lra-lean` | Lean 4 formalization workspace. | to monorepo `lean/` |
| `lra-nurbs` | C++ / Vulkan / geometry / simulation workspace. | to monorepo `nurbs_dde/` |
| `lra-knowledge-explorer` | Extraction pipeline and HTML theorem explorer. | receives rebuild dispatch from monorepo |
| `lra-numerical-analysis` | Numerical methods, computational experiments, benchmarks, plots, numerical reports. | independent/specialized |
| `lra-pdf-extractor` | PDF/source ingestion, bibliography extraction and normalization, local-model-assisted candidate extraction/decorating, Python GUI review workflow, staged LaTeX/BibTeX/JSON outputs. | independent tool repo; produces reviewable candidates only |
| `lra-source-profiles` | Dynamic source profiles, candidate mathematical source classification, active volume/chapter source indexes, stable project attachment exports, and source review workflow. | independent profile/staging repo; produces reviewed source-profile artifacts only |

`lra-pdf-extractor` is an acceleration and staging tool. It does not own
downstream notes, bibliography, canonical YAML, theorem explorer internals, or
governance rules.

`lra-source-profiles` is a source selection and profile staging tool. It does
not own final LRA note content, final bibliography shards, canonical YAML,
theorem explorer internals, or governance rules.

## Downstream Governance Copies

Downstream governance files should be generated from `lra-governance`, not
edited as independent sources.

Governance tool implementations remain canonical in
`lra-governance/tools/governance/`. Downstream leaf repositories may contain
thin wrappers at matching paths so local commands keep working, but those
wrappers delegate to `lra-governance` and fail when the canonical checkout is
unavailable.
