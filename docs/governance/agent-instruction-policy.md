# Agent Instruction Policy

Canonical rules live under `docs/governance/`, `docs/architecture/`, and
`docs/governance/repo-overlays/`.

## Canonical Sources

Canonical agent-instruction sources are:

- `docs/governance/*.md`
- `docs/architecture/*.md`
- `docs/governance/repo-overlays/*.md`

The root `AGENTS.md` in `lra-governance` is the local router for agents working
in this source repo. It points to canonical source directories rather than
replacing them.

Agent-specific files are generated artifacts:

- downstream `AGENTS.md`
- downstream `CLAUDE.md`
- downstream `GEMINI.md`
- downstream `.github/copilot-instructions.md`
- downstream `.github/instructions/*.instructions.md`
- downstream compatibility wrappers, if any

## Generation Rule

Generated agent files must be assembled from:

1. global core governance rules,
2. exactly one appropriate repo overlay or repo-family overlay,
3. provider-specific wrapper formatting.

Generation formula:

```text
global core rules + repo overlay + provider wrapper = generated instruction file
```

## Provider Rule

- Codex uses `AGENTS.md`.
- Claude uses `CLAUDE.md`, which should import or point to `AGENTS.md` where
  supported.
- Copilot uses `.github/copilot-instructions.md` and optionally
  `.github/instructions/*.instructions.md`.
- Gemini uses `GEMINI.md`.

## Source Header Rule

Every generated agent file must identify:

- the source repo (`lra-governance`),
- the source documents used,
- whether the file is generated,
- the local-edit policy.

## Secret Rule

Generated instructions may name required secret variables, such as `SYNC_PAT`,
but must never contain token values, credentials, or machine-local secrets.

## Drift Rule

Downstream generated files must not become local sources of truth. Emergency
local repairs must be ported back to `lra-governance` before the next sync.

## Local Edit Rule

Generated files are full-replaced by governance sync. Emergency downstream
edits are allowed only as temporary repair and must be ported upstream into
`lra-governance` before the next generation or sync.
