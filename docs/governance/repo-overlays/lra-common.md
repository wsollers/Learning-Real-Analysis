# lra-common Overlay

Stub overlay for shared LaTeX infrastructure.

Owned concerns:

- `common/`,
- `bibliography/`,
- shared LaTeX macros, environments, boxes, colors, and preambles,
- common-to-volume sync expectations.

## Agent Scope

Edit shared LaTeX infrastructure here, not in volume repo copies. When changing
`common/` or `bibliography/`, expect sync workflows to propagate updates to
volume repos and the monorepo.

Do not edit canonical YAML here; that remains owned by `Learning-Real-Analysis`.
