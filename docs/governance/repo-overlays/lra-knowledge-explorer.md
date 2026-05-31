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
