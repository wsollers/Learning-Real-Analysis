# Volume Renumbering and Frontmatter Migration Plan

Status: Phase 0 inventory and planning only.

This report inventories the current Learning Real Analysis volume repositories
and proposes a safe migration path toward an eight-volume architecture. No
content moves, repository renames, image edits, theorem-label changes,
proof-label changes, citation changes, or cross-reference changes are included
in this phase.

Phase 1 governance documents:

- `docs/architecture/volume-architecture.md`
- `docs/architecture/frontmatter-and-frontispiece-standard.md`

## Current Repository Inventory

### Existing Volume Repositories

The active split volume repositories are:

| Repository | Branch inspected | Current volume directory | Clean status |
|---|---:|---|---|
| `wsollers/lra-volume-i` | `main` | `volume-i/` | clean |
| `wsollers/lra-volume-ii` | `main` | `volume-ii/` | clean |
| `wsollers/lra-volume-iii` | `main` | `volume-iii/` | clean |
| `wsollers/lra-volume-iv` | `main` | `volume-iv/` | clean |
| `wsollers/lra-volume-v` | `main` | `volume-v/` | clean |

Each volume repository is Overleaf-ready and contains:

- `.github/` with generated instructions and `sync-to-monorepo.yml`;
- `bibliography/`;
- `common/`;
- `constitution/`;
- `docs/`;
- `images/`;
- a single `volume-*` content directory;
- `.latexmkrc`;
- `main.tex`;
- repository guidance files such as `AGENTS.md`, `CLAUDE.md`, `GEMINI.md`,
  `DESIGN.md`, `README.md`, and `REPOSITORY_STRUCTURE.md`.

The `Learning-Real-Analysis` monorepo was also inspected. It currently has one
local merge/sync history divergence already reconciled locally during this
planning pass, and it still has untracked image files:

- `images/hausdorff.png`;
- `images/kolmogorov.png`;
- `images/newton.png`;
- `images/noether.png`;
- `images/poincare.png`;
- `images/reimann.png`.

Those untracked images were not modified or staged.

### Current Top-Level Content by Volume

| Volume repo | Major chapter folders | TeX files | `chapter.yaml` files | Notes dirs | Proof dirs | Archive dirs |
|---|---|---:|---:|---:|---:|---:|
| `lra-volume-i` | `algebras-of-sets`, `axiom-systems`, `lambda-calculus`, `model-theory`, `predicate-logic`, `proof-techniques`, `propositional-logic`, `sets-relations-functions`, `type-theory` | 333 | 12 | 22 | 12 | 2 |
| `lra-volume-ii` | `complex-numbers`, `integers`, `landau-foundations-analysis`, `lean`, `natural-numbers`, `numbers-summary`, `peano-systems`, `rationals`, `reals` | 341 | 8 | 16 | 7 | 0 |
| `lra-volume-iii` | `algebra`, `analysis`, `discrete-calculus`, `geometry`, `number-theory`, `quantum-calculus`, `topology` | 524 | 16 | 54 | 28 | 0 |
| `lra-volume-iv` | `calculus`, `computational-linear-algebra`, `fourier-analysis`, `geometric-modeling`, `ordinary-differential-equations` | 112 | 4 | 9 | 5 | 0 |
| `lra-volume-v` | `numerical-analysis` | 14 | 1 | 2 | 1 | 0 |

## Current Volume Title and Frontispiece Inventory

### Volume I

- Title page file: `volume-i/volume-i-title.tex`
- Quote/frontispiece file: `volume-i/cantor-quote.tex`
- Volume index: `volume-i/index.tex`
- Current title page title: `Sets and Logic`
- Current quote/frontispiece title: `Sets and Logic`
- Current part title/comment: `Mathematical Logic`
- Current mathematician/image: Georg Cantor, `images/cantor.png`
- Target: `Logic, Sets, and Proof` with Georg Cantor
- Notes:
  - The target mathematician is already correct.
  - The displayed title needs standardization across title page, quote page,
    index comments, and `\part`.

### Volume II

- Title page file: `volume-ii/volume-ii-title.tex`
- Quote/frontispiece file: `volume-ii/peano-quote.tex`
- Volume index: `volume-ii/index.tex`
- Current title page title: `Origins of the Numbers`
- Current quote/frontispiece title: `Origins of the Numbers`
- Current part title/comment: `Foundations of Formal Number Systems`
- Current mathematician/image: Giuseppe Peano, `images/peano.png`
- Target: `Origins of the Numbers` with Giuseppe Peano
- Notes:
  - Volume II should retain Peano.
  - `volume-ii/index.tex` currently inputs `volume-ii/peano-quote` twice.
  - The quote page misspells `Giuseppe` as `Guiseppe`.
  - The title should remain, but part-title/frontmatter conventions should be
    standardized later.

### Volume III

- Title page file: `volume-iii/volume-iii-title.tex`
- Quote/frontispiece file: `volume-iii/hilbert-quote.tex`
- Additional frontmatter/image file: `volume-iii/torus.tex`
- Volume index: `volume-iii/index.tex`
- Current title page title: `Advanced Mathematics`
- Current quote/frontispiece title: `Abstract Mathematics`
- Current part title/comment: `Volume III - Abstract Mathematics`
- Current `main.tex` comment: `Analysis, Metric Spaces, Topology`
- Current mathematician/image: David Hilbert, `images/hilbert.png`
- Additional image reference: `images/torus-pursuit-wallpaper.png`
- Target: `Analysis` with Augustin-Louis Cauchy
- Notes:
  - This is the largest frontmatter mismatch.
  - Hilbert should eventually move to the target Volume VIII.
  - The torus frontmatter/image should be reviewed separately and not changed
    during metadata standardization unless explicitly retained as non-volume
    frontmatter.
  - Analysis content should remain here in the short term.
  - Algebra and topology/geometry content should be mapped to later volumes.

### Volume IV

- Title page file: `volume-iv/volume-iv-title.tex`
- Quote/frontispiece file: `volume-iv/euler-quote.tex`
- Volume index: `volume-iv/index.tex`
- Current title page title: `Applied and Computational Mathematics`
- Current quote/frontispiece title: `Applied and Computational Mathematics`
- Current part title: `Volume IV - Applied and Computational Mathematics`
- Current mathematician/image: Leonhard Euler, `images/euler.png`
- Target: `Algebra and Abstract Structures` with Emmy Noether
- Notes:
  - Euler and most computational material should eventually shift to target
    Volume VI or Volume VII.
  - Noether image exists locally in the monorepo as an untracked file but was
    not modified or staged.

### Volume V

- Title page file: `volume-v/volume-v-title.tex`
- Quote/frontispiece file: `volume-v/gauss-quote.tex`
- Volume index: `volume-v/index.tex`
- Current title page title: `Numerical Mathematics and Approximation`
- Current quote/frontispiece title: `Numerical Mathematics and Approximation`
- Current part title/comment: `Numerical Analysis and Approximation`
- Current mathematician/image: Carl Gauss, `images/gauss.png`
- Target: `Topology and Geometry` with Henri Poincare
- Notes:
  - Current Volume V content belongs closer to target Volume VII.
  - Gauss is not part of the target frontispiece list.
  - `images/poincare.png` exists locally in the monorepo as an untracked file
    but was not modified or staged.

## Current Mathematician and Image Inventory

### Expected Target Images

The target architecture expects these monorepo-relative image paths:

| Target volume | Mathematician | Expected image path | Current split-repo status |
|---|---|---|---|
| Volume I | Georg Cantor | `images/cantor.png` | present in `lra-volume-i` |
| Volume II | Giuseppe Peano | `images/peano.png` | present in `lra-volume-ii` |
| Volume III | Augustin-Louis Cauchy | `images/cauchy.png` | missing; must be generated and tracked before the Volume III frontispiece is finalized |
| Volume IV | Emmy Noether | `images/noether.png` | tracked in the monorepo as of Phase 3C |
| Volume V | Henri Poincare | `images/poincare.png` | tracked in the monorepo as of Phase 3C |
| Volume VI | Leonhard Euler | `images/euler.png` | present in `lra-volume-iv` |
| Volume VII | Isaac Newton | `images/newton.png` | tracked in the monorepo as of Phase 3C |
| Volume VIII | David Hilbert | `images/hilbert.png` | present in `lra-volume-iii` |

### Current Split-Repo Image Directories

| Repository | Current `images/` contents |
|---|---|
| `lra-volume-i` | `cantor.png` |
| `lra-volume-ii` | `peano.png` |
| `lra-volume-iii` | `hilbert.png`, `torus-pursuit-wallpaper.png` |
| `lra-volume-iv` | `euler.png` |
| `lra-volume-v` | `gauss.png` |

### Image Path Convention

All inspected frontispiece image references already use the desired
monorepo-relative style:

```latex
images/<filename>.png
```

No inspected frontispiece uses a forbidden volume-local path such as
`volume-i/images/...` or `volume-*/images/...`.

## Current Workflow and Sync Inventory

### Split-Repo Sync Workflows

Each current volume repository has `.github/workflows/sync-to-monorepo.yml`.
Each workflow:

- runs on pushes to `main`;
- is path-scoped to that repository's `volume-*` directory;
- checks out `wsollers/Learning-Real-Analysis`;
- runs `rsync -av --delete source/volume-*/ monorepo/volume-*/`;
- commits `chore: sync volume-* from lra-volume-*` when changed.

These workflows now cover all eight active volume repositories.

### Monorepo Build and Root Files

The monorepo root `main.tex` currently inputs:

- `volume-i/index`;
- `volume-ii/index`;
- `volume-iii/index`;
- `volume-iv/index`;
- `volume-v/index`.

The monorepo currently has standalone root files:

- `volume-i-main.tex`;
- `volume-ii-main.tex`;
- `volume-iii-main.tex`;
- `volume-iv-main.tex`;
- `volume-v-main.tex`.

No standalone root files exist yet for:

- `volume-vi-main.tex`;
- `volume-vii-main.tex`;
- `volume-viii-main.tex`.

### Docker and Migration Scripts

Phase 3B updated the monorepo Docker compile script `docker/compile.ps1` from:

```powershell
$validVolumes = @('i', 'ii', 'iii', 'iv', 'v', 'vi', 'vii', 'viii')
```

Phase 3B updated the monorepo migration script `docker/migrate_volumes.py` from:

```python
VOLUMES = ["i", "ii", "iii", "iv", "v", "vi", "vii", "viii"]
```

The migration script recognizes all eight volume identifiers. Split
repositories now exist for Volumes VI-VIII.

### Knowledge Explorer Trigger

The monorepo workflow `.github/workflows/trigger-knowledge-rebuild.yml`
currently watches:

- `volume-i/**`;
- `volume-ii/**`;
- `volume-iii/**`;
- `volume-iv/**`;
- `volume-v/**`;
- `volume-vi/**`;
- `volume-vii/**`;
- `volume-viii/**`;
- `common/**`;
- `bibliography/**`;
- `theorem-explorer/**.py`.

This keeps the extraction rebuild trigger aligned with the active eight-volume
architecture.

### Documentation With Fixed Five-Volume Assumptions

Phase 3B updates the active monorepo documentation to mention the target
`lra-volume-i` through `lra-volume-viii` layout. Historical inventory notes in
this plan may still mention the pre-migration five-volume state.

- `DESIGN.md`;
- `REPOSITORY_STRUCTURE.md`;
- `docs/architecture/repository-layout.md`;
- `docs/architecture/volume-layout.md`;
- `docs/architecture/multi-repo-sync.md`;
- `docs/architecture/latex-build-and-rendering.md`;
- `docs/governance/repo-overlays/lra-volume.md`;
- `constitution/schema/file-schema.yaml`;
- `docker/README.md`;
- `docker/MIGRATION.md`.

## Proposed Target Volume Architecture

| Target volume | Repository | Display title | Frontispiece |
|---|---|---|---|
| Volume I | `wsollers/lra-volume-i` | `Logic, Sets, and Proof` | Georg Cantor |
| Volume II | `wsollers/lra-volume-ii` | `Origins of the Numbers` | Giuseppe Peano |
| Volume III | `wsollers/lra-volume-iii` | `Analysis` | Augustin-Louis Cauchy |
| Volume IV | `wsollers/lra-volume-iv` | `Algebra and Abstract Structures` | Emmy Noether |
| Volume V | `wsollers/lra-volume-v` | `Topology and Geometry` | Henri Poincare |
| Volume VI | `wsollers/lra-volume-vi` | `Computational Mathematics` | Leonhard Euler |
| Volume VII | `wsollers/lra-volume-vii` | `Numerical and Approximation Mathematics` | Isaac Newton |
| Volume VIII | `wsollers/lra-volume-viii` | `Mathematical Logic and Foundations` | David Hilbert |

## Proposed Repository Creation List

New repositories eventually needed:

- `wsollers/lra-volume-vi`;
- `wsollers/lra-volume-vii`;
- `wsollers/lra-volume-viii`.

Each should be created from the same Overleaf-ready volume repository pattern:

- `.github/workflows/sync-to-monorepo.yml`;
- generated agent instruction files;
- `bibliography/`;
- `common/`;
- `constitution/`;
- `docs/`;
- `images/`;
- `main.tex`;
- `.latexmkrc`;
- `volume-vi/`, `volume-vii/`, or `volume-viii/`.

Do not create these repositories until the metadata/frontmatter conventions
are standardized and the monorepo scripts know about eight volumes.

## Proposed Content Migration Map

### Content That Should Remain in Place Initially

- Volume I should keep the current logic, sets, and proof foundation content
  during early phases:
  - `propositional-logic`;
  - `predicate-logic`;
  - `sets-relations-functions`;
  - `axiom-systems`;
  - `proof-techniques`.
- Volume II should keep:
  - `peano-systems`;
  - `natural-numbers`;
  - `integers`;
  - `rationals`;
  - `reals`;
  - `complex-numbers`;
  - `landau-foundations-analysis`;
  - `numbers-summary`.
- Volume III should keep active analysis chapters during early phases:
  - `analysis/real-analysis`;
  - `analysis/bounding`;
  - `analysis/sequences`;
  - `analysis/elementary-functions`;
  - `analysis/functions`;
  - `analysis/continuity`;
  - `analysis/differentiation`;
  - `analysis/riemann-integration`;
  - `analysis/measure-theory`;
  - `analysis/series`;
  - `analysis/function-sequences`.

### Content That Appears to Belong in Analysis

- Volume II:
  - `reals` may stay in Volume II as construction/foundations, but any
    advanced analysis material inside it should be audited before moving.
  - `landau-foundations-analysis` can remain Volume II because it supports
    number-system foundations.
- Volume III:
  - all `analysis/*` chapters listed above.
- Volume IV:
  - `calculus` and `fourier-analysis` may belong to Analysis if treated
    theoretically, or to Computational Mathematics if treated operationally.
    They should be audited before moving.

### Content That Appears to Belong in Algebra and Abstract Structures

- Volume I:
  - `algebras-of-sets` is algebraic but also supports logic/set foundations.
    Keep in place initially; consider moving or cross-listing later.
- Volume III:
  - `algebra/abstract-algebra`;
  - `algebra/algebraic-structures`;
  - `algebra/linear-algebra`;
  - `algebra/order-and-lattice/lattice-order`;
  - `algebra/set-algebra/boolean-algebra`;
  - `algebra/set-algebras`;
  - `algebra/category-theory`;
  - `algebra/algebraic-geometry` may belong in Algebra or Geometry depending
    on final scope.
- Volume IV:
  - `computational-linear-algebra` should not move to pure Algebra until it is
    split into theoretical and computational parts.

### Content That Appears to Belong in Topology and Geometry

- Volume III:
  - `topology/point-set-topology`;
  - `geometry/trigonometry`;
  - `geometry/analytical-geometry`;
  - `geometry/euclidean-geometry`;
  - `geometry/non-euclidean-geometry`;
  - `geometry/classical-geometry`;
  - `geometry/differential-geometry`;
  - `geometry/riemannian-geometry`.
- Volume IV:
  - `geometric-modeling` may belong in Computational Mathematics or Topology
    and Geometry depending on whether it is presented as NURBS/computation or
    geometric theory.

### Content That Appears to Belong in Computational Mathematics

- Volume IV:
  - `computational-linear-algebra`;
  - computational parts of `geometric-modeling`;
  - computational/numerical parts of `ordinary-differential-equations`;
  - applied portions of `fourier-analysis`.
- Separate satellite repositories such as `lra-nurbs` should remain separate
  unless a later plan explicitly integrates expository content into Volume VI.

### Content That Appears to Belong in Numerical and Approximation Mathematics

- Volume V:
  - `numerical-analysis`;
  - `numerical-analysis/notes/interval-arithmetic`;
  - `numerical-analysis/notes/ieee-floating-point`.
- Volume IV:
  - `computational-linear-algebra/notes/numerical-linear-algebra`;
  - numerical portions of `ordinary-differential-equations`;
  - numerical approximation portions of `geometric-modeling`.

### Content That Appears to Belong in Mathematical Logic and Foundations

- Volume I:
  - `model-theory`;
  - `type-theory`;
  - `lambda-calculus`;
  - advanced or specialized parts of `axiom-systems`;
  - later advanced proof theory, computability, or set-theoretic foundations.
- These should not be moved immediately. Volume I needs them available while
  the foundational logic arc is rebuilt. Later phases can either keep early
  foundations in Volume I and move advanced material to Volume VIII, or create
  Volume VIII as a higher-level continuation with stubs and forward links.

### Stub Rather Than Move Immediately

Use stubs before full moves for:

- Volume IV `Algebra and Abstract Structures`;
- Volume V `Topology and Geometry`;
- Volume VI `Computational Mathematics`;
- Volume VII `Numerical and Approximation Mathematics`;
- Volume VIII `Mathematical Logic and Foundations`.

This avoids high-risk bulk moves and protects theorem labels, proof labels,
cross-references, and Overleaf file limits while the architecture stabilizes.

## Proposed Frontmatter and Image Standardization Plan

1. Define a canonical volume metadata block before changing display text:
   - volume number;
   - repository;
   - display title;
   - frontispiece mathematician;
   - lifespan;
   - image path;
   - quote/frontispiece file name.
2. Standardize all frontispiece image paths to `images/<filename>.png`.
3. Standardize quote/frontispiece file names in a later phase. Candidate
   convention:
   - `volume-i/frontispiece.tex`;
   - `volume-ii/frontispiece.tex`;
   - and so on.
4. Keep old quote filenames temporarily as router files if needed to avoid
   breaking existing inputs.
5. Update title page, quote/frontispiece page, `\part`, `main.tex` comments,
   and `chapter.yaml`/volume metadata together per volume.
6. Do not generate or replace images until the image inventory is confirmed.
7. When images are generated later, use the house style:
   monochrome engraved line art, circular medallion portrait with bold border,
   subtle associated formulas/diagrams, and a rectangular engraved nameplate
   containing exactly the mathematician's full name and lifespan.

## Proposed Workflow and Sync Update Plan

1. Update governance/docs to describe eight volumes rather than five.
2. Update `constitution/schema/file-schema.yaml` for `volume-vi`,
   `volume-vii`, and `volume-viii`.
3. Update monorepo root files:
   - `main.tex`;
   - `volume-vi-main.tex`;
   - `volume-vii-main.tex`;
   - `volume-viii-main.tex`.
4. Update Docker scripts:
   - `docker/compile.ps1`;
   - `docker/migrate_volumes.py`;
   - `docker/README.md`;
   - `docker/MIGRATION.md`.
5. Create sync workflows for new repos:
   - `lra-volume-vi/.github/workflows/sync-to-monorepo.yml`;
   - `lra-volume-vii/.github/workflows/sync-to-monorepo.yml`;
   - `lra-volume-viii/.github/workflows/sync-to-monorepo.yml`.
6. Update `trigger-knowledge-rebuild.yml` to watch all active volume trees.
7. Update repository maps:
   - `DESIGN.md`;
   - `REPOSITORY_STRUCTURE.md`;
   - architecture docs;
   - repo overlay docs.

## Risk List

- Overleaf file limits are the main driver; bulk moves could temporarily make
  one repo too large before the new repos are ready.
- Current monorepo and split repos have sync workflows that can overwrite
  volume directories with `rsync --delete`.
- Historical five-volume script assumptions have been updated, root inclusion
  is enabled, and VI-VIII split-repo sync workflows are active.
- Existing quote/frontispiece files mix display-title content with image and
  quote content.
- Volume III has substantial mixed-domain content; moving it all at once would
  be risky.
- Current Volume IV combines computational, applied, geometry, calculus, ODE,
  and Fourier material.
- Current Volume V is numerical but target Volume V is topology/geometry.
- Some target images are not tracked in split repos yet.
- `images/reimann.png` appears to be a misspelling of `riemann`; it is not part
  of the target frontispiece list and should not be normalized without a
  separate image inventory decision.
- The knowledge explorer now watches all eight active volume trees.
- Theorem labels, proof labels, citations, and cross-references must survive
  migration unchanged.
- Archives must remain intact.

## Recommended Commit Phases

### Phase 0: Inventory and Plan

- Add this report only.
- Do not edit TeX, workflows, images, or repository layout.

### Phase 1: Standardize Metadata and Frontmatter Conventions

- Add canonical metadata rules for volume title, frontispiece, image path, and
  quote/frontispiece file naming.
- Add or update governance docs for eight-volume architecture.
- Do not rename display titles yet.

### Phase 2: Rename Displayed Volume Titles in TeX

- Update title pages, quote/frontispiece pages, `\part` titles, and comments.
- Keep existing file names as routers where needed.
- Fix Volume II duplicate `peano-quote` input and Peano spelling.

### Phase 3: Create New Repositories or Stubs

- Create `lra-volume-vi`, `lra-volume-vii`, and `lra-volume-viii`.
- Add empty/stub volume directories and standalone builds.
- Add sync workflows but keep content minimal.

### Phase 4: Move or Stub Content Into the New Volume Layout

- Prefer stubs first.
- Move content only in small chapter-level commits.
- Preserve theorem labels, proof labels, citations, and cross-references.
- Leave archives in place.

### Phase 5: Attach Mathematician Frontispieces

- Confirm tracked image inventory.
- Generate missing images only after metadata and target filenames are stable.
- Use only `images/<filename>.png` references.

### Phase 6: Update Monorepo Workflows, Sync Scripts, Build Scripts, and Source-Push Scripts

- Update all fixed five-volume assumptions to eight volumes.
- Update migration, build, Docker, and extraction trigger paths.
- Test volume-specific builds and full monorepo build.

### Phase 7: Audit Links, Labels, Image Paths, and Build Outputs

- Run image path audit.
- Run label/reference audit.
- Run volume builds.
- Run monorepo build if file limits permit locally.
- Check knowledge explorer extraction scope.

## Exact Next Codex Prompt for Phase 1

```text
Phase 1: Standardize volume metadata and frontmatter governance only.

Do not rename displayed volume titles yet.
Do not move content.
Do not create repositories.
Do not edit images.
Do not alter theorem labels, proof labels, citations, or cross-references.

Using docs/plans/volume-renumbering-and-frontmatter-migration-plan.md as the
source plan, add governance/docs that define the canonical eight-volume
architecture, volume metadata fields, frontispiece metadata fields, image path
rules, and quote/frontispiece file naming convention.

Also audit where the current five-volume assumptions are documented, but do
not update scripts yet unless they are pure documentation references.

Return a summary of files changed and any remaining ambiguities.
```
