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

Do not inline formal proof code in the volume source. The volume points to the
formalization; `lra-lean` owns the checked formal source, and
`lra-knowledge-explorer` owns the UI surface that displays the mapped
verification code.

Use status wording that does not overclaim:

- `pending` when no target has been written,
- `statement` when a formal statement exists but the proof is incomplete,
- `checked` only when the target declaration is accepted by the formal build
  without placeholders for that declaration.
