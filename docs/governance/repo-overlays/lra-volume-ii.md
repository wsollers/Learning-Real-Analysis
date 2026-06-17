# lra-volume-ii Overlay

Volume II uses the generic `lra-volume.md` overlay plus the additional
verification-link contract below.

Owned concerns:

- Volume II content and Overleaf-ready source layout,
- stable theorem, definition, axiom, lemma, proposition, and corollary labels,
- cross-repository verification metadata that points to `lra-lean`,
- volume-to-monorepo content sync.

## Agent Scope

Follow the generic volume overlay. Do not place formal proof implementation
work in `lra-volume-ii`; formal proof implementation is owned by `lra-lean`.

## Verification Links

Every theorem-like or definition-like artifact in Volume II should have a
stable LaTeX label that can be mapped to a formal verification target. When the
target is not available yet, record the status as pending rather than omitting
the relationship.

Volume II may carry lightweight verification metadata such as:

- the LaTeX label,
- the verification status,
- the target formalization repository,
- the target module and declaration name when known.

Do not inline formal proof code as ordinary volume prose. The volume points to
the formalization; `lra-lean` owns the checked formal source, and
`lra-knowledge-explorer` owns the UI surface that displays mapped verification
code.

Volume II may include rare, source-anchored `formalizationrecord` boxes for the
Landau/Feferman number-system spine when all of the following hold:

- the written theorem or definition is part of the number-system foundation,
- the source anchor is explicit, such as `Landau, Foundations of Analysis,
  Proposition X`,
- the box names the reconstructed LaTeX theorem via `\Formalizes{...}`,
- the box names the Lean declaration via `\LeanDeclaration{...}`,
- the code shown is a verification record, not a second theorem statement.

Use this pattern for archival bridge points only:

```tex
\begin{formalizationrecord}{Lean 4 Verification Record}
\FormalizationSource{Landau, \emph{Foundations of Analysis}, Proposition X}
\Formalizes{thm:addition-with-one}
\LeanDeclaration{addition\_with\_one}

\begin{leancode}
...
\end{leancode}
\end{formalizationrecord}
```

When the intended Lean declaration is not available or not complete, include
`\Unverified` in the record source. The command renders nothing and exists so
validation and reporting tools can count pending verification records without
making the PDF noisier.

Use status wording that does not overclaim:

- `pending` when no target has been written,
- `statement` when a formal statement exists but the proof is incomplete,
- `checked` only when the target declaration is accepted by the formal build
  without placeholders for that declaration.
