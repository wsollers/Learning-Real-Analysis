# Analysis Backlog — Gaps & Novel Concepts to Add

Generated from audit of uploaded lecture notes and Applebaum,
*Limits, Limits Everywhere* (OUP, 2012) against the live `volume-iii` chapters.

---

## ⚠️ Immediate Reminder

> **Add Dedekind cuts after finishing the rational number theorems.**
> Applebaum §11.1 and your existing `volume-iii` notes both point here.
> This closes the loop: rational theorems → Dedekind cuts → completeness → analysis proper.
> The construction via cuts gives `sup(c)` a concrete meaning and grounds everything
> in `bounding` on something more than an axiom.

---

## Priority 1 — Sequences Chapter (WIP Today)

These are confirmed absent from `volume-iii/analysis/sequences/chapter.yaml`.
Do these in order — each one is a prerequisite for the next.

### Definitions to Add

| Label | Display Title | Notes |
|---|---|---|
| `def:bounded-sequence` | Bounded Sequence | Distinct from bounded *sets*. A sequence `(aₙ)` is bounded if `∃ K > 0` such that `|aₙ| ≤ K` for all `n`. Add bounded-above and bounded-below variants. |
| `def:null-sequence` | Null Sequence | A sequence that converges to zero. Simple but named — used constantly in proofs. |
| `def:oscillates` | Oscillating Sequence | A sequence that is neither convergent nor properly divergent. |
| `def:oscillates-finitely` | Finitely Oscillating Sequence | Oscillates and is bounded. |
| `def:oscillates-infinitely` | Infinitely Oscillating Sequence | Oscillates and is not bounded. |
| `def:properly-divergent` | Properly Divergent Sequence | Diverges to `+∞` or `−∞`. |

**Note on the divergence taxonomy:** Applebaum §4.2 presents this classification
in one clean block — the cleanest presentation encountered so far. The key insight
is that divergence subdivides into *proper divergence* (escapes to ±∞) and
*oscillation* (neither convergent nor properly divergent), and oscillation further
subdivides by boundedness. This is the taxonomy that most books bury or omit.

### Theorems to Add

| Label | Display Title | Source | Notes |
|---|---|---|---|
| `thm:convergent-implies-bounded` | Convergent Sequences Are Bounded | Applebaum Thm 4.2.1 | Every convergent sequence is bounded. Proof uses MFT + triangle inequality. Key corollary: unbounded ⟹ divergent. |
| `thm:nonneg-sequence-limit-nonneg` | Non-negative Sequence Has Non-negative Limit | Applebaum Thm 4.1.2 | If `aₙ ≥ 0` for all `n` and `aₙ → l`, then `l ≥ 0`. Proof by contradiction. Delicate: `aₙ > 0` does NOT imply `l > 0`. |
| `thm:divergence-comparison` | Comparison Theorem for Divergence | Applebaum Thm 4.1.3 | If `aₙ ≤ bₙ` for all `n`: (1) `aₙ → +∞` implies `bₙ → +∞`; (2) `bₙ → −∞` implies `aₙ → −∞`. |
| `thm:null-times-bounded-is-null` | Null × Bounded = Null | Applebaum Thm 4.2.2 | If `(aₙ)` is null and `(bₙ)` is bounded, then `(aₙbₙ)` is null. Short proof, high payoff — used to dispatch e.g. `cos(n)/n²`. |
| `thm:monotone-convergence-theorem` | Monotone Convergence Theorem | Applebaum Thm 5.2.1 | (1) Bounded above + monotone increasing ⟹ converges to `sup(aₙ)`. (2) Bounded below + monotone decreasing ⟹ converges to `inf(aₙ)`. Load-bearing existence theorem. |
| `cor:monotone-sequences-converge-or-diverge` | Monotone Sequences Converge or Properly Diverge | Applebaum Cor 5.2.1 | (1) Monotone increasing ⟹ converges or diverges to `+∞`. (2) Monotone decreasing ⟹ converges or diverges to `−∞`. Follows from MCT. |
| `thm:geometric-sequence-behavior` | Behavior of the Geometric Sequence `rⁿ` | Applebaum §4.1.1 | Complete case analysis on `r`: `r < −1` oscillates infinitely; `r = −1` oscillates finitely; `−1 < r < 1` converges to 0; `r = 1` converges to 1; `r > 1` diverges to `+∞`. Excellent formalized worked example. |

### Sequence-Indexed Sup/Inf Results (bridge from `bounding` to `sequences`)

The `bounding` chapter proves these for *sets*. The *sequence-indexed* versions
are distinct statements and belong in `sequences`.

| Label | Display Title | Notes |
|---|---|---|
| `lem:sequence-sup-inf-monotone-under-pointwise-order` | Sup/Inf Are Monotone Under Pointwise Order of Sequences | If `bₙ ≥ cₙ` for all `n`, then `inf(bₙ) ≥ inf(cₙ)` and `sup(bₙ) ≥ sup(cₙ)`. This is Applebaum Lemma 5.1 part (2) in sequence language. |
| `thm:sup-subadditive-for-sequences` | Sup of Sum Is at Most Sum of Sups (Sequences) | `sup(aₙ + bₙ) ≤ sup(aₙ) + sup(bₙ)`. Sequence version of the set result already in `bounding`. |

---

## Priority 2 — Inequalities Chapter

Verify these in `volume-iii/analysis/real-analysis` before adding — they may already exist.

| Label | Display Title | Source | Status |
|---|---|---|---|
| `thm:am-gm-inequality` | Arithmetic Mean–Geometric Mean Inequality | Applebaum Thm 3.5.1 (Theorem of the Means) | Verify in `real-analysis` subdir |
| `thm:cauchy-schwarz-sums` | Cauchy–Schwarz Inequality for Sums | Applebaum §3, Exercise 9 | Verify in `real-analysis` subdir |
| `cor:reverse-triangle-inequality` | Reverse Triangle Inequality | Applebaum Cor 3.3.1 | `‖|a| − |b|‖ ≤ |a − b|`. Clean corollary to triangle inequality; often skipped in treatments. |

---

## Priority 3 — Novel Content Worth Adding from Applebaum

These items are not gaps per se — they are concepts Applebaum frames unusually well
or covers that are absent from the repo entirely.

### Fibonacci / Golden Section as a Limit (Applebaum §4.4)

Derives `φ = limₙ→∞ fₙ₊₁/fₙ` by solving the recurrence `gₙ = gₙ₋₁ + gₙ₋₂`
as a difference equation with trial solution `gₙ = rⁿ`. Full proof. No other book
in the current stack does this with a complete proof chain.

**Add to:** `volume-iii/analysis/sequences` or `volume-iv` as a worked application.

---

### Square Root Algorithm via MCT (Applebaum §5.4)

The sequence defined by `a₁² ≥ p` and `aₙ₊₁ = ½(aₙ + p/aₙ)` converges to `√p`
for any prime `p`. The proof uses:

1. The key inequality `p ≤ ¼(y + p/y)² ≤ y²` (whenever `y² ≥ p`)
2. MCT to establish convergence
3. Algebra of limits to identify the limit as `√p`

This connects MCT directly to a constructive algorithm. The inequality chain is
elegant and worth having in the notes on its own.

**Add to:** `volume-iii/analysis/sequences` as a worked application of MCT.

---

### `lim sup` and `lim inf` via Monotone Sequences (Applebaum §5, Exercise 12)

Define `aₙ = sup{xₘ : m ≥ n}` and `bₙ = inf{xₘ : m ≥ n}`. Then:

- `(aₙ)` is monotone decreasing and bounded below, hence convergent by MCT.
- `(bₙ)` is monotone increasing and bounded above, hence convergent by MCT.
- Define `lim sup xₙ = limₙ aₙ` and `lim inf xₙ = limₙ bₙ`.
- A bounded sequence `(xₙ)` converges to `l` iff `lim sup xₙ = lim inf xₙ = l`.

This is the *right* way to introduce lim sup/inf — as an immediate consequence of
MCT, not as a standalone definition. Add after MCT is settled.

**Add to:** `volume-iii/analysis/sequences` — new subsection after MCT.

---

### Infinite Products (Applebaum Ch. 8)

Not present in the repo at all. Euler's product formula

$$\zeta(r) = \sum_{n=1}^{\infty} \frac{1}{n^r} = \prod_{p \text{ prime}} \left(1 - \frac{1}{p^r}\right)^{-1}$$

is one of the most beautiful results in analysis and connects directly to:
- Number theory (prime number theorem)
- Complex analysis (Riemann hypothesis)
- Analytic number theory

**Add to:** `volume-iii/analysis` as a new chapter, or `volume-iv`. Low priority
for now but flag for when series is settled.

---

### Construction of ℝ via Cauchy Sequences (Applebaum §11.2)

Verify whether this exists in the repo. If not:

- Define rational Cauchy sequences.
- Define equivalence of two rational Cauchy sequences.
- Define `ℝ` as equivalence classes of rational Cauchy sequences.
- Verify arithmetic and order carry through.
- Prove completeness of `ℝ` constructed this way.

Complementary to Dedekind cuts (see top reminder). Both constructions should
ultimately live in `volume-ii` or early `volume-iii`.

---

## BibTeX — Applebaum Reference

```bibtex
@book{applebaum2012limits,
  author    = {Applebaum, David},
  title     = {Limits, Limits Everywhere: The Tools of Mathematical Analysis},
  publisher = {Oxford University Press},
  address   = {Oxford},
  year      = {2012},
  isbn      = {978-0-19-964008-9},
  pages     = {217},
}
```

---

*Last updated: 2026-04-25 — generated from Applebaum audit + volume-iii chapter.yaml cross-reference.*
