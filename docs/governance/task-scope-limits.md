# Task Scope Limits

Agents must classify the task before editing.

## Default Scope Rule

Edit only the repository and file families named by the task. Do not expand a
local cleanup, content edit, or governance task into a broad multi-repo
migration without explicit approval.

## Stop Conditions

Stop and ask before:

- force pushing,
- rebasing shared branches,
- resetting, stashing, or deleting user work,
- applying a non-fast-forward merge unexpectedly,
- overwriting generated files without a dry-run,
- editing downstream synced copies instead of their owning source repo,
- inventing mathematical predicates or theorem content.

## Generated Wrapper Scope

Generated wrapper work follows the controlled workflow in
`docs/workflows/generated-wrapper-sync.md`. Preview and drift reports are safe
when requested. Downstream writes require explicit repo selection and explicit
authorization for the write step.

## Protected Local Directory

Do not touch `Learning-Real-Analysis/scripts/` during governance migration
work. It is intentionally untracked and reserved for a later tooling
cleanup/migration task.
