# Add Theorem With Proof Stub

Use this workflow in a leaf volume repository. Do not create canonical theorem
or proof source files directly in `Learning-Real-Analysis`.

## Required Steps

1. Add the theorem-like statement in the appropriate `notes/**/*.tex` file.
2. Give every top-level `theorem`, `proposition`, `lemma`, and `corollary` a
   stable source label.
3. Create the matching proof stub in the leaf repo `proofs/**/*.tex` tree.
4. Follow the proof-stub structure in `docs/governance/proof-standards.md` and
   `constitution/schema/file-schema.yaml`.
5. Inspect nearby statements, proof stubs, and topic indexes before editing.
6. Ensure the proof file is routed through the chapter's proof indexes.
7. Run:

```bash
python scripts/validate_leaf_proofs.py --root . --strict
```

The theorem and proof stub should be committed together. If validation fails,
the leaf repo is not ready to sync into the integration monorepo.

## Ownership

The leaf repo is the source of truth. `Learning-Real-Analysis` receives the
result through the approved sync/pull path.
