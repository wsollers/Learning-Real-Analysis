# Theorem Explorer — Extraction Pipeline

## One-command run

From the repo root:

```
python theorem-explorer/run_extraction.py
```

That's it. The script handles everything.

---

## What it does

**Pass 1** — `extract_lra_chapter.py`

Walks each chapter's `notes/` and `proofs/notes/` trees, extracts every
`definition`, `theorem`, `lemma`, `proposition`, `corollary`, and `axiom`
environment (including those nested inside `tcolorbox`), captures all trailing
`remark*` blocks, and matches each theorem to its proof file via `\hyperref[prf:...]`
links. Writes per-chapter seed files into `<chapter>/.explorer/`.

Output per chapter:
- `<chapter>/.explorer/knowledge-seed.json`
- `<chapter>/.explorer/graph-edges-seed.json`

**Pass 2** — `seed_to_knowledge_json_v3_fixed6.py`

Reads the seed files, interprets house-style remark blocks
(`Standard quantified statement`, `Negated quantified statement`,
`Dependencies`, `Interpretation`, `Proof structure`, etc.),
extracts proof sketches and step lists from proof files,
builds dependency/implication/equivalence edges,
and writes the full explorer-ready JSON.

Output per chapter:
- `<chapter>/.explorer/knowledge.json`
- `<chapter>/.explorer/graph-edges.json`
- `<chapter>/.explorer/proof-errors.json`
- `<chapter>/.explorer/graph-edge-errors.json`

**Merge** — `run_extraction.py`

Combines per-chapter outputs into the combined files the explorer reads:
- `theorem-explorer/knowledge.json`      ← loaded by knowledge-explorer.html
- `theorem-explorer/graph-edges.json`    ← loaded by knowledge-explorer.html
- `theorem-explorer/proof-errors.json`   ← error report
- `theorem-explorer/graph-edge-errors.json`

---

## Chapters extracted

```
volume-ii/natural-numbers
volume-ii/rationals
volume-iii/analysis/bounding
```

To add more chapters, edit the `CHAPTERS` list in `run_extraction.py`.

---

## Viewing the explorer

Open `theorem-explorer/knowledge-explorer.html` in a browser, then either:

**Option A — Local server (recommended, auto-loads JSON)**
```
cd theorem-explorer
python -m http.server 8000
```
Then open: http://localhost:8000/knowledge-explorer.html

The explorer will auto-fetch `knowledge.json` and `graph-edges.json`
from the same folder.

**Option B — Manual file load**

Open `knowledge-explorer.html` directly in a browser, click "Load selected
files", and choose `knowledge.json` and `graph-edges.json` from
`theorem-explorer/`.

---

## Error reports

After running, check:

| File | What it contains |
|---|---|
| `proof-errors.json` | Missing proofs, TODO stubs, missing dependency blocks, predicate notation in standard fields, proof spacing artifacts |
| `graph-edge-errors.json` | Dependency links pointing to labels that don't exist in the extracted node set |

---

## Script reference

| Script | Purpose |
|---|---|
| `run_extraction.py` | One-command runner — calls pass 1 + pass 2 for each chapter, merges output |
| `extract_lra_chapter.py` | Pass 1 — raw LaTeX extractor, writes seed JSON |
| `seed_to_knowledge_json_v3_fixed6.py` | Pass 2 — semantic compiler, writes explorer-ready JSON |
