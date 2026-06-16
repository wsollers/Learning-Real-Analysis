# Populate Proof Stub

Use this workflow when a proof stub already exists and the task is to replace
TODO proof bodies with canonical proof content.

## Required Steps

1. Work in the leaf volume repository that owns the proof file.
2. Locate the existing proof file by theorem label and `\LRAProofFor{...}`.
3. Preserve the durable proof container described in
   `docs/governance/proof-standards.md` and
   `constitution/schema/file-schema.yaml`.
4. Replace only the professional proof TODO body and detailed learning proof
   TODO body unless the task explicitly authorizes broader edits.
5. Add or refine dependency/proof-structure remarks as needed.
6. Run:

```bash
python scripts/build_volume.py --validate-only
```

## Invariant

Proof population happens in place. Do not delete, replace, or relocate the
proof file as part of ordinary proof generation.
