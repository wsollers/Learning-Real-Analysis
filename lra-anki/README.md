# LRA Anki Flashcards

This module contains Anki-importable CSV flashcards for LRA memorization and rule-recognition practice.

## Importing into Anki

1. Open Anki.
2. Choose `File -> Import`.
3. Select one of the CSV files in this module.
4. Set the field separator to comma if Anki does not detect it automatically.
5. Map fields as:
   - `Front` -> Front
   - `Back` -> Back
   - `Tags` -> Tags
6. Enable HTML/LaTeX rendering as appropriate for your Anki setup.

Each CSV has exactly these columns:

```csv
Front,Back,Tags
```

Mathematical content is written in `...`.

## Deck Design

The memorization decks intentionally include both directions:

- prompt to answer,
- answer to prompt.

This applies to derivative rules, elementary derivatives, trigonometric derivatives, inverse trigonometric derivatives, exponential/logarithmic derivatives, chain-rule templates, logarithmic-differentiation templates, and unit-circle values.

The rule-identification decks are not doubled. Their goal is method recognition before computation. A rule-identification card shows an expression, integral, or ODE on the front and names only the method or rule family on the back.

Rule-identification cards should not include full worked solutions. The derivative, integral, and ODE recognition decks are meant to build fluency before computation.

## Validation

Run:

```powershell
python lra-anki/scripts/validate_flashcards.py
```

The validator checks headers, blank fields, math wrapping, forward/reverse coverage for memorization decks, duplicate fronts, and prints per-file counts.
