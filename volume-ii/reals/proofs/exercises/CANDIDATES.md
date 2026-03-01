# Exercise Candidates — Reals

Propositions and corollaries whose proofs follow routinely from surrounding results.

| Result | Location in notes | What the proof requires |
|--------|-------------------|------------------------|
| Uniqueness of supremum and infimum | `foundations/notes-bounds.tex` | Two-line antisymmetry argument; if $s$ and $s'$ are both suprema then $s \le s'$ and $s' \le s$ |
| $\varepsilon$-Approximation corollary | `foundations/notes-bounds.tex` | Immediate from $\varepsilon$-characterization + upper bound definition; one application |
| Floor Lemma (existence + uniqueness) | `foundations/notes-completeness.tex` | Existence: Archimedean gives nonempty $L$; $L$ bounded above; $\mathbb{Z}$ discrete so $L$ attains max. Uniqueness: if $m, m'$ both satisfy, then $|m - m'| < 1$ and $m - m' \in \mathbb{Z}$ forces equality |
| Existence of square roots | `foundations/notes-completeness.tex` | Define $S = \{t \ge 0 : t^2 \le a\}$, show $s = \sup S$ satisfies $s^2 = a$ by ruling out $s^2 > a$ and $s^2 < a$ via $\varepsilon$-argument; good exercise in using completeness |
| Irrationals dense in $\mathbb{R}$ | `foundations/notes-completeness.tex` | Apply density of $\mathbb{Q}$ to $(a/\sqrt{2}, b/\sqrt{2})$ or use rational between $a - \sqrt{2}$ and $b - \sqrt{2}$; one application |
