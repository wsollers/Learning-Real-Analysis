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

## Generated And Synced Files

Do not edit downstream synced copies as sources of truth. Apply source changes
in the owning repo and use the approved sync path.
