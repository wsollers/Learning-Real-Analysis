# lra-pdf-extractor Overlay

This overlay applies to `lra-pdf-extractor`.

## Owned Concerns

`lra-pdf-extractor` owns tooling for:

- PDF and source-file ingestion,
- text extraction and cleanup,
- bibliography metadata extraction and normalization,
- duplicate bibliography detection,
- candidate `.bib` entry generation,
- candidate mathematical artifact extraction,
- note decoration assistance,
- local-model-assisted cleanup and classification,
- Python GUI workflows for reviewing and organizing extracted candidates,
- staged LaTeX, BibTeX, JSON, and review-queue outputs.

## Non-Owned Concerns

`lra-pdf-extractor` does not own:

- final LRA note content,
- canonical predicate / notation / relation YAML,
- shared LaTeX infrastructure,
- theorem explorer internals,
- global governance standards,
- generated agent instruction wrappers,
- Lean formalization rules,
- NURBS / Vulkan / simulation rules,
- numerical-analysis benchmark rules,
- volume-specific authoring rules.

## Integration Boundary

`lra-pdf-extractor` may produce candidate artifacts for other repos, but those
artifacts must be reviewed and applied through normal PRs in the owning
repository.

It must not directly overwrite:

- volume note files,
- `lra-common/bibliography/analysis.bib`,
- canonical YAML files,
- theorem explorer generated data,
- governance files.

## Local Model Use

Local models may assist with:

- OCR cleanup suggestions,
- hyphenation and ligature repair suggestions,
- source metadata extraction,
- candidate artifact classification,
- low-risk summarization,
- draft decoration suggestions.

Local models must not be treated as authorities for:

- final mathematical rewrites,
- invented predicates,
- invented labels,
- direct note insertion,
- silent bulk edits,
- canonical bibliography updates.

## Future Agent Rules

When generated agent wrappers are implemented, this repo should receive:

- global safety and ownership rules,
- this `lra-pdf-extractor` overlay,
- Python tooling guidance,
- GUI workflow guidance,
- local-model safety guidance.

It should not receive Lean, NURBS/Vulkan, volume-only, or
numerical-analysis-specific overlays.

