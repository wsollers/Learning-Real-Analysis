# Generated Wrapper Sync Workflow

Generated agent wrappers are derived from `lra-governance` and copied
downstream only through a controlled process.

## Source Flow

The generation formula is:

```text
global governance docs + repo overlay + provider wrapper format
```

Generated downstream files are not canonical. Emergency downstream edits must
be ported upstream into `lra-governance` before the next sync.

## Preview And Validation

Before any write:

1. Generate wrapper previews under a report directory.
2. Validate preview headers, overlay routing, and specialist-rule boundaries.
3. Run wrapper drift reporting against downstream repos.
4. Review the planned create, replace, identical, and blocked statuses.

Preview and drift outputs are local reports and should not be committed unless
a task explicitly asks for a report artifact.

## Controlled Write

Wrapper sync is dry-run by default. Write mode must be explicit,
repo-selected, and guarded. It must not silently sync every repo.

The sync tool writes only generated wrapper files:

- `AGENTS.md`
- `CLAUDE.md`
- `GEMINI.md`
- `.github/copilot-instructions.md`
- `.github/instructions/lra.instructions.md`

Write mode must refuse dirty target repos and non-main target branches unless a
task explicitly authorizes an exception. The sync tool does not stage, commit,
or push downstream repos.

## Review

Downstream generated wrappers should be committed through reviewable PRs or
controlled commits after inspection. Reviewers should check the generated-file
header, source repo, overlay routing, absence of secrets, and absence of
unrelated positive specialist guidance.
