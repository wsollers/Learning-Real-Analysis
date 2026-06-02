# File Splitting Standards

Source sections: `DESIGN.md` sections 2.5, 2.6, 2.7, 8, 9, 10.1, and 15.

## Atomicity

Each theorem-like environment shall contain exactly one independently
nameable mathematical object.

Every mathematical concept shall be introduced in its own definition
environment and shall possess its own unique label. Grouping multiple
independent mathematical concepts into a single definition environment is
prohibited.

Every nontrivial TikZ figure shall exist as an independent figure source file.
Embedded nontrivial `tikzpicture` environments in note, proof, exercise, or
exposition files are prohibited.

The canonical atomic artifact rules are governed by
`atomic-artifact-standards.md`.

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
