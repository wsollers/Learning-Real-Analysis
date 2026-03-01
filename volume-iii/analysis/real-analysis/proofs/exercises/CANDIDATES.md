# Exercise Candidates — Real Analysis

Propositions and corollaries whose proofs follow routinely from surrounding results.

| Result | Location in notes | What the proof requires |
|--------|-------------------|------------------------|
| Uniqueness of limits | `sequences/notes-convergence.tex` | Assume two limits $L, L'$; apply $\varepsilon/2$ argument; conclude $|L - L'| < \varepsilon$ for all $\varepsilon > 0$ |
| Convergent $\Rightarrow$ Cauchy | `sequences/notes-cauchy.tex` | Triangle inequality with $\varepsilon/2$; straightforward once convergence definition is known |
| Cauchy sequences are bounded | `sequences/notes-cauchy.tex` | Same head-tail decomposition as convergent-implies-bounded; apply with $\varepsilon = 1$ |
| Subsequences inherit limits | `sequences/notes-subsequences.tex` | If $x_n \to L$ and $(x_{n_k})$ is a subsequence, apply definition: $n_k \ge k$ (Index Growth) gives $n_k \ge N$ eventually |
| Convergence via even and odd subsequences | `sequences/notes-subsequences.tex` | If $(x_{2k})$ and $(x_{2k-1})$ both converge to $L$, show $(x_n) \to L$ by taking $N = \max(N_{\text{even}}, N_{\text{odd}})$ |
| $\varepsilon$-Approximation corollary (sequences) | `sequences/notes-sequence-bounds.tex` | Immediate from definition of supremum applied to range of sequence |
| Algebra of limits (sum, product, quotient) | `sequences/notes-algebra-of-sequences.tex` | Standard $\varepsilon$-$N$ arguments for each; sum is most instructive; product requires boundedness lemma |
| Existence of subsequential limits | `sequences/notes-subsequences.tex` | Corollary of BW: every bounded sequence has at least one subsequential limit |
| Sequential compactness of $[a,b]$ | `sequences/notes-subsequences.tex` | Any sequence in $[a,b]$ is bounded, hence has convergent subsequence by BW; limit lies in $[a,b]$ by closure |
| Direct Comparison Test | `series/notes-series-tests.tex` | Compare partial sums; boundedness + monotone convergence |
| Ratio and Root Tests | `series/notes-series-tests.tex` | Compare to geometric series; requires algebra of limits |
| Alternating Series Test | `series/notes-series-tests.tex` | Show partial sums form a Cauchy sequence using decreasing terms going to 0 |
