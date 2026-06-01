# Volume Layout

Source: `DESIGN.md` sections 2.2, 2.5, 2.6, and `REPOSITORY_STRUCTURE.md`.

Volume repos are self-contained and Overleaf-ready.

```text
lra-volume-N/
  main.tex
  .latexmkrc
  common/
  bibliography/
  volume-N/
    index.tex
    <chapter>/
      index.tex
      chapter.yaml
      notes/
      proofs/
```

`common/` and `bibliography/` are synced copies from `lra-common`. They are not
owned by volume repos.

Volume repos own volume content only. They do not own Lean formalization,
C++/Vulkan simulation, numerical benchmarking, or global governance.
