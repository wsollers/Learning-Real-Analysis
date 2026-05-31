# Refactoring Standards

Repo-specific refactoring constraints belong in additive overlays, not in
divergent forks of these global rules.

## Default Rule

Refactor only the requested scope. Preserve labels, filenames, dependency
links, theorem/proof navigation, and extraction-visible structure unless the
task explicitly authorizes changing them.

## Mathematical Safety

Do not invent mathematical content during structural refactors. If a refactor
reveals a missing theorem, predicate, dependency, or proof obligation, report
it rather than silently filling it.

## Stub Chapters

When a governance rebuild needs planned chapter locations before content
migration, follow `stub-chapter-standards.md`. Stub chapters are architecture
only; they must preserve build shape without inventing mathematical content.

## Stub Sections

When a governance rebuild needs planned topic locations inside an existing
chapter, follow `stub-section-standards.md`. Stub sections are architecture
only; they must update local routers without inventing mathematical content.

## Generated And Synced Files

Do not edit downstream synced copies as sources of truth. Apply source changes
in the owning repo and use the approved sync path.
