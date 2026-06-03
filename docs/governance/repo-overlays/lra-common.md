# lra-common Overlay

Stub overlay for shared LaTeX infrastructure.

Owned concerns:

- `common/`,
- `bibliography/`,
- bibliography helper scripts,
- shared LaTeX macros, environments, boxes, colors, and preambles,
- common-to-volume sync expectations.

## Agent Scope

Edit shared LaTeX infrastructure here, not in volume repo copies. When changing
`common/` or `bibliography/`, expect sync workflows to propagate updates to
volume repos and the monorepo.

Add bibliography entries only in `lra-common`, preferably through the split
volume bibliography files. Mobile photo, screenshot, OCR, and extractor
candidates must be searched and deduplicated before promotion to a canonical
`.bib` file.

Do not edit canonical YAML here; that remains owned by `Learning-Real-Analysis`.
