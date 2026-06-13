# Volume II Dependency Remark Migration

Date: 2026-06-12

## Scope

This was a format-only cleanup pass for `lra-volume-ii`.

Converted only legacy dependency blocks of this shape:

```latex
\begin{remark*}[Dependencies]
... \hyperref[...] ...
\end{remark*}
```

to:

```latex
\begin{dependencies}
... \hyperref[...] ...
\end{dependencies}
```

Prose-only dependency remarks were intentionally left untouched because they
require label resolution before they can become graph edges.

## Result

- Converted parseable legacy dependency remarks: 69.
- Left prose-only dependency remarks unchanged: 10.
- Extracted dependency edge count remained stable: 481.
- Volume II dependency extraction issues dropped from 186 to 117.
- Volume II dependency validation warnings dropped from 255 to 186.
- Validation errors remained 161.

Remaining `remark*[Dependencies]` blocks are prose-only and need semantic
resolution before canonicalization.

## Verification

Commands run:

```powershell
python tools/migration/migrate_dependency_remarks.py --root F:\repos\lra-volume-ii\volume-ii --write --json build/dependency-graph/dependency-remark-migration.json
python tools/governance/dependency_graph.py edges --root F:\repos\lra-volume-ii --universe build/dependency-graph/formal-universe.json --out build/dependency-graph/volume-ii-edges.json --markdown build/dependency-graph/volume-ii-edges.md
python tools/governance/dependency_graph.py validate --universe build/dependency-graph/formal-universe.json --edges build/dependency-graph/volume-ii-edges.json --policy docs/governance/dependency-root-policy.yaml --out build/dependency-graph/volume-ii-validation.json --markdown build/dependency-graph/volume-ii-validation.md
```

The broad Volume II validation-only `build-repo` gate was also run. It failed on
existing repository-wide layout, note-block, and decoration issues. This pass
did not attempt to make Volume II globally validator-clean.

## Next Cleanup Queue

1. Fix the remaining missing dependency targets.
2. Resolve the ambiguous global dependency targets.
3. Add missing dependency declarations.
4. Populate primitive roots and documented external assumptions.
5. Promote the identified missing theorem targets into formal statements.

## Semantic Resolution Pass

Follow-up pass on 2026-06-12 resolved the 10 prose-only dependency remarks.

Resolved to explicit dependency edges:

- `thm:rational-plus-irrational-is-irrational` -> `thm:rational-fraction-subtraction`.
- `thm:rational-minus-irrational-is-irrational` -> `thm:rational-fraction-subtraction`.
- `thm:nonzero-rational-times-irrational-is-irrational` -> `thm:rational-fraction-reciprocal`.
- `thm:quotient-by-nonzero-rational-preserves-irrationality` -> `thm:rational-fraction-reciprocal`.
- `cor:irrationals-not-closed-under-multiplication` -> `thm:sqrt2-irrational`.
- `thm:q-dense-in-r` -> `thm:archimedean-property`, `def:rational-numbers`, with a missing floor/integer-part lemma recorded.
- `thm:irrationals-uncountable` -> `thm:R-uncountable`, `thm:Q-countable`, with a missing uncountable-minus-countable lemma recorded.
- `thm:irrationals-not-complete-as-ordered-set` -> `def:least-upper-bound-property`, `def:supremum`, `def:rational-numbers`, with a missing inherited-suborder completeness-failure lemma recorded.
- `thm:real-upper-lower-perturbation` -> `prop:order-arithmetic`, with a missing real ordered-field division-by-2 closure lemma recorded.

Left as an explicit gap without graph edges:

- `thm:cantors-intersection-theorem` in `volume-ii/reals/notes/foundations/name.tex`.
  The cited compactness facts are metric-space facts, but the current universe
  only exposes nearby real-line compactness material. Linking those would be
  semantically wrong.

Post-pass graph results:

- Extracted dependency edges increased from 481 to 494.
- Volume II dependency extraction issues dropped from 117 to 98.
- Volume II dependency validation errors dropped from 161 to 153.
- Volume II dependency validation warnings dropped from 186 to 176.
- No `remark*[Dependencies]` blocks remain under `volume-ii/reals`.

Scope adjustment:

- Excluded the Mendelson and Tao integer construction notes from this dependency
  cleanup effort. They remain in the source tree, but their dependency graph
  cleanup is deferred to a separate construction-focused pass.
- With those files excluded by policy, validation errors dropped from 153 to
  127.
- `missing_dependency_declaration` dropped from 87 to 61.
