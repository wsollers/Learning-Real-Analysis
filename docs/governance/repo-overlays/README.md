# Repo Overlays

Repo overlays are additive rule layers. They clarify how global governance
applies to one repository or repository class.

They must not become divergent forks of global rules.

Specialist rule placement:

- Lean-specific rules belong only in `lra-lean.md`.
- C++ / Vulkan / simulation rules belong only in `lra-nurbs.md`.
- Numerical-analysis / benchmark / plotting rules belong only in
  `lra-numerical-analysis.md`.
- PDF/source ingestion, bibliography extraction, local-model cleanup, and
  candidate staging rules belong only in `lra-pdf-extractor.md`.
- Source-profile selection, source classification, active-profile export, and
  project attachment staging rules belong only in `lra-source-profiles.md`.
- Volume repos receive only volume-content guidance.

Specialist overlays now include:

- `lra-lean.md`
- `lra-nurbs.md`
- `lra-knowledge-explorer.md`
- `lra-numerical-analysis.md`
- `lra-pdf-extractor.md`
- `lra-source-profiles.md`

Each generated downstream wrapper should combine the global rules with exactly
the matching overlay. Overlays should link to local README or workflow files
for operational details instead of copying large local technical manuals.
