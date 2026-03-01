# Learning Abstract Mathematics — Project Structure

## Compilation

Compile `main.tex` from the project root in Overleaf.  
All `\input{}` paths are **relative to the project root**.

---

## Volume Layout

```
main.tex                        ← Root file (single \input per volume)
blueprint-macros.tex            ← Shared tcolorbox macros
bibliography/
appendices/
common/
glossary-foundations.tex
map.tex

volume-i/                       ← VOLUME I: Mathematical Logic
  index.tex                     ← \part{Mathematical Logic} + chapter inputs
  propositional-logic/
    index.tex
    notes.tex  |  proofs/  |  worksheets/  |  reading-list.tex  |  capstone.tex
  predicate-logic/
  sets-relations-functions/
  axiom-systems/
  proof-techniques/
  model-theory/                 ← STUB (coming soon)
  type-theory/                  ← STUB (coming soon)

volume-ii/                      ← VOLUME II: Foundations of Formal Number Systems
  index.tex                     ← \part{Foundations…} + chapter inputs
  natural-numbers/              ← STUB (links to volume-i/axiom-systems for Peano)
  integers/
    index.tex
    notes/  |  proofs/tao/  |  proofs/mendelson/
  rationals/                    ← STUB (coming soon)
  reals/                        ← from real-line-foundations
    index.tex
    notes/  |  proofs/  |  worksheets/  |  flashcards.tex
  complex/                      ← STUB (coming soon)

volume-iii/                     ← VOLUME III: Abstract Mathematics
  index.tex                     ← \part{Abstract Mathematics}
  analysis/
    real-analysis/              ← shares content with volume-ii/reals
    metric-spaces/
    point-set-topology/
    measure-theory/             ← STUB (coming soon)
  algebra/
    algebraic-structures/
    set-algebras/
    linear-algebra/
    abstract-algebra/
    algebraic-geometry/
```

---

## Chapter Template (house style)

Each chapter folder contains:

| File | Purpose |
|------|---------|
| `index.tex` | Chapter root: `\chapter{}`, `\section{}`s, `\input{}`s |
| `reading-list.tex` | Annotated bibliography |
| `notes.tex` or `notes/` | Lecture notes with tcolorbox environments |
| `worksheets.tex` or `worksheets/` | Practice problems |
| `proofs/index.tex` | Orchestrator: `\input{...}` each proof file |
| `proofs/<source>/<ID>.tex` | Individual proof file |
| `capstone.tex` | End-of-chapter synthesis |

### Proof File Naming Convention

```
<SUBJECT>-<SOURCE>-<CHAPTER>-<SECTION>-<EXERCISE>.tex
e.g.  PL-BJO-C01-E02a.tex   (Propositional Logic, Bjørndahl, Ch 1, Ex 2a)
      RA-TAO-C04-S4.1-E01.tex
      AA-GAL-C03-E05.tex
```

---

## Status Key

| Symbol | Meaning |
|--------|---------|
| ✓ | Active content present |
| STUB | Placeholder, coming soon |

| Chapter | Status |
|---------|--------|
| Propositional Logic | ✓ |
| Predicate Logic | ✓ |
| Sets, Relations, Functions | ✓ |
| Axiom Systems | ✓ |
| Proof Techniques | ✓ |
| Model Theory | STUB |
| Type Theory | STUB |
| Natural Numbers | STUB (Peano content in Axiom Systems) |
| Integers | ✓ |
| Rationals | STUB |
| Reals | ✓ |
| Complex | STUB |
| Real Analysis | ✓ (shared with Reals) |
| Metric Spaces | ✓ |
| Point-Set Topology | ✓ |
| Measure Theory | STUB |
| Algebraic Structures | ✓ |
| Set Algebras | ✓ |
| Linear Algebra | ✓ |
| Abstract Algebra | ✓ |
| Algebraic Geometry | ✓ |
