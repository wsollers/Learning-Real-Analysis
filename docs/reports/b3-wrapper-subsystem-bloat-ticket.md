# Ticket B3 — Wrapper-subsystem artifact bloat (PARKED / separate cleanup)

**Status:** Parked. Out of scope for the validator-engine / capability-system refactor.
**Origin:** Surfaced as finding **B3** in the START-HERE audit (bloat pass).
**Owner ruling (recorded):** Park it; log as a separate cleanup ticket; do **not** let it
touch the validator-engine / capability work.

## Scope boundary

This bloat belongs to the **agent-wrapper generation/sync subsystem**
(`generate_agent_wrappers.py`, `sync_agent_wrappers.py`, `report_wrapper_drift.py`,
`merge_repo_overlays.py`, `validate_repo_rules.py`, `tools/governance/templates/`), **not**
the decoration validator engine or the capability/resolver/overlay system. Keep this cleanup
on its own branch/PR so it cannot perturb the engine refactor.

## Inventory (committed, regenerable artifacts)

Three near-identical full materialized copies of every repo's five wrapper files
(`AGENTS.md`, `CLAUDE.md`, `GEMINI.md`, `.github/copilot-instructions.md`,
`.github/instructions/lra.instructions.md`):

- `build/wrapper-preview/`
- `reports/generated-agent-wrapper-preview/`
- `tmp/verification-preview/`

Plus ~14 timestamped drift/sync snapshot directories under `reports/`:

- `reports/wrapper-drift/`, `reports/wrapper-drift-after-*/`, `reports/wrapper-drift-before-*/`,
  `reports/wrapper-drift-final/`
- `reports/wrapper-sync-*/`

## Proposed cleanup (for the separate PR)

1. Confirm nothing in CI or the wrapper pipeline consumes these committed copies as **inputs**
   (they appear to be **outputs**/snapshots; the generator + templates + `overlays-config`
   are the real source of truth).
2. Move the regenerable preview/verification trees and timestamped snapshots to `.gitignore`
   (`build/`, `tmp/`, and the `reports/wrapper-drift-*` / `reports/wrapper-sync-*` snapshots),
   or delete them outright if no longer referenced.
3. Keep exactly one canonical preview location if a committed preview is genuinely wanted;
   drop the other two.

## Why parked, not done now

The current refactor pass is scoped to MAXIMIZE COVERAGE / MINIMIZE BLOAT on the **validator
engine + capability system**. This is real bloat but in an adjacent subsystem; folding it in
now would widen the blast radius of the engine work for no engine benefit.
