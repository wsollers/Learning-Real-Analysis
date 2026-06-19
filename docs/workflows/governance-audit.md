# Governance Audit Workflow

Use this workflow when auditing governance bloat, authority leakage, task
routing, prompt size, or duplicate rule descriptions.

## Scope

Read the full governance corpus only when the user explicitly asks for a
governance audit or consolidation task. Ordinary implementation tasks should
start with `AGENTS.md` and `docs/agent-task-index.md`.

Do not create a new governance document unless no existing authority layer can
reasonably own the content.

## Checklist

1. Confirm `AGENTS.md` remains short and router-like.
2. Confirm `docs/agent-task-index.md` routes tasks to minimal required docs,
   schemas, tools, outputs, and validation checks.
3. Confirm constitution files answer what is valid, not repository layout or
   workflow procedure.
4. Confirm architecture docs own repo maps, volume maps, folder layout, sync
   topology, build boundaries, and generated-file ownership.
5. Confirm governance docs state authored-content rules without duplicating
   detailed workflow steps.
6. Confirm workflow docs state how to perform a task and point to canonical
   standards/schema for the rules.
7. Confirm repo overlays add local constraints without forking global rules.
8. Confirm prompts consume schema/data files instead of embedding large copied
   rule lists when the rule is machine-readable.
9. Confirm synced downstream copies are treated as redundant by design for
   isolated checkouts, not as additional canonical sources.
10. Confirm machine-checkable rules are represented in existing schema/data
    files before proposing a new schema system.
11. When any file under `constitution/schema/` changes, update or explicitly
    audit the deterministic validators that enforce it in the same change.
    For volume, chapter, file, block, or artifact-matrix rules, update the
    relevant `tools/governance/validate_volume.py` module or document why the
    requirement is delegated to another deterministic validator.

## Mechanical Checks

Run the checks that are available for the audit target. Typical checks include:

```powershell
python -m py_compile tools\governance\audit_proof_layout.py tools\governance\audit_volume_layout.py tools\governance\validate_volume.py
```

If schema files changed, also run the integrated validator against an affected
leaf volume:

```powershell
python tools\governance\validate_volume.py <target-repo> --fail-on-errors
```

```powershell
python - <<'PY'
from pathlib import Path
import json
import yaml

for path in Path("constitution/schema").glob("*.yaml"):
    yaml.safe_load(path.read_text(encoding="utf-8"))
json.loads(Path("constitution/schemas/audit-report.json").read_text(encoding="utf-8"))
print("schema parse: PASS")
PY
```

Use grep-based checks for repeated headings, repeated key phrases, and stale
path references when no dedicated validator exists. Do not claim full link or
markdown validation unless such a checker actually ran.

## Report

Report:

- authority leaks found,
- duplicated rules and their proposed canonical home,
- misplaced architecture/workflow/governance material,
- schema/data extraction candidates,
- task routes that need tightening,
- validation commands run,
- downstream synced copies that need regeneration.
