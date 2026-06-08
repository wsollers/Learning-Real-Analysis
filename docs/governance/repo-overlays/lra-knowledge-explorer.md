# lra-knowledge-explorer Overlay

Stub overlay for theorem explorer and extraction pipeline work.

Owned concerns:

- extraction pipeline implementation,
- knowledge graph and edge generation,
- explorer UI,
- rebuild dispatch expectations.

## Agent Scope

Extraction implementation and UI changes belong here. Monorepo changes may
trigger rebuild dispatch, but extractor code ownership remains with
`lra-knowledge-explorer`.

Do not duplicate canonical YAML ownership here.

## Formal Verification Surface

When explorer records include formal verification metadata, the UI should show
it as a first-class proof companion rather than as ordinary prose. The proof
modal should include a `Verification` tab that displays:

- the verification system,
- status,
- module and declaration when known,
- source path when known,
- well-formatted formal code when available.

The UI must not present pending or incomplete targets as checked. Missing code
or missing metadata should render as an explicit empty state, not as a broken
panel.
