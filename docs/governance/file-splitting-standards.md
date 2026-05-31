# File Splitting Standards

Source sections: `DESIGN.md` sections 2.5, 2.6, 2.7, 8, 9, 10.1, and 15.

## Atomicity

Each theorem-like environment should contain one named mathematical object.
Do not bundle multiple predicates, operations, relations, conditions, or named
statements into one formal environment.

## Chapter Layout

Chapters follow the canonical layout:

```text
<chapter>/
  index.tex
  chapter.yaml
  notes/
  proofs/
```

The chapter `index.tex` controls chapter-level inputs. Proof indexes control
proof input order.

## File Names

Filenames are lowercase, hyphen-separated, ASCII, and free of LaTeX markup.
Proof filenames align with proof labels and theorem roots.

## Stability

Prefer high-fidelity preservation during refactors. Do not silently reorganize
mathematical files as part of unrelated edits.
