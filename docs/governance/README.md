# LRA Governance

This directory is the canonical source for project-wide governance rules.
`DESIGN.md` is now a lightweight router into these modular sources, not the
full governance manual.

Agent-specific files such as `AGENTS.md`, `CLAUDE.md`, `GEMINI.md`,
`.github/copilot-instructions.md`, and `.github/instructions/*.instructions.md`
are generated artifacts, not source-of-truth documents.

Future generators will combine:

1. global core governance rules,
2. the appropriate repo overlay,
3. provider-specific wrapper formatting.

Repo overlays are additive. They refine the global rules for a repository's
owned work, but they do not fork or weaken the global rules.

## Modules

- `agent-instruction-policy.md`
- `task-scope-limits.md`
- `authoring-standards.md`
- `proof-standards.md`
- `handwritten-proof-vault-standards.md`
- `extraction-standards.md`
- `notation-standards.md`
- `file-splitting-standards.md`
- `refactoring-standards.md`
- `stub-chapter-standards.md`
- `stub-section-standards.md`
- `model-view-standards.md`
- `build-render-standards.md`
- `decoration-audit-standards.md`
- `repo-overlays/`

## Source Compatibility

Current policy should be added to the smallest applicable modular document in
this directory or in `docs/architecture/`, `docs/governance/repo-overlays/`, or
`docs/workflows/`. Do not add large rule blocks to `DESIGN.md`.

## Rule Boundaries

Global rules apply to every repository unless an additive repo overlay narrows
the operational context. Overlays may add repository-specific validation,
build, or ownership rules, but they must not contradict the global rules.
