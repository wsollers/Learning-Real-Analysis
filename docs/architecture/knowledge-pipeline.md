# Knowledge Pipeline Architecture

The LRA knowledge pipeline turns LaTeX source into reviewable graph and
explorer artifacts. It depends on stable mathematical structure, not on
provider-specific agent behavior.

## Ownership

- `Learning-Real-Analysis` owns the integrated LaTeX source tree and canonical
  YAML sources.
- `lra-knowledge-explorer` owns theorem extraction, graph generation, explorer
  internals, and generated explorer data.
- `lra-pdf-extractor` may produce candidate source-ingestion artifacts, but it
  does not own final notes, graph data, canonical YAML, or explorer internals.
- `lra-source-profiles` may produce reviewed source-profile selections,
  source indexes, and cached source extracts, but it does not own final notes,
  graph data, canonical YAML, or explorer internals.
- `lra-governance` owns the standards that make extraction possible.

## Extraction Inputs

The pipeline relies on:

- one theorem-like object per formal environment,
- stable statement labels,
- dependency remarks with `\hyperref[label]{Readable Name}` items,
- statement labels using `def:`, `ax:`, `thm:`, `lem:`, `prop:`, or `cor:`,
- proof labels using `prf:` only for proof locations,
- canonical chapter and proof file structure,
- canonical YAML from `Learning-Real-Analysis`.

Dependency edges must target mathematical statement labels. A `prf:` label may
support theorem/proof navigation, but it is not a dependency target.

## Graph And Explorer Outputs

Extractor output should preserve enough metadata to identify:

- source repository and file,
- statement environment and label,
- display title,
- dependency labels,
- proof availability,
- source line or structural location when available.

Generated explorer data is not authored directly in volume repos. It is derived
from the integrated source tree and owned by the theorem explorer pipeline.

## Candidate Artifacts

Candidate artifacts from `lra-pdf-extractor`, `lra-source-profiles`, or other
ingestion and source-profile tools must be staged and reviewed before entering
notes, bibliography, canonical YAML, or explorer data. Local model or Ollama
output is advisory only and must not be treated as a mathematical authority.

## Maintenance

If authoring standards change label, dependency, proof, or logical-block
structure, update the extraction standards and theorem explorer pipeline before
assuming generated graph output remains valid.
