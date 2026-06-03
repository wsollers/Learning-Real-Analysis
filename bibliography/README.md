# Split Bibliography

This directory is owned by `lra-common`.

Add new entries to exactly one split file:

- `volume-i-foundations.bib`
- `volume-ii-number-systems.bib`
- `volume-iii-analysis.bib`
- `volume-iv-algebra.bib`
- `volume-v-topology-geometry.bib`
- `volume-vi-computational.bib`
- `volume-vii-numerical-approximation.bib`
- `volume-viii-logic-foundations.bib`
- `general-reference.bib`

`analysis.bib` is a legacy pointer only. Do not add entries there.

Before committing, run:

```powershell
python scripts/check_bibliography.py
```

To search before adding a mobile/OCR/extractor candidate, run:

```powershell
python scripts/check_bibliography.py --find "author or title words"
```

Local PDF inventory, ISBN scanning, rename planning, and reading-tab launchers
are documented in:

```text
docs/workflows/reading-library-tools.md
```

Those tools describe local holdings and reading workflows. They do not replace
BibTeX entries, and they do not rename or delete files automatically.
