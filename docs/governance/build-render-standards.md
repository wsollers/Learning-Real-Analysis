# Build And Render Standards

Source sections: `DESIGN.md` section 2.9, repo READMEs, and repo workflows.

## Global Expectation

Use the build or render path owned by the repository being edited. Do not
substitute a specialist repo's validation workflow for a volume or governance
task.

## Volume Repos

Volume repos are Overleaf-ready and build through their local `main.tex` with
synced `common/` and `bibliography/`.

## Monorepo

`Learning-Real-Analysis` owns omnibus integration builds and canonical YAML
sources. Docker and extraction integration live there unless a repo overlay
states otherwise.

## Specialist Repos

Lean validation belongs in the `lra-lean` overlay. C++/Vulkan validation
belongs in the `lra-nurbs` overlay. Numerical benchmark and plotting validation
belongs in the `lra-numerical-analysis` overlay.
