# Theorem Explorer Pipeline

Source: `REPOSITORY_STRUCTURE.md`, `Learning-Real-Analysis/.github/workflows/trigger-knowledge-rebuild.yml`, and `lra-knowledge-explorer/README.md`.

## Ownership

`lra-knowledge-explorer` owns the extractor implementation, graph generation,
and explorer UI.

`Learning-Real-Analysis` owns the integrated LaTeX source tree and dispatches
rebuilds after relevant content or pipeline changes land on `main`.

## Dispatch Model

The monorepo is the single dispatch point because leaf repos already sync their
content into it. This avoids duplicated rebuild triggers across every volume
and shared infrastructure repo.

## Extraction Inputs

Extractor runs should point at the local `Learning-Real-Analysis` clone. The
pipeline depends on stable labels, dependency blocks, and canonical chapter
structure.
