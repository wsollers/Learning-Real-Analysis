# Math Ontology Migration Guide

This guide explains how to migrate from the current predicate/notation/relations setup to the new
three-layer ontology:

- `terms.yaml`
- `predicates.yaml`
- `operators.yaml`

while continuing to generate compatible:

- `knowledge.json`
- `graph-edges.json`

The migration is **deterministic**. The ontology files become the source of truth, and the explorer
files become generated artifacts.

---

## 1. Target architecture

### Source files
- `ontology.yaml`
- `terms.yaml`
- `predicates.yaml`
- `operators.yaml`
- `relations.yaml`
- `notation.yaml`
- `migration.yaml`

### Existing content sources
- TeX source files
- `chapter.yaml`

### Generated outputs
- `knowledge.json`
- `graph-edges.json`

---

## 2. Core design principle

Use these distinctions strictly:

### Terms
Objects or constructors:
- intervals
- balls
- partitions
- upper/lower sums

### Predicates
Truth-valued assertions:
- `Supremum(s,A)`
- `ContinuousAt(f,c)`
- `InteriorPoint(x,A)`

### Operators
Higher-order scope/asymptotic operators:
- `Eventually(P)`
- `Frequently(P)`
- `LocallyAt(P,c)`
- `UniformlyOn(P,A)`

Do **not** encode terms as predicates or operators as predicates.

---

## 3. Migration sequence

## Step 1 - Add the new ontology files

Commit the new YAML files first without touching the notes. This gives the extractor a complete
canonical registry before any rewrite happens.

Recommended commit:
- add `ontology.yaml`
- add `terms.yaml`
- add `predicates.yaml`
- add `operators.yaml`
- add `relations.yaml`
- add `notation.yaml`
- add `migration.yaml`

---

## Step 2 - Update extractor loading order

Change the extractor so it loads files in this order:

1. `ontology.yaml`
2. `notation.yaml`
3. `terms.yaml`
4. `predicates.yaml`
5. `operators.yaml`
6. `relations.yaml`
7. `migration.yaml`
8. `chapter.yaml`
9. TeX files

The extractor should then build a canonical registry keyed by ontology ids such as:

- `pred:supremum`
- `term:interval-closed`
- `op:eventually`

---

## Step 3 - Add normalization layer

Before any theorem extraction, normalize all encountered names.

For every symbol found in TeX or metadata:

1. Look it up in canonical registries
2. If missing, consult `migration.yaml`
3. Rewrite it to its canonical ontology id
4. Record whether it is a term, predicate, or operator

This is the key step that prevents breakage.

---

## 4. TeX migration

## 4.1 What should stay the same

Statements such as

```tex
\operatorname{Supremum}(s,A)
```

can stay conceptually the same. The extractor maps them through canonical ids.

## 4.2 What must change

### A. Constructors currently written as predicates
Examples:
- `\operatorname{ClosedInterval}(a,b)`
- `\operatorname{OpenBall}(x,r)`

These must be treated as **terms**, not predicates.

### B. Higher-order scope or tail words currently written like predicates
Examples:
- `\operatorname{Eventually}(...)`
- `\operatorname{Frequently}(...)`

These must be treated as **operators**.

### C. Renamed canonical predicates
Examples:
- `\operatorname{UniformlyContinuous}` → `\operatorname{UniformlyContinuousOn}`
- `\operatorname{DenseSubset}` → `\operatorname{DenseIn}`

---

### D. Intentional proof TODO stubs

Proof files containing intentional `TODO` stubs are navigational work items, not failed proof
content. They should remain linked in `chapter.yaml` and proof indexes, but the knowledge extractor
must not convert their proof bodies into proof sketches, proof steps, or proof dependency knowledge.

The proof auditor may report them as `stub: true`; this is expected and should not count as a
validation failure.

### E. Semantic change vocabulary

For change-control concepts such as Lipschitz continuity and uniform continuity, prefer terms that
denote quantities:

- `InputSeparation(x,u)` for `|x-u|`
- `OutputVariation(f,x,u)` for `|f(x)-f(u)|`
- `TargetVariation(f,x,L)` for `|f(x)-L|`

Then predicates can read fluently. For example:

```tex
\operatorname{LipschitzOn}(f,A,K)
\Longleftrightarrow
K\ge 0
\land
\forall x,u\in A\;
\operatorname{OutputVariation}(f,x,u)
\le
K\operatorname{InputSeparation}(x,u).
```

Slogan: Lipschitz continuity means output variation is bounded by a fixed scaling of input
separation.

## 4.3 Recommended TeX rewrite policy

### Policy
Do **not** attempt a fully semantic rewrite in one pass.
Instead do three deterministic passes:

#### Pass 1 - rename-only pass
Safe string-level replacements from `migration.yaml`.

#### Pass 2 - kind annotation pass
Mark each recognized name as term/predicate/operator in the extractor.

#### Pass 3 - optional cleanup pass
Later, rewrite human-facing TeX for style consistency.

This keeps the first migration safe.

---

## 4.4 Examples

### Example 1 - continuity name
Before:
```tex
\operatorname{UniformlyContinuous}(f,A)
```

After:
```tex
\operatorname{UniformlyContinuousOn}(f,A)
```

### Example 2 - interval constructor
Before:
```tex
\operatorname{ClosedInterval}(a,b)
```

After in ontology interpretation:
- recognized as `term:interval-closed`

Human-facing TeX may remain unchanged initially.

### Example 3 - asymptotic operator
Before:
```tex
\operatorname{Eventually}(P)
```

After in ontology interpretation:
- recognized as `op:eventually`

---

## 5. chapter.yaml migration

`chapter.yaml` should remain mostly chapter-structural metadata, not ontology source.

### Keep in `chapter.yaml`
- chapter id
- title
- volume
- section ordering
- notes/proofs paths
- deck/category hints
- chapter-level dependencies

### Do not duplicate there
- predicate definitions
- term constructors
- operator definitions

### Recommended additions to `chapter.yaml`
Add a small canonical block like this:

```yaml
ontology:
  profile: lra-math-ontology-v3
  generated_outputs:
    - knowledge.json
    - graph-edges.json
  preserve_legacy_ids: true
```

This lets every chapter declare which ontology profile it expects.

---

## 6. knowledge explorer migration

Your current explorer format is simpler than the ontology source and should remain generated.

## 6.1 knowledge.json generation rules

For each extracted note/theorem/definition:

- keep existing theorem-like node ids such as:
  - `def:supremum`
  - `prop:maximum-implies-supremum`
  - `thm:limit-sum`

- add canonical ontology references in generated metadata, for example:
```json
"ontology_refs": ["pred:supremum"]
```

### Recommended generated fields
- `ontology_refs`
- `canonical_depends_on`
- `legacy_depends_on`
- `node_kind`
- `source_registry_kind` (`predicate`, `term`, `operator`, or `mixed`)

This lets the explorer stay backward-compatible while the extractor becomes smarter.

## 6.2 graph-edges.json generation rules

Internally, the ontology may have:
- `defines`
- `characterizes`
- `implies`
- `specializes`
- `depends_on`
- `rendered_as`

But for explorer compatibility, generate:

```json
{ "from": "...", "to": "...", "kind": "depends_on" }
```

for all load-bearing edges.

If desired, you can later add:
```json
"semantic_kind": "defines"
```
while keeping `"kind": "depends_on"` for the viewer.

---

## 7. Legacy ID preservation

Preserve existing explorer-facing ids whenever possible.

### Recommended rule
- ontology ids are semantic
- explorer ids are compatibility ids

Examples:

- ontology: `pred:supremum`
- explorer node id: `def:supremum`

- ontology: `pred:function-limit`
- explorer node id: `def:limit-function`

Use `migration.yaml` to bridge these deterministically.

This is essential because your current knowledge explorer data already references legacy ids in
dependency lists.

---

## 8. Extractor implementation checklist

## 8.1 Registry stage
- load ontology files
- build canonical registry
- build alias table
- build id translation table

## 8.2 Parsing stage
- scan TeX theorem/definition blocks
- extract operatorname usages
- classify each usage as term/predicate/operator
- resolve to canonical ontology ids

## 8.3 Validation stage
Emit warnings for:
- unknown names
- a term used where a predicate is expected
- an operator used where a term is expected
- legacy alias still present after rewrite
- duplicate semantic concepts with different names

## 8.4 Generation stage
- emit `knowledge.json`
- emit `graph-edges.json`
- emit optional migration audit report

---

## 9. Recommended migration audit report

Generate a Markdown or JSON report listing:

### Unknown names
Names found in TeX that do not resolve anywhere.

### Legacy names
Names resolved through alias tables rather than canonical names.

### Kind mismatches
Examples:
- `ClosedInterval` used in predicate position
- `Eventually` used as ordinary predicate instead of operator

### ID conflicts
Cases where more than one ontology item maps to one legacy id.

---

## 10. Practical rollout plan

### Phase A
Add ontology files and extractor support only.

### Phase B
Run extractor in compatibility mode:
- generate existing explorer artifacts
- do not rewrite TeX yet
- collect audit warnings

### Phase C
Apply deterministic TeX rename pass from `migration.yaml`.

### Phase D
Rebuild explorer artifacts and compare:
- node counts
- edge counts
- theorem ids
- dependency ids

### Phase E
Once stable, do optional human-facing TeX cleanup.

---

## 11. What to migrate first

Do these concepts first because they are the most structurally important:

1. intervals → terms
2. balls / neighborhoods → terms or region predicates
3. Eventually / Frequently → operators
4. continuity naming normalization
5. density naming normalization
6. partition / tagged partition / sums → terms

These are the highest leverage changes for future integration and measure theory.

---

## 12. Semantic sentence targets

The ontology should eventually help the source say what a definition means, not merely restate its
symbols. These sentences are target examples for future migration passes. They are not all required
in the current TeX.

### Lipschitz continuity

Target sentence:

> Lipschitz continuity means output variation is bounded by a fixed scaling of input separation.

Suggested vocabulary:

- `OutputVariation(f,x,u)` for `|f(x)-f(u)|`
- `InputSeparation(x,u)` for `|x-u|`
- `LipschitzOn(f,A [, K])` for the predicate, with `K >= 0` explicit when `K` is supplied

Sample predicate reading:

```tex
\[
\operatorname{LipschitzOn}(f,A,K)
\Longleftrightarrow
K \ge 0
\land
\forall x,u\in A\;
\operatorname{OutputVariation}(f,x,u)
\le
K\operatorname{InputSeparation}(x,u).
\]
```

### Uniform continuity

Target sentence:

> Uniform continuity means every requested output tolerance is guaranteed by a single input-separation
> tolerance across the whole set.

Suggested vocabulary:

- `OutputVariation(f,x,u)`
- `InputSeparation(x,u)`
- `UniformlyContinuousOn(f,A)`

### Differentiability, Zorich style

Target sentence:

> Differentiability means that the output change is a linear response to the input change, up to an
> approximation error that is negligible compared with the input change.

Equivalent teaching sentence:

> In one variable, the linear response is a proportional response. In several variables, the
> proportionality constant is replaced by a linear map.

Suggested vocabulary:

- `InputChange(h)` for the input displacement
- `OutputChange(f,a,h)` for `f(a+h)-f(a)`
- `LinearResponse(L,h)` for `L(h)`
- `ApproximationError(f,a,L,h)` for `OutputChange(f,a,h)-LinearResponse(L,h)`
- `NegligibleErrorComparedToInputChange(f,a,L)` for the little-o condition
- `LinearMap(L)` for the Zorich/general derivative object
- `DifferentiableAt(f,a)` for the predicate

General predicate reading:

```tex
\[
\operatorname{DifferentiableAt}(f,a)
\Longleftrightarrow
\exists L\;
\Bigl(
  \operatorname{LinearMap}(L)
  \land
  \operatorname{NegligibleErrorComparedToInputChange}(f,a,L)
\Bigr).
\]
```

Smallness condition:

```tex
\[
\operatorname{NegligibleErrorComparedToInputChange}(f,a,L)
\Longleftrightarrow
\frac{
  \left\|\operatorname{ApproximationError}(f,a,L,h)\right\|
}{
  \left\|\operatorname{InputChange}(h)\right\|
}
\to 0
\quad\text{as }h\to 0.
\]
```

One-variable specialization:

```tex
\[
\operatorname{LinearResponse}(\lambda,h)=\lambda h.
\]
```

Use this material when the Zorich derivative chapter is ready; do not force the current derivative
files to adopt it prematurely.

---

## 13. Integration-specific note

The new structure is designed so that integration extends naturally.

### Terms
- partitions
- tagged partitions
- upper sums
- lower sums
- gauges

### Predicates
- `RiemannIntegrableOn(f,I)`
- `DeltaFine(P,\delta [, I])`
- `RefinementOf(Q,P)`

This avoids having integration concepts stuffed awkwardly into predicate-only files.

---

## 14. Final recommendation

Treat:
- ontology YAMLs as source-of-truth
- TeX as source mathematical exposition
- `chapter.yaml` as chapter metadata
- explorer files as generated compatibility artifacts

That gives you:
- deterministic migration
- stable legacy ids
- room to grow into integration, measure theory, and later functional analysis

without painting yourself into a corner.
