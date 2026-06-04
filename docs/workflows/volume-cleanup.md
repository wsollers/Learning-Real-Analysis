# Volume Cleanup Workflow

Volume cleanup is a high-fidelity refactor of existing mathematical notes. It
must preserve mathematical content while improving structure, labels,
decoration, and extraction readiness.

## Scope

Edit only the named volume repo and named content scope. Do not turn a local
cleanup into a multi-repo sync, shared-LaTeX edit, Lean task, NURBS/Vulkan task,
numerical-analysis task, or PDF-extractor task.

Volume repos own `volume-N/` content only. Synced `common/`,
generated wrappers, canonical YAML, and governance files are not volume-owned
source.

## Cleanup Rules

Preserve:

- theorem and definition meaning,
- labels and references unless explicitly authorized,
- proof navigation,
- dependency links,
- chapter and proof input order,
- one-object-per-environment structure.

Apply current standards for:

- structural/load-bearing box decisions,
- interpretation remarks when useful,
- formal or quantified statement blocks when applicable,
- predicate readings only where canonical predicates apply,
- dependency remarks with statement-label `\hyperref` targets,
- theorem/proof separation.

If a missing predicate, label, proof, dependency, or mathematical clarification
is discovered, record it for review. Do not silently invent it.
