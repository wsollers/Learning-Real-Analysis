# Frontmatter and Frontispiece Standard

This document governs volume frontmatter and mathematician frontispieces. It
is documentation/governance only. It does not authorize changing current TeX
frontmatter, generating images, moving content, or modifying archives.

## Required Frontmatter Consistency

Each volume must eventually keep these elements in sync with the canonical
volume metadata record:

- title page;
- frontispiece page;
- volume index;
- `\part` title;
- volume metadata.

The displayed volume title must be identical in all five places once the title
standardization phase has run.

## Standard Frontispiece File Name

The preferred frontispiece file name is:

```text
volume-*/frontispiece.tex
```

Existing files such as `cantor-quote.tex`, `peano-quote.tex`,
`hilbert-quote.tex`, `euler-quote.tex`, and `gauss-quote.tex` may remain
temporarily as router files until later migration phases replace or retire
them.

Do not rename current frontispiece or quote files during governance-only
phases.

## Image Path Rule

All LaTeX image paths for volume frontispieces must be monorepo-relative.

Allowed:

```latex
\includegraphics[width=0.6\textwidth]{images/<filename>.png}
```

Forbidden:

```text
volume-i/images/...
volume-ii/images/...
volume-*/images/...
```

The path rule applies to both split volume repositories and the monorepo. Even
inside a volume repository, the LaTeX source must reference
`images/<filename>.png`.

## House Image Style

Each volume frontispiece should eventually use the standard mathematician
portrait style:

- monochrome black-and-white engraved line art;
- centered circular medallion portrait;
- bold dark circular border;
- relevant formulas or diagrams lightly arranged inside the circle;
- rectangular engraved nameplate below the medallion;
- short stem connecting the medallion to the nameplate;
- plaque text containing exactly the mathematician's full name and lifespan.

The plaque must not include titles, volume numbers, quotes, nicknames, or
extra commentary.

## Target Frontispieces

| Volume | Display title | Mathematician | Lifespan | Image path |
|---|---|---|---|---|
| Volume I | `Logic, Sets, and Proof` | Georg Cantor | 1845-1918 | `images/cantor.png` |
| Volume II | `Origins of the Numbers` | Giuseppe Peano | 1858-1932 | `images/peano.png` |
| Volume III | `Analysis` | Augustin-Louis Cauchy | 1789-1857 | `images/cauchy.png` |
| Volume IV | `Algebra and Abstract Structures` | Emmy Noether | 1882-1935 | `images/noether.png` |
| Volume V | `Topology and Geometry` | Henri Poincare | 1854-1912 | `images/poincare.png` |
| Volume VI | `Computational Mathematics` | Leonhard Euler | 1707-1783 | `images/euler.png` |
| Volume VII | `Numerical and Approximation Mathematics` | Isaac Newton | 1643-1727 | `images/newton.png` |
| Volume VIII | `Mathematical Logic and Foundations` | David Hilbert | 1862-1943 | `images/hilbert.png` |

## Generation Boundary

If a required image does not exist, a later image-generation phase may create
it. That phase must:

- use the house image style above;
- save the file as `images/<lastname>.png`;
- reference the image as `images/<lastname>.png`;
- avoid volume-local image directories.

Governance-only phases must not generate, replace, delete, resize, or
re-encode images.

## Migration Discipline

Frontmatter changes must be small and auditable:

- first standardize metadata conventions;
- then standardize displayed titles;
- then introduce `frontispiece.tex` routers or files;
- then attach or update image assets;
- then remove obsolete router files only when no inputs reference them.

Archive material must remain untouched. Existing historical frontmatter inside
archives should not be normalized unless a later archive-specific task
explicitly requires it.

## Deferred Phase 0 Findings

The following known issues are deferred and must not be fixed in this
governance phase:

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
