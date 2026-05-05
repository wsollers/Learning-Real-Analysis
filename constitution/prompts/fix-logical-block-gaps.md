# Fix Prompt: Missing Logical Blocks — Continuity & Differentiation
# Covers: def / thm / lem / prop — gap-filling only, no regeneration

---

## Role

You are a LaTeX editor for a formal mathematics repository.
You fill missing logical blocks in existing note files.
You do not regenerate environments. You do not rewrite existing blocks.
You insert only the blocks that are absent.
Output is raw LaTeX source only. No commentary outside the LaTeX.

---

## Output Encoding

All output must be ASCII raw LaTeX source.
Do not emit Unicode mathematical symbols or Unicode punctuation.
Write every symbol with a LaTeX command: `\forall`, `\exists`, `\in`,
`\land`, `\lor`, `\Rightarrow`, `\Longleftrightarrow`, `\to`,
`\varepsilon`, `\delta`, `\mathbb{R}`, `\le`, `\ge`, `\subseteq`,
`\operatorname{...}`.
Do not write rendered symbols as Unicode characters.

---

## Governing Rules (from DESIGN.md §10 and generate-statement.md)

### Block order (DESIGN.md §10.2)

After every definition, theorem, lemma, proposition, or corollary,
blocks appear in this exact order. Omit only when the trigger is not met.

```
1.  remark*[Standard quantified statement]          — always
2.  remark*[Definition predicate reading]           — if predicate exists (defs only)
    remark*[Predicate reading]                      — theorem-like
3.  remark*[Negated quantified statement]           — when negation illuminates
4.  remark*[Negation predicate reading]             — if step 3 generated
5.  remark*[Failure modes]                          — when applicable
6.  remark*[Failure mode decomposition]             — if step 5 generated
7.  remark*[Contrapositive quantified statement]    — thm/lem/prop/cor, when illuminating
8.  remark*[Contrapositive predicate reading]       — if step 7 generated
9.  remark*[Interpretation]                         — always
10. remark*[Dependencies] or \NoLocalDependencies   — always
```

### Notation discipline (DESIGN.md Rule A)

Predicate names (`\operatorname{...}`) appear **only** in:
- Definition predicate reading
- Negation predicate reading
- Contrapositive predicate reading
- Failure mode decomposition

They must **never** appear in:
- Definition / theorem bodies
- Standard quantified statements
- Negated quantified statements
- Contrapositive quantified statements

### Long display rule (DESIGN.md §3.1.1)

Multi-clause logical statements use `aligned` inside display math.
Break at implications, conjunctions, disjunctions.

### Negation rule (DESIGN.md §10.5)

The negated quantified statement block contains the formal negation only.
Explanatory prose about the negation belongs in the failure-mode block, not here.

### Contrapositive selectivity (DESIGN.md §10.12)

Contrapositive blocks are included only when mathematically illuminating —
not merely because a contrapositive can be formally written.
For implication theorems where the contrapositive is a standard proof tool,
include it. For biconditionals, include the negation of the iff instead.

### Failure mode decomposition (DESIGN.md §10.6)

When failure modes are generated, also generate a decomposition block.
This block may use underbraces or equivalent visual grouping.
Canonical predicates are permitted here.

---

## Task: Fill These Specific Gaps

Process each item below in order.
For each item, output **only** the missing blocks, in canonical order,
ready to paste immediately before the `\begin{remark*}[Interpretation]`
block that already exists (unless a different insertion point is noted).

---

### CONTINUITY CHAPTER

---

#### GAP 1 — `def:continuous-at-point-seq`
**File:** `notes/point-continuity/notes-continuity.tex`
**Missing:** `remark*[Failure modes]` and `remark*[Failure mode decomposition]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block for this definition
**Context:** The negated quantified statement and negation predicate reading
already exist. The definition states:
> $f$ is continuous at $c$ iff for every $(x_n)\subseteq A$,
> $x_n\to c$ implies $f(x_n)\to f(c)$.

Generate:
- `remark*[Failure modes]` — prose: continuity fails when some sequence
  $(x_n)\subseteq A$ with $x_n\to c$ has $f(x_n)\not\to f(c)$; describe
  the two sub-branches (wrong limit vs no limit).
- `remark*[Failure mode decomposition]` — formal underbrace display showing
  the existential witness decomposed into its two conjuncts.

---

#### GAP 2 — `def:sequential-discontinuity-at-a-point`
**File:** `notes/point-continuity/notes-discontinuity.tex`
**Missing:** `remark*[Negation predicate reading]`, `remark*[Failure modes]`,
`remark*[Failure mode decomposition]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** Negated quantified statement already exists and reads:
> $f$ is continuous at $c \Longleftrightarrow \forall (x_n)\subseteq A:\;
> x_n\to c \Rightarrow f(x_n)\to f(c)$.
The definition is of $\operatorname{DiscontinuousAt}(f,c)$.

Generate the three missing blocks. The negation predicate reading should name
$\neg\operatorname{DiscontinuousAt}(f,c) \Longleftrightarrow \operatorname{ContinuousAt}(f,c)$
and give its full quantified form. Failure modes: the sequential witness fails
to exist. Failure mode decomposition: formal display of the negation with
underbrace annotations.

---

#### GAP 3 — `def:neighborhood-discontinuity-at-a-point`
**File:** `notes/point-continuity/notes-discontinuity.tex`
**Missing:** `remark*[Definition predicate reading]`, `remark*[Negated quantified statement]`,
`remark*[Negation predicate reading]`, `remark*[Failure modes]`,
`remark*[Failure mode decomposition]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** Only the standard quantified statement and interpretation exist.
The standard quantified statement reads:
> $f$ is discontinuous at $c \Longleftrightarrow
> \exists V \text{ nbhd of }f(c)\;\forall U \text{ nbhd of }c\;
> \exists x\in U\cap A:\; f(x)\notin V$.

Generate all five missing blocks. The negation (continuity) is the swap of
all three quantifiers: $\forall V\;\exists U\;\forall x\in U\cap A:\; f(x)\in V$.
The failure mode decomposition should display this quantifier reversal
with underbrace labelling of each quantifier's role.

---

#### GAP 4 — `lem:equivalent-discontinuity-at-a-point`
**File:** `notes/point-continuity/notes-discontinuity.tex`
**Missing:** `remark*[Predicate reading]`, `remark*[Failure modes]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** This is a lemma stating that three formulations of discontinuity
are equivalent. The standard quantified statement already shows all three
biconditionals chained. The predicate reading should express the chain
as $\operatorname{DiscontinuousAt}(f,c)$ holding iff each of the three
formulations holds. The failure modes block: the equivalence cannot fail
for functions where $c$ is a limit point of $A$; describe what happens
if $c$ is not a limit point (vacuous cases).

---

#### GAP 5 — `def:types-of-discontinuity-at-a-point`
**File:** `notes/point-continuity/notes-discontinuity.tex`
**Missing:** `remark*[Definition predicate reading]`,
`remark*[Negation predicate reading]`, `remark*[Failure modes]`,
`remark*[Failure mode decomposition]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** The standard quantified statement and negated quantified statement
already exist. The three types are: removable ($\exists L:\;\lim f = L \ne f(x_0)$),
jump ($\exists L_-,L_+$ with $L_-\ne L_+$), essential (not removable).

Generate:
- `remark*[Definition predicate reading]` — define
  $\operatorname{RemovableDiscontinuity}(f,x_0)$,
  $\operatorname{JumpDiscontinuity}(f,x_0)$,
  $\operatorname{EssentialDiscontinuity}(f,x_0)$ using $\coloneqq$.
- `remark*[Negation predicate reading]` — negate each of the three predicates.
- `remark*[Failure modes]` — prose: what must fail for each type not to hold.
- `remark*[Failure mode decomposition]` — formal display of
  $\neg\operatorname{RemovableDiscontinuity}$ and
  $\neg\operatorname{JumpDiscontinuity}$ with underbrace labels.

---

#### GAP 6 — `prop:lipschitz-implies-uc`
**File:** `notes/uniform-continuity/notes-uniform-continuity.tex`
**Missing:** `remark*[Contrapositive quantified statement]`,
`remark*[Contrapositive predicate reading]`, and a
`remark*[Strict implication note]`
**Insert before:** the existing `\begin{remark*}[Dependencies]` block
(i.e., after the existing Interpretation block)
**Context:** This is a one-way implication:
$\operatorname{Lipschitz}(f,A,K) \Rightarrow \operatorname{UniformlyContinuous}(f,A)$.
The contrapositive is mathematically useful because it gives the standard
method for proving a function is not Lipschitz.

Generate:
- `remark*[Contrapositive quantified statement]`:
  $\neg\operatorname{UniformlyContinuous}(f,A) \Rightarrow
  \forall K\ge 0:\;\exists x,y\in A\;(|f(x)-f(y)|>K|x-y|)$.
- `remark*[Contrapositive predicate reading]`: predicate form.
- `remark*[Strict implication note]`: prose-only remark explaining
  the implication is strict (Lipschitz $\Rightarrow$ UC, but not conversely),
  with the canonical counterexample $f(x)=\sqrt{x}$ on $[0,1]$ (UC but not
  Lipschitz at 0). Note the relevance to GPU Lipschitz-constant estimation:
  uniform continuity alone does not supply a computable $K$.

---

### DIFFERENTIATION CHAPTER

---

#### GAP 7 — `def:topological-definition-of-derivative-at-a-point`
**File:** `notes/derivative-definition/notes-derivative-definition.tex`
**Missing:** `remark*[Negated quantified statement]`,
`remark*[Negation predicate reading]`, `remark*[Failure modes]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** Standard quantified statement and predicate reading exist.
The definition reads:
> $f$ is differentiable at $c$ with derivative $L$ iff
> $\forall V \ni L\;\exists U \ni c\;\forall x\in (U\setminus\{c\})\cap D:\;
> \frac{f(x)-f(c)}{x-c}\in V$.

The negation is:
> $\exists V \ni L\;\forall U \ni c\;\exists x\in(U\setminus\{c\})\cap D:\;
> \frac{f(x)-f(c)}{x-c}\notin V$.

The failure modes: the difference quotient escapes some fixed window around
$L$ no matter how tightly the input is restricted. Geometric reading: no
punctured neighbourhood around $c$ maps all secant slopes into a given
neighbourhood of $L$.

---

#### GAP 8 — `def:sequential-definition-of-derivative-at-a-point`
**File:** `notes/derivative-definition/notes-derivative-definition.tex`
**Missing:** `remark*[Negated quantified statement]`,
`remark*[Negation predicate reading]`, `remark*[Failure modes]`,
`remark*[Failure mode decomposition]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** Standard quantified statement and predicate reading exist.
The definition reads:
> $\forall (x_n)\subseteq A\setminus\{c\}:\; x_n\to c \Rightarrow
> \frac{f(x_n)-f(c)}{x_n-c}\to L$.

The negation is the primary tool for disproving differentiability:
> $\exists (x_n)\subseteq A\setminus\{c\}:\; x_n\to c \land
> \frac{f(x_n)-f(c)}{x_n-c}\not\to L$.

Generate all four blocks. The failure mode decomposition should show the
existential witness split into its two conjuncts with underbrace labels.
Include the standard witnesses: two subsequences with different quotient
limits (e.g., $|x|$ at $0$), or quotients growing without bound.

---

#### GAP 9 — `thm:derivative-equivalence`
**File:** `notes/derivative-definition/notes-derivative-definition.tex`
**Missing:** `remark*[Predicate reading]`, `remark*[Negated quantified statement]`,
`remark*[Failure modes]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** The standard quantified statement currently just says
`\text{(i) }\Longleftrightarrow\text{(ii)}\Longleftrightarrow\text{(iii)}`.
It does not spell out the chain in predicate form.

Generate:
- `remark*[Predicate reading]` — express the full chain:
  $\operatorname{DerivativeAt}(f,c,L)
  \Longleftrightarrow \operatorname{DerivativeAt}_{\mathrm{top}}(f,c,L)
  \Longleftrightarrow \operatorname{DerivativeAt}_{\mathrm{seq}}(f,c,L)$.
- `remark*[Negated quantified statement]` — negate the biconditional chain:
  all three predicates fail simultaneously since they are equivalent.
  Reduce to the sequential negation as the computationally useful form.
- `remark*[Failure modes]` — the equivalence cannot produce contradictory
  verdicts; explain the only degenerate case (non-limit-point $c$ making
  the limits vacuous).

---

#### GAP 10 — `prop:derivative-h-form-equivalence`
**File:** `notes/derivative-definition/notes-derivative-definition.tex`
**Missing:** `remark*[Negated quantified statement]`, `remark*[Failure modes]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** The proposition states
$\operatorname{DerivativeAt}(f,c,L) \Longleftrightarrow
\lim_{h\to 0}\frac{f(c+h)-f(c)}{h} = L$.

Generate:
- `remark*[Negated quantified statement]` — the $h$-form limit fails to equal
  $L$: state this precisely, noting that $h\ne 0$ is implicit in the limit.
- `remark*[Failure modes]` — two branches: (i) incremental ratio has no finite
  limit; (ii) ratio converges to a value other than $L$.

---

#### GAP 11 — `thm:differentiable-implies-continuous`
**File:** `notes/derivative-definition/notes-derivative-definition.tex`
**Missing:** `remark*[Predicate reading]`, `remark*[Contrapositive quantified statement]`,
`remark*[Contrapositive predicate reading]`
**Insert before:** the existing `remark*[Failure mode decomposition]` block
(which already contains the $|x|$ counterexample note)
**Context:** The standard quantified statement reads:
$\operatorname{DerivableAt}(f,c) \Rightarrow \operatorname{ContinuousAt}(f,c)$.

Generate:
- `remark*[Predicate reading]` — restate using canonical predicate names.
- `remark*[Contrapositive quantified statement]` —
  $\neg\operatorname{ContinuousAt}(f,c) \Rightarrow \neg\operatorname{DifferentiableAt}(f,c)$.
- `remark*[Contrapositive predicate reading]` — predicate form of the same.

The contrapositive is the standard proof tool here: to show
non-differentiability, show discontinuity.

---

#### GAP 12 — `thm:uniqueness-of-the-derivative`
**File:** `notes/derivative-definition/notes-derivative-definition.tex`
**Missing:** `remark*[Predicate reading]`, `remark*[Contrapositive quantified statement]`,
`remark*[Contrapositive predicate reading]`, `remark*[Failure modes]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** The standard quantified statement reads:
$\operatorname{DerivativeAt}(f,c,L) \land \operatorname{DerivativeAt}(f,c,L')
\Rightarrow L = L'$.

Generate:
- `remark*[Predicate reading]` — same statement in predicate form.
- `remark*[Contrapositive quantified statement]` — $L\ne L' \Rightarrow
  \neg(\operatorname{DerivativeAt}(f,c,L) \land \operatorname{DerivativeAt}(f,c,L'))$.
- `remark*[Contrapositive predicate reading]` — predicate form.
- `remark*[Failure modes]` — include the proof sketch: $\varepsilon = |L-L'|/2$
  argument leading to $|L-L'| < |L-L'|$; note that the cluster point
  condition on $c$ is essential.

---

#### GAP 13 — `thm:rolles-theorem`
**File:** `notes/mean-value-theorem/notes-mean-value-theorem.tex`
**Missing:** `remark*[Predicate reading]`, `remark*[Contrapositive quantified statement]`,
`remark*[Contrapositive predicate reading]`, `remark*[Failure modes]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** The three hypotheses are:
$\operatorname{ContinuousOn}(f,[a,b])$,
$\operatorname{DifferentiableOn}(f,(a,b))$,
$f(a)=f(b)$.
The conclusion: $\exists c\in(a,b):\; f'(c)=0$.

Generate:
- `remark*[Predicate reading]` — express with named predicates.
- `remark*[Contrapositive quantified statement]` —
  $\forall c\in(a,b)\;(f'(c)\ne 0) \Rightarrow
  \neg\operatorname{ContinuousOn}(f,[a,b]) \lor
  \neg\operatorname{DifferentiableOn}(f,(a,b)) \lor f(a)\ne f(b)$.
- `remark*[Contrapositive predicate reading]` — predicate form.
- `remark*[Failure modes]` — one bullet per hypothesis: what happens when
  each is dropped. Use concrete counterexamples:
  (i) discontinuity → $\operatorname{sgn}(x-\tfrac{1}{2})$ on $[0,1]$;
  (ii) non-differentiability → $|x-\tfrac{1}{2}|$ on $[0,1]$;
  (iii) $f(a)\ne f(b)$ → $f(x)=x$ on $[0,1]$.

---

#### GAP 14 — `thm:mean-value-theorem`
**File:** `notes/mean-value-theorem/notes-mean-value-theorem.tex`
**Missing:** `remark*[Predicate reading]`, `remark*[Contrapositive quantified statement]`,
`remark*[Contrapositive predicate reading]`, `remark*[Failure modes]`
**Insert before:** the existing `\begin{remark*}[Interpretation]` block
**Context:** Two hypotheses: $\operatorname{ContinuousOn}(f,[a,b])$ and
$\operatorname{DifferentiableOn}(f,(a,b))$.
Conclusion: $\exists c\in(a,b):\; f'(c)=\frac{f(b)-f(a)}{b-a}$.

Generate:
- `remark*[Predicate reading]` — predicate form.
- `remark*[Contrapositive quantified statement]` —
  $\forall c\in(a,b)\;\bigl(f'(c)\ne\frac{f(b)-f(a)}{b-a}\bigr)
  \Rightarrow \neg\operatorname{ContinuousOn}(f,[a,b]) \lor
  \neg\operatorname{DifferentiableOn}(f,(a,b))$.
- `remark*[Contrapositive predicate reading]` — predicate form.
- `remark*[Failure modes]` — one bullet per hypothesis with counterexamples.
  Also note: the MVT is an existence result; it does not locate $c$.

---

## Output Format

For each gap, output the LaTeX blocks in this structure:

```
% ── GAP N: label ─────────────────────────────────────────────────────────────

\begin{remark*}[Block title]
...
\end{remark*}

\begin{remark*}[Next block title]
...
\end{remark*}
```

One gap per section. Do not output the surrounding context, the definition
body, or the existing blocks. Output only the new blocks to be inserted.

After all gaps are output, output nothing further.

---

## Pre-Output Checks (silent)

Before outputting each block, verify:

1. No `\operatorname{...}` predicate name appears in any standard, negated,
   or contrapositive quantified statement. If found, remove it.
2. Multi-clause displays use `aligned`. If not, add it.
3. Each block header matches exactly one of the canonical titles listed in
   the block order above.
4. Every variable in a logical block is either quantified or fixed by the
   immediately preceding statement.
5. The negation is formed by pushing `\neg` inward through quantifiers
   (dual: $\forall\leftrightarrow\exists$, $\land\leftrightarrow\lor$,
   $\Rightarrow$ negated as $P\land\neg Q$).
6. Contrapositives are hypothesis-conclusion swap with both negated.
7. The failure mode decomposition is a formal display (underbrace preferred),
   not a prose restatement of the failure modes block.

---

## Notes on Specific Items

**GAP 3 (neighbourhood discontinuity):** The quantifier reversal
$\exists V\;\forall U\;\exists x \mapsto \forall V\;\exists U\;\forall x$
is the central mathematical fact. The decomposition should make the
reversal visually explicit.

**GAP 6 (Lipschitz → UC):** The strict implication note is important for
the downstream GPU/Vulkan Lipschitz-constant work: uniform continuity of a
simulation function does not supply a computable Lipschitz bound for GPU
optimization. Only an explicit Lipschitz condition does.

**GAP 8 (sequential derivative negation):** This is the primary tool for
proving non-differentiability at corners and cusps. The failure mode
decomposition should include the two canonical witness types: (i) two
subsequences with different limiting difference quotients; (ii) quotients
diverging to infinity.

**GAP 12 (uniqueness):** The proof sketch in the failure modes block
($\varepsilon = |L-L'|/2$ argument) is the standard proof technique and
belongs in the notes layer, not only in the proof file.

**GAPS 13 and 14 (Rolle and MVT):** The hypothesis-failure analysis with
explicit counterexamples is the content that makes these theorems useful
for building intuition. Do not omit the counterexamples.
