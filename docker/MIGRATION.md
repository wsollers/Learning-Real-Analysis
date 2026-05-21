# Volume Migration Script

Use `migrate_volumes.py` to copy all volume content from this monorepo into the
corresponding `lra-volume-N` repos.

## Quick start

```bash
# Install dependency
pip install requests

# Run (migrates all 5 volumes + syncs common/ to lra-common)
python3 docker/migrate_volumes.py --token ghp_YOURTOKEN

# Single volume only
python3 docker/migrate_volumes.py --token ghp_YOURTOKEN --volumes iii

# Re-sync common/ only
python3 docker/migrate_volumes.py --token ghp_YOURTOKEN --volumes i --skip-common
```

Create a token at https://github.com/settings/tokens — needs `repo` scope.

## What it migrates

- `volume-N/` content \u2192 `lra-volume-N`
- `common/` + `bibliography/` \u2192 `lra-common` (unless `--skip-common`)

Each repo gets a single commit with all files. Safe to re-run.
