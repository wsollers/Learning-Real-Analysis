# 1. Canonical Repository Structure

## 1.1 Top-Level Layout

```text
Learning-Real-Analysis/
  main.tex                    ← Root compiler: \input per volume only
  bibliography/
    analysis.bib              ← All citation sources
  common/                     ← Shared LaTeX infrastructure (see §9)
    preamble.tex
    colors.tex
    environments.tex
    macros.tex
    boxes.tex
  predicates.yaml             ← Canonical predicate library (see §10)
  notation.yaml               ← Notation and controlled vocabulary
  relations.yaml              ← Inter-predicate relation graph
  proofs-to-do-analysis.md    ← Master proof tracking file (see §12)
  volume-i/                   ← Mathematical Logic
  volume-ii/                  ← Foundations of Formal Number Systems
  volume-iii/                 ← Abstract Mathematics (proof-primary)
  volume-iv/                  ← Applied and Computational Mathematics
```

## 1.2 Volume Contents

| Volume | Contents |
|---|---|
| `volume-i/` | Propositional logic, predicate calculus, sets-relations-functions, axiom systems, proof techniques |
| `volume-ii/` | Natural numbers, integers, rationals, reals, complex numbers |
| `volume-iii/` | Analysis, metric spaces, topology, measure theory, algebra, algebraic geometry |
| `volume-iv/` | Calculus, computational linear algebra, geometry/geometric modelling, ODE (future), probability (future) |

**Rule:** `volume-iv/` is excluded from card extraction by default.

## 1.3 Canonical Chapter Structure

Every chapter in every volume follows this exact layout:

```text
<chapter>/
  index.tex                   ← Chapter entry point: breadcrumb + roadmap + \input chain
  chapter.yaml                ← Machine-readable card data for all environments (see §10)
  notes/
    index.tex                 ← Orchestrator: \input each section in dependency order
    <section>/
      notes-<section>.tex     ← Content file: toolkit box + bare environments
      figure-<n>.tex          ← Extracted TikZ figures only (never inline)
  proofs/
    index.tex                 ← Orchestrator: \input exercises/index + notes/index
    notes/
      index.tex               ← \input each proof in dependency order
      <proof-id>.tex          ← One proof per file (see §6.3 for naming)
    exercises/
      index.tex               ← \input each exercise proof
      <proof-id>.tex          ← Exercise proofs (transient)
      capstone-<chapter>.tex  ← One capstone exercise per chapter (see §1.4)
```

**Rules:**

- `notes/<section>/` has no `index.tex` — the section file **is** the content unit.
- `proofs/notes/index.tex` inputs proof files in dependency order.
- `proofs/exercises/index.tex` inputs exercise proofs; capstone last.
- The chapter `index.tex` is the only file that inputs `proofs/index.tex`.
- `figure-*.tex` files contain only TikZ code — no `\documentclass`, no `\begin{document}`.
- Reading list files are not compiled into the document; use Zotero externally.



## 1.5 Capstone Exercise

Every chapter has exactly one capstone exercise:

- **File:** `proofs/exercises/capstone-<chapter>.tex`
- **Appears last in:** `proofs/exercises/index.tex`
- **Purpose:** a single exercise that synthesises the chapter's core techniques
- **Status tracked in:** `proofs-to-do-analysis.md` (see §12)




