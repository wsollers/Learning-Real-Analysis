# Volume II Dependency Graph Pilot

Date: 2026-06-12

## Scope

This pilot starts the dependency-graph cleanup without rewriting source files.

Universe scope:

- all active `lra-volume-*` repositories under `F:\repos`;
- active volume source only;
- archives, generated folders, common files, docs, build output, and governance
  copies are excluded.

Cleanup scope:

- `lra-volume-ii` dependency declarations and immediate edges.

## Tooling Added

Added `tools/governance/dependency_graph.py` with read-only subcommands:

```powershell
python tools/governance/dependency_graph.py universe --repos-root F:\repos --repo-filter lra-volume-* --out build/dependency-graph/formal-universe.json --markdown build/dependency-graph/formal-universe.md
python tools/governance/dependency_graph.py edges --root F:\repos\lra-volume-ii --universe build/dependency-graph/formal-universe.json --out build/dependency-graph/volume-ii-edges.json --markdown build/dependency-graph/volume-ii-edges.md
python tools/governance/dependency_graph.py validate --universe build/dependency-graph/formal-universe.json --edges build/dependency-graph/volume-ii-edges.json --policy docs/governance/dependency-root-policy.yaml --out build/dependency-graph/volume-ii-validation.json --markdown build/dependency-graph/volume-ii-validation.md
```

Added `docs/governance/dependency-root-policy.yaml` as the root policy scaffold.
It currently allows all `ax:*` labels as roots, but has no primitive definitions
or external assumptions listed yet.

## Pilot Output

Global formal universe:

- repos scanned: 8;
- formal nodes: 1,302;
- universe issues: 54.

Volume II edge extraction:

- declarations: 330;
- extracted edges: 481;
- extraction issues: 186.

Volume II validation:

- errors: 161;
- warnings: 255.

## Issue Categories

| Issue | Count | Meaning |
|---|---:|---|
| `missing_formal_label` | 29 | Active formal environments without labels in the global universe. |
| `duplicate_global_label` | 25 | Labels that collide across active volume repos. |
| `missing_dependency_declaration` | 87 | Volume II formal nodes with no dependency declaration. |
| `legacy_dependency_remark` | 79 | Volume II nodes using `remark*[Dependencies]` instead of the canonical `dependencies` environment. |
| `dependencies_without_hyperref` | 10 | Dependency blocks with no parseable formal hyperref edge. |
| `missing_dependency_target` | 6 | Dependency targets not found in the global universe. |
| `ambiguous_dependency_target` | 4 | Dependency targets with multiple global matches. |
| `closure_leaf_not_allowed_root` | 176 | The closure reaches a leaf that is neither an axiom nor listed as an approved primitive/root. |

## Interpretation

The global universe extraction is viable, but global label uniqueness is not yet
clean. This matters before a global topological sort can be trusted.

Volume II already has many parseable immediate edges. The first cleanup pass
should focus on mechanical correctness:

1. resolve global duplicate labels that affect Volume II dependency targets;
2. repair missing dependency targets such as `def:real-as-cut` and
   `def:real-as-cauchy`;
3. add missing dependency declarations to Volume II nodes;
4. convert legacy dependency remarks to the canonical environment after the
   graph is semantically corrected;
5. populate `dependency-root-policy.yaml` with reviewed primitive definitions
   and documented external assumptions.

The closure warnings are expected with an empty primitive-definition policy.
They should not be treated as final failures until the root policy has been
reviewed.

## No Rewrite Yet

This pilot intentionally does not rewrite any dependency block. Canonical
rendering should wait until the edge set and root policy are corrected.
