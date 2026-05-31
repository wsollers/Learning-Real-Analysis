# Generated File Policy

Agent instruction files and downstream compatibility wrappers should be treated
as generated artifacts.

Generated files must include:

- this header form:

```markdown
<!--
GENERATED FILE — DO NOT EDIT BY HAND.

Source repo: wsollers/lra-governance
Generated from:
- docs/governance/...
- docs/architecture/...
- docs/governance/repo-overlays/<overlay>.md

Regenerate from lra-governance.
Emergency downstream edits must be ported upstream before the next sync.
-->
```

- source document list,
- source commit, revision, or hash when available,
- generation timestamp when useful and reproducibility does not require a
  stable preview,
- local-edit warning,
- pointer back to `lra-governance`.

Generated files are full-replaced outputs. Generators must support dry-run mode
and report drift before writing. Any future write mode must compare existing
downstream files with generated content before replacement.

Generated files must not contain secrets or machine-local credentials.

The generated file source list must be sufficient for a reviewer to identify
which global rules, architecture rules, and repo overlay were used.

Downstream generated wrappers are full-replaced only by the governed wrapper
sync tool. Downstream wrapper edits are emergency-only and must be ported
upstream before the next sync. Generated wrappers should be committed in
downstream PRs or controlled downstream commits only after review.
