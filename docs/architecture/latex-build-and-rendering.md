# LaTeX Build And Rendering Architecture

LaTeX build behavior follows repository ownership. Agents should use the build
path for the repository being edited and avoid importing specialist validation
workflows into unrelated repos.

## Monorepo

`Learning-Real-Analysis` owns omnibus integration builds, Docker build
infrastructure, cross-volume assembly, canonical YAML, and downstream rebuild
dispatch.

Docker build details remain in the monorepo's Docker documentation. Governance
docs record ownership and safety boundaries rather than duplicating every build
command.

## Volume Repos

Each `lra-volume-*` repo is Overleaf-ready. Its local `main.tex` uses synced
`common/` copies and inputs only that volume's content.

Volume repos own volume content only. They do not own shared LaTeX
infrastructure, canonical YAML, Lean formalization, NURBS/Vulkan simulation,
numerical-analysis benchmark workflows, or PDF extraction tooling.

## Shared LaTeX Infrastructure

`lra-common` owns shared LaTeX infrastructure:

- `common/`,
- `bibliography/`,
- macros,
- environments,
- boxes,
- colors,
- preambles.

Changes to shared LaTeX infrastructure must be made in `lra-common`, then
propagated through the approved sync path.

## Figures

Dependency figures live in dedicated `figure-<n>.tex` files and are input by
notes files. Every nontrivial TikZ figure shall live in a dedicated figure
source file. Figure source files shall contain TikZ code only: no document
preamble, no figure environment, no caption, no label, no surrounding prose,
and no inline color-system redefinition.

Figure colors, boxes, and legend macros come from shared infrastructure rather
than local ad hoc definitions.

## Specialist Validation

Lean validation belongs to `lra-lean`; C++/Vulkan/geometry validation belongs
to `lra-nurbs`; numerical benchmark and plotting validation belongs to
`lra-numerical-analysis`. These workflows must not be applied as volume-content
rules.
