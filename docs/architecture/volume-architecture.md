# Eight-Volume Architecture

This document defines the canonical target volume architecture for Learning
Real Analysis. It is governance for future migration work only. It does not
rename current displayed titles, move content, create repositories, alter
archives, or change build and sync scripts.

## Target Volumes

| Volume | Roman | Repository | Display title | Frontispiece | Lifespan | Image path | Status |
|---:|---|---|---|---|---|---|---|
| 1 | I | `wsollers/lra-volume-i` | `Logic, Sets, and Proof` | Georg Cantor | 1845-1918 | `images/cantor.png` | active |
| 2 | II | `wsollers/lra-volume-ii` | `Origins of the Numbers` | Giuseppe Peano | 1858-1932 | `images/peano.png` | active |
| 3 | III | `wsollers/lra-volume-iii` | `Analysis` | Augustin-Louis Cauchy | 1789-1857 | `images/cauchy.png` | active |
| 4 | IV | `wsollers/lra-volume-iv` | `Algebra and Abstract Structures` | Emmy Noether | 1882-1935 | `images/noether.png` | active |
| 5 | V | `wsollers/lra-volume-v` | `Topology and Geometry` | Henri Poincare | 1854-1912 | `images/poincare.png` | active |
| 6 | VI | `wsollers/lra-volume-vi` | `Computational Mathematics` | Leonhard Euler | 1707-1783 | `images/euler.png` | active |
| 7 | VII | `wsollers/lra-volume-vii` | `Numerical and Approximation Mathematics` | Isaac Newton | 1643-1727 | `images/newton.png` | active |
| 8 | VIII | `wsollers/lra-volume-viii` | `Mathematical Logic and Foundations` | David Hilbert | 1862-1943 | `images/hilbert.png` | active |

## Canonical Volume Metadata Fields

Every volume must have one canonical metadata record. The record may live in a
volume-level YAML file or another approved registry, but it must expose these
fields:

- `volume_number`: Arabic number, such as `1`.
- `roman_numeral`: Roman numeral, such as `I`.
- `repository`: GitHub repository name, such as `wsollers/lra-volume-i`.
- `display_title`: canonical displayed volume title.
- `frontispiece_mathematician`: full mathematician name.
- `mathematician_lifespan`: birth-death years, formatted as `YYYY-YYYY`.
- `image_path`: monorepo-relative image path, formatted as
  `images/<filename>.png`.
- `frontispiece_file`: preferred frontispiece source file, normally
  `volume-*/frontispiece.tex`.
- `status`: one of `active`, `planned`, `stub`, `migrated`, or `archived`.

The metadata record is the authority for title and frontispiece consistency.
Generated title pages, frontispiece pages, volume indexes, and repository
documentation should agree with this record once the relevant migration phase
has run.

## Status Values

- `active`: the volume repository exists and is part of the current build or
  migration workflow.
- `planned`: the target volume is part of the architecture but is not yet a
  fully populated repository.
- `stub`: the repository or volume directory exists with placeholder content.
- `migrated`: content has been moved or split according to the target
  architecture.
- `archived`: historical content retained for reference and not part of the
  active volume route.

Archives must remain untouched unless a later task explicitly authorizes an
archive-only maintenance change.

## Title Consistency Rule

For each volume, these places must eventually use the same displayed volume
title:

- title page;
- frontispiece page;
- volume index;
- `\part` title;
- volume metadata.

Existing mismatches are migration targets, not permission to make opportunistic
edits. Title changes must occur in the title-standardization phase.

## Migration Rule

The migration must proceed by stubs first and small chapter-level moves later.
Do not perform bulk moves across multiple domains in a single commit. Do not
rename repositories, create new repositories, or move content until the
metadata conventions, frontmatter conventions, and workflow update plan are in
place.

When content eventually moves:

- preserve theorem labels;
- preserve proof labels;
- preserve citations;
- preserve cross-references;
- leave archives intact;
- prefer router files or compatibility stubs when they reduce breakage;
- test the source and destination volume builds before pushing.

## Current Integration Points

The monorepo has active directories and standalone roots for Volumes I-VIII.
Local Docker/migration tooling recognizes the eight-volume identifier set.
Split repositories and volume-to-monorepo sync workflows exist for all eight
volume repositories. The monorepo root inputs Volumes I-VIII, and the knowledge
rebuild workflow watches all eight `volume-*` source trees.

## Deferred Phase 0 Findings

The Phase 0 inventory found the following issues. They are intentionally
deferred and must not be fixed as part of this governance phase:

- Volume I title rename from `Sets and Logic` to
  `Logic, Sets, and Proof`;
- Volume II duplicate `peano-quote` input;
- Volume II misspelling `Guiseppe`;
- Volume III mismatch between `Advanced Mathematics` and
  `Abstract Mathematics`;
- Volume III Hilbert frontispiece eventually moving to Volume VIII;
- Volume IV Euler frontispiece eventually moving to Volume VI;
- Volume V numerical content eventually moving to Volume VII;
- untracked monorepo images for Hausdorff, Kolmogorov, Newton, Noether,
  Poincare, and misspelled `reimann`.

## Related Documents

- `docs/architecture/frontmatter-and-frontispiece-standard.md`
- `docs/plans/volume-renumbering-and-frontmatter-migration-plan.md`
