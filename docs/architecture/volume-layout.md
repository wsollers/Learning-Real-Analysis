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

The target architecture contains eight volume repositories:

- `wsollers/lra-volume-i`
- `wsollers/lra-volume-ii`
- `wsollers/lra-volume-iii`
- `wsollers/lra-volume-iv`
- `wsollers/lra-volume-v`
- `wsollers/lra-volume-vi`
- `wsollers/lra-volume-vii`
- `wsollers/lra-volume-viii`

As of Phase 3B, the monorepo has standalone roots and stub directories for
Volumes VI-VIII. External split repositories and sync workflows for those
volumes remain deferred.
