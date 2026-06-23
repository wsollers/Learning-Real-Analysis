# Knowledge Extraction Workflow

Knowledge extraction turns stable LaTeX source into graph and explorer
artifacts.

## Inputs

Use the local split volume repositories as the canonical TeX source for
explorer extraction:

- `lra-volume-i`
- `lra-volume-ii`
- `lra-volume-iii`

The integrated `Learning-Real-Analysis` monorepo must not be used as an
extraction source; it may lag the split repositories. Missing expected volume
repositories, missing chapter directories, missing `notes/index.tex`, or
missing `proofs/index.tex` are hard errors.

Canonical YAML remains in `Learning-Real-Analysis` until those shared metadata
files are split out:

- `predicates.yaml`
- `notation.yaml`
- `relations.yaml`

Tools that need canonical YAML should receive the monorepo root explicitly, but
formal statements and proofs must be read from the split volume repos.

## Expected Source Shape

Extraction assumes source follows `docs/governance/extraction-standards.md`,
`docs/governance/dependency-standards.md`, and the structured rules in
`constitution/schema/block-registry.yaml`,
`constitution/schema/artifact-matrix.yaml`, and
`constitution/schema/file-schema.yaml`.

## Review

Generated graph, JSON, or explorer artifacts should be reviewed against the
source labels and dependency blocks. If extraction exposes missing labels,
missing dependencies, or ambiguous predicate needs, report them as source or
governance issues rather than inventing replacements.

## Governance-Owned Refresh

When the refresh is initiated from a local multi-repo checkout, run it from
`lra-governance` instead of routing through `Learning-Real-Analysis`.

Stage 1 prepares repositories:

- verify `lra-governance`, `lra-proof-vault`, every `lra-volume-*` repo, and
  `lra-knowledge-explorer` are on expected branches;
- verify those repos are clean and even with their GitHub remotes;
- pull only when a repo is clean and behind;
- fail loudly on local dirt, divergent history, or missing repositories.

Use the governance preflight command:

```powershell
python tools\governance\extraction_pipeline\preflight.py
```

For local development immediately after intentional checkpoint commits, use
`--allow-ahead` to allow clean repositories that have not yet been pushed:

```powershell
python tools\governance\extraction_pipeline\preflight.py --allow-ahead
```

Stage 2 generates data:

- regenerate `chapter.yaml` from volume source;
- validate dependency block shape and source compliance;
- extract formal nodes, authored proof files, and dependency edges from the
  split volume repos with the same live TeX file inventory provider used by the
  validators (`tools/governance/core/file_inventory.py`);
- fail if extraction and validation would not be reading the same reachable
  `notes/index.tex` and `proofs/index.tex` closures.

The first conservative Stage 2 command is read-only against the volume repos.
It derives a chapter manifest from source, extracts dependency data, validates
the graph, and writes ignored run artifacts:

```powershell
python tools\governance\extraction_pipeline\generate_data.py
```

Generated run artifacts live under `runs/extraction-*`. Raw logs and human
reports live under `logs/` and are ignored.

Stage 3 combines data:

- merge per-volume extraction output into canonical knowledge artifacts;
- validate labels, dependency targets, duplicate nodes, and schema shape;
- reject legacy dependency forms unless the run is explicitly a migration run.

Stage 4 deploys data:

- copy validated explorer artifacts into `lra-knowledge-explorer`;
- confirm proof tabs in the explorer are populated from live volume proof files
  before falling back to proof-vault handwritten attempt records;
- build `proof-vault-index.json` from route-style metadata in
  `lra-proof-vault`, including reviewed proof attempt OCR text, Markdown, and
  TeX display artifacts when `ocr_text_path`, `markdown_path`, and `tex_path`
  are present;
- commit and push generated data from that repo;
- invoke the Pages/deploy workflow only when commit/push does not already do it.

Stage 5 generates audit data:

- generate `lra-knowledge-explorer/reorder` batches from the same combined
  canonical data;
- initially include definitions and theorem-like statements;
- later extend the batch generator to proof dependency routes.

Stage 6 reports:

- write a raw `logs/extraction-log-YYYYMMDD-HHMMSS.log`;
- write a human `logs/extraction-report-YYYYMMDD-HHMMSS.md`;
- include repo SHAs, touched files, processed counts, validation failures,
  legacy-shape findings, generated artifact paths, and deploy status.

Before changing the pipeline, archive the current comparison baseline with:

```powershell
python tools\governance\extraction_pipeline\archive_baseline.py
```

Committed baselines live under `baselines/extraction/`. Timestamped logs and
reports remain local generated output and are ignored.
