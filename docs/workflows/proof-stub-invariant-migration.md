# Proof Stub Invariant Migration

This workflow applies while older leaf volume repositories are being migrated
to the theorem/proof-stub invariant.

## Temporary State

The invariant is active for new work: every new top-level theorem,
proposition, lemma, or corollary that is expected to have a proof must be
committed with a compile-safe proof stub and `\LRAProofFor{...}`.

Older proof files may temporarily fail validation because they predate
`\LRAProofFor{...}` or the required two-body proof-stub structure. Those files
should be migrated with the leaf-local migration tool rather than edited in
`Learning-Real-Analysis`.

## Migration Command

Preview first:

```bash
python scripts/migrate_existing_proofs_to_invariant.py --root . --dry-run
```

Apply only after reviewing the report:

```bash
python scripts/migrate_existing_proofs_to_invariant.py --root . --apply
python scripts/build_volume.py --validate-only
```

## Sync Gate

The preferred policy is hard blocking: leaf repos that fail strict validation
should not sync until migrated. Do not add a non-blocking migration-mode flag
unless explicitly approved for a specific repository and removed after the
migration window.
