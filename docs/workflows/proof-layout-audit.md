# Proof Layout Audit Workflow

Use this workflow to mechanically classify canonical proof files as compliant
full proofs, compliant proof stubs, or non-compliant proof files.

Machine-readable proof layout authority lives in
`constitution/schema/file-schema.yaml`.

Run the deterministic audit tool from `lra-governance`:

```powershell
python tools\governance\audit_proof_layout.py --root F:\repos\lra-volume-ii --refactor-mode
```

Use `--strict` when the target repo is expected to satisfy the current
topic-mirrored proof architecture:

```powershell
python tools\governance\audit_proof_layout.py --root F:\repos\lra-volume-ii --strict
```

Use JSON output for generated reports:

```powershell
python tools\governance\audit_proof_layout.py --root F:\repos\lra-volume-ii --format json
```

The audited rule set is the proof-file and proof-status portion of
`constitution/schema/file-schema.yaml`, with human-facing context in
`docs/governance/proof-standards.md`.

The audit does not judge mathematical correctness.
