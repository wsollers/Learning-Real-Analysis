# Volume Migration Script

Use `migrate_volumes.py` to copy all volume content from this monorepo into the
corresponding `lra-volume-N` repos.

## Quick start

```bash
# Install dependency
pip install requests

# Run (migrates active split volume repos I-V + syncs common/ to lra-common)
python3 docker/migrate_volumes.py --token ghp_YOURTOKEN

# Include planned VI-VIII only after their split repos exist
python3 docker/migrate_volumes.py --token ghp_YOURTOKEN --include-planned-split-repos

# Single volume only
python3 docker/migrate_volumes.py --token ghp_YOURTOKEN --volumes iii

# Re-sync common/ only
python3 docker/migrate_volumes.py --token ghp_YOURTOKEN --volumes i --skip-common
```

Create a token at https://github.com/settings/tokens — needs `repo` scope.

## What it migrates

- `volume-N/` content \u2192 `lra-volume-N`
- `common/` + `bibliography/` \u2192 `lra-common` (unless `--skip-common`)

The script recognizes Volumes I-VIII. By default it only migrates active split
repos I-V; split-repo pushes for VI-VIII are deferred until those repositories
exist.

Each repo gets a single commit with all files. Safe to re-run.
