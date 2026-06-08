# Volume Layout Audit Workflow

Use this workflow to mechanically audit volume, chapter, and topic folder
layout.

Machine-readable layout authority lives in
`constitution/schema/file-schema.yaml`.

Run from `lra-governance` against a leaf repo, volume, chapter, or section.
When unsure, discover the available targets first:

```powershell
python tools\governance\audit_volume_layout.py --root F:\repos\lra-volume-ii --list-targets
```

Use `--strict` when the target is expected to satisfy the current
volume/chapter/topic architecture:

```powershell
python tools\governance\audit_volume_layout.py --root F:\repos\lra-volume-ii --chapter whole-numbers --strict
```

Use `--volume <volume-name>`, `--chapter <chapter-name>`, or
`--section <topic-name>` to avoid whole-repo legacy noise. Section scope audits
the containing chapter because topic pairing and router reachability are
chapter-level invariants.

Use JSON output for generated reports:

```powershell
python tools\governance\audit_volume_layout.py --root F:\repos\lra-volume-ii --format json
```

The audited rule set is the volume/chapter/topic layout portion of
`constitution/schema/file-schema.yaml`, with human-facing context in
`docs/architecture/volume-layout.md`.

The audit does not move files or modify source.

## Route Metadata After Refactor

If the audited refactor moved theorem source paths, proof source paths, or the
topic folders containing them, regenerate and validate the leaf theorem-route
artifacts before syncing to the proof vault:

```powershell
python scripts\generate_theorem_routes.py --root .
python scripts\generate_theorem_routes.py --root . --validate-only
```

The proof vault must consume the regenerated route snapshot rather than stale
`theorem_tex` or `proof_tex` paths.
