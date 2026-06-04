# Bibliography Entry Workflow

Bibliography is owned by the volume that uses it. Do not add new canonical
BibTeX entries to `lra-common`.

Each `lra-volume-*` repository maintains the bibliography shard needed for its
standalone build:

- `lra-volume-i/bibliography/volume-i-foundations.bib`
- `lra-volume-ii/bibliography/volume-ii-number-systems.bib`
- `lra-volume-iii/bibliography/volume-iii-analysis.bib`
- `lra-volume-iv/bibliography/volume-iv-algebra.bib`
- `lra-volume-v/bibliography/volume-v-topology-geometry.bib`
- `lra-volume-vi/bibliography/volume-vi-computational.bib`
- `lra-volume-vii/bibliography/volume-vii-numerical-approximation.bib`
- `lra-volume-viii/bibliography/volume-viii-logic-foundations.bib`

## Standard Process

1. Identify the volume whose notes cite the source.
2. Search that volume's bibliography shard for an existing entry or nearby key.
3. Add the entry to that volume-owned `.bib` file only.
4. Use the entry from the volume content with natbib-compatible commands such
   as `\citep{...}` or `\citet{...}`.
5. Build the volume root, for example `latexmk -lualatex main.tex` from the
   owning volume repo.
6. Let the volume sync workflow copy both `volume-N/**` and the volume
   bibliography shard into `Learning-Real-Analysis`.

## Monorepo Assembly

The monorepo full build assembles the bibliography by listing the volume-owned
shards in `main.tex`. Per-volume monorepo roots list only their corresponding
volume shard.

The monorepo bibliography directory is an integration target. Do not edit a
monorepo bibliography shard as the source of truth; make the change in the
owning volume repo and sync it in.

## Candidate Material

Extractor-generated `.bib` output is candidate material only. Review candidate
entries before adding them to the owning volume shard. Do not write candidate
entries into `lra-common` or an aggregate bibliography file.