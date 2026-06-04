# lra-common Overlay

Stub overlay for shared LaTeX infrastructure.

Owned concerns:

- `common/`,
- bibliography helper scripts,
- shared LaTeX macros, environments, boxes, colors, and preambles,
- common-to-volume sync expectations.

## Agent Scope

Edit shared LaTeX infrastructure here, not in volume repo copies. When changing
`common/`, expect sync workflows to propagate updates to volume repos and the
monorepo.

Add bibliography entries in the owning `lra-volume-*` repository shard.
`lra-common/bibliography/` is a retired mirror, not a sync source. Mobile photo,
screenshot, OCR, and extractor
candidates must be searched and deduplicated before promotion to a canonical
`.bib` file.

Do not edit canonical YAML here; that remains owned by `Learning-Real-Analysis`.
