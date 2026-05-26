#!/usr/bin/env python3
"""
migrate_volumes.py  (v4 — adds lean/ and nurbs_dde/ satellite repo migration)
------------------------------------------------------------------------------
Migrates content from Learning-Real-Analysis (monorepo) into the
corresponding split repos via the GitHub REST API.

Repo mapping:
  volume-N/   →  lra-volume-N  (path preserved: volume-N/ in both repos)
  lean/       →  lra-lean      (path remapped:  lean/* → repo root)
  nurbs_dde/  →  lra-nurbs     (path remapped:  nurbs_dde/* → repo root)
  common/ + bibliography/  →  lra-common  (path preserved)

Changes in v4:
  - Added --lean and --nurbs flags to migrate satellite repos
  - migrate_satellite() handles the monorepo-subdir → repo-root path remap
  - Excluded .github/ from satellite syncs to avoid overwriting actions

Changes in v3:
  - force=True on update_ref (fixes 422 on diverged branches)
  - Re-fetch HEAD before commit

Changes in v2:
  - create_tree chunked to <=200 entries (fixes 422 on large volumes)

Usage:
    pip install requests

    # Migrate all volumes + common:
    python3 migrate_volumes.py --token ghp_YOURTOKEN

    # Migrate lean and nurbs satellites:
    python3 migrate_volumes.py --token ghp_YOURTOKEN --lean --nurbs --skip-common

    # Single volume:
    python3 migrate_volumes.py --token ghp_YOURTOKEN --volumes iii --skip-common
"""

import argparse
import base64
import time
import requests


OWNER = "wsollers"
MONOREPO = "Learning-Real-Analysis"
BRANCH = "main"
VOLUMES = ["i", "ii", "iii", "iv", "v"]
TREE_CHUNK_SIZE = 200


def gh(token: str, method: str, path: str, **kwargs):
    url = f"https://api.github.com{path}"
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    r = requests.request(method, url, headers=headers, **kwargs)
    if r.status_code == 429 or (r.status_code == 403 and "rate" in r.text.lower()):
        reset = int(r.headers.get("X-RateLimit-Reset", time.time() + 60))
        wait = max(reset - time.time(), 5)
        print(f"  Rate limited — sleeping {wait:.0f}s")
        time.sleep(wait)
        r = requests.request(method, url, headers=headers, **kwargs)
    return r


def get_tree(token: str, repo: str, sha: str) -> list[dict]:
    r = gh(token, "GET", f"/repos/{OWNER}/{repo}/git/trees/{sha}?recursive=1")
    r.raise_for_status()
    data = r.json()
    if data.get("truncated"):
        print("  WARNING: tree was truncated. Some files may be missing.")
    return data["tree"]


def get_blob_content(token: str, repo: str, sha: str) -> bytes:
    r = gh(token, "GET", f"/repos/{OWNER}/{repo}/git/blobs/{sha}")
    r.raise_for_status()
    data = r.json()
    if data["encoding"] == "base64":
        return base64.b64decode(data["content"])
    return data["content"].encode()


def get_latest_commit_sha(token: str, repo: str, branch: str = "main") -> str:
    r = gh(token, "GET", f"/repos/{OWNER}/{repo}/git/ref/heads/{branch}")
    r.raise_for_status()
    return r.json()["object"]["sha"]


def get_tree_sha_for_commit(token: str, repo: str, commit_sha: str) -> str:
    r = gh(token, "GET", f"/repos/{OWNER}/{repo}/git/commits/{commit_sha}")
    r.raise_for_status()
    return r.json()["tree"]["sha"]


def create_blob(token: str, repo: str, content_bytes: bytes) -> str:
    encoded = base64.b64encode(content_bytes).decode()
    r = gh(token, "POST", f"/repos/{OWNER}/{repo}/git/blobs", json={
        "content": encoded,
        "encoding": "base64",
    })
    r.raise_for_status()
    return r.json()["sha"]


def create_tree_chunk(token: str, repo: str, base_tree_sha: str | None, entries: list[dict]) -> str:
    payload: dict = {"tree": entries}
    if base_tree_sha:
        payload["base_tree"] = base_tree_sha
    r = gh(token, "POST", f"/repos/{OWNER}/{repo}/git/trees", json=payload)
    if not r.ok:
        print(f"  create_tree failed: {r.status_code} {r.text[:400]}")
        r.raise_for_status()
    return r.json()["sha"]


def create_tree_chunked(token: str, repo: str, base_tree_sha: str, entries: list[dict]) -> str:
    total = len(entries)
    current_base = base_tree_sha
    for start in range(0, total, TREE_CHUNK_SIZE):
        chunk = entries[start : start + TREE_CHUNK_SIZE]
        end = min(start + TREE_CHUNK_SIZE, total)
        print(f"    tree chunk [{start+1}–{end} / {total}]")
        current_base = create_tree_chunk(token, repo, current_base, chunk)
    return current_base


def create_commit(token: str, repo: str, tree_sha: str, parent_sha: str, message: str) -> str:
    r = gh(token, "POST", f"/repos/{OWNER}/{repo}/git/commits", json={
        "message": message,
        "tree": tree_sha,
        "parents": [parent_sha],
    })
    r.raise_for_status()
    return r.json()["sha"]


def update_ref(token: str, repo: str, commit_sha: str, branch: str = "main"):
    r = gh(token, "PATCH", f"/repos/{OWNER}/{repo}/git/refs/heads/{branch}", json={
        "sha": commit_sha,
        "force": True,
    })
    if not r.ok:
        print(f"  update_ref failed: {r.status_code} {r.text[:300]}")
        r.raise_for_status()


def _do_migrate(token: str, target_repo: str, target_files: list[dict],
                strip_prefix: str, commit_message: str):
    """
    Core migration logic. Uploads blobs, builds tree, commits.
    strip_prefix: path prefix to remove from monorepo paths when writing
                  to target repo (e.g. 'lean/' so lean/LRA/X → LRA/X).
                  Pass '' to preserve paths as-is.
    """
    print(f"  Fetching {target_repo} HEAD...")
    target_commit = get_latest_commit_sha(token, target_repo)
    target_tree_sha = get_tree_sha_for_commit(token, target_repo, target_commit)
    print(f"  Target commit: {target_commit[:10]}")

    print(f"  Uploading {len(target_files)} blobs to {target_repo}...")
    tree_entries = []
    for i, item in enumerate(target_files, 1):
        if i % 50 == 0 or i == 1 or i == len(target_files):
            print(f"    [{i}/{len(target_files)}] {item['path']}")
        content = get_blob_content(token, MONOREPO, item["sha"])
        new_blob_sha = create_blob(token, target_repo, content)
        dest_path = item["path"]
        if strip_prefix and dest_path.startswith(strip_prefix):
            dest_path = dest_path[len(strip_prefix):]
        tree_entries.append({
            "path": dest_path,
            "mode": item.get("mode", "100644"),
            "type": "blob",
            "sha": new_blob_sha,
        })
        time.sleep(0.05)

    print(f"  Building tree in chunks of {TREE_CHUNK_SIZE}...")
    new_tree_sha = create_tree_chunked(token, target_repo, target_tree_sha, tree_entries)

    print("  Re-fetching HEAD before commit...")
    target_commit = get_latest_commit_sha(token, target_repo)

    print("  Creating commit...")
    new_commit_sha = create_commit(token, target_repo, new_tree_sha, target_commit, commit_message)

    print("  Updating main branch (force)...")
    update_ref(token, target_repo, new_commit_sha)
    print(f"  ✓ Done! {len(target_files)} files in {target_repo}")


def migrate_volume(token: str, volume: str):
    vol_dir = f"volume-{volume}"
    target_repo = f"lra-volume-{volume}"
    print(f"\n{'='*60}")
    print(f"Migrating {vol_dir} → {target_repo}")
    print(f"{'='*60}")

    print("  Fetching monorepo tree...")
    mono_commit = get_latest_commit_sha(token, MONOREPO)
    mono_tree_sha = get_tree_sha_for_commit(token, MONOREPO, mono_commit)
    mono_tree = get_tree(token, MONOREPO, mono_tree_sha)

    target_files = [
        item for item in mono_tree
        if item["type"] == "blob" and item["path"].startswith(f"{vol_dir}/")
    ]
    print(f"  Found {len(target_files)} files in {vol_dir}/")
    if not target_files:
        print("  WARNING: no files found — skipping")
        return

    _do_migrate(
        token, target_repo, target_files,
        strip_prefix="",   # volume-N/ path preserved in both repos
        commit_message=f"feat: migrate {vol_dir} content from Learning-Real-Analysis monorepo",
    )


def migrate_satellite(token: str, monorepo_dir: str, target_repo: str):
    """
    Migrate a satellite project (lean, nurbs_dde) from monorepo subdir
    to target repo root. Files under monorepo_dir/ land at repo root.
    .github/ files in the target repo are never overwritten.
    """
    print(f"\n{'='*60}")
    print(f"Migrating {monorepo_dir}/ → {target_repo} (repo root)")
    print(f"{'='*60}")

    print("  Fetching monorepo tree...")
    mono_commit = get_latest_commit_sha(token, MONOREPO)
    mono_tree_sha = get_tree_sha_for_commit(token, MONOREPO, mono_commit)
    mono_tree = get_tree(token, MONOREPO, mono_tree_sha)

    prefix = f"{monorepo_dir}/"
    target_files = [
        item for item in mono_tree
        if item["type"] == "blob"
        and item["path"].startswith(prefix)
        and not item["path"].startswith(f"{prefix}.github/")
    ]
    print(f"  Found {len(target_files)} files in {monorepo_dir}/")
    if not target_files:
        print("  WARNING: no files found — skipping")
        return

    _do_migrate(
        token, target_repo, target_files,
        strip_prefix=prefix,  # lean/LRA/X.lean → LRA/X.lean at repo root
        commit_message=f"feat: migrate {monorepo_dir}/ content from Learning-Real-Analysis monorepo",
    )


def sync_common_to_lra_common(token: str):
    print(f"\n{'='*60}")
    print("Syncing common/ + bibliography/ → lra-common")
    print(f"{'='*60}")

    mono_commit = get_latest_commit_sha(token, MONOREPO)
    mono_tree_sha = get_tree_sha_for_commit(token, MONOREPO, mono_commit)
    mono_tree = get_tree(token, MONOREPO, mono_tree_sha)

    target_files = [
        item for item in mono_tree
        if item["type"] == "blob" and (
            item["path"].startswith("common/") or
            item["path"].startswith("bibliography/")
        )
    ]
    print(f"  Found {len(target_files)} files")

    _do_migrate(
        token, "lra-common", target_files,
        strip_prefix="",
        commit_message="chore: sync common/ and bibliography/ from Learning-Real-Analysis monorepo",
    )


def main():
    parser = argparse.ArgumentParser(
        description="Migrate LRA content from monorepo to split repos (v4)"
    )
    parser.add_argument("--token", required=True, help="GitHub PAT with repo scope")
    parser.add_argument("--volumes", nargs="+", default=VOLUMES, choices=VOLUMES,
                        help="Volumes to migrate (default: all)")
    parser.add_argument("--lean", action="store_true",
                        help="Migrate lean/ → lra-lean")
    parser.add_argument("--nurbs", action="store_true",
                        help="Migrate nurbs_dde/ → lra-nurbs")
    parser.add_argument("--skip-common", action="store_true",
                        help="Skip syncing common/ to lra-common")
    parser.add_argument("--skip-volumes", action="store_true",
                        help="Skip all volume migrations")
    args = parser.parse_args()

    print("LRA Migration (v4)")
    print(f"Owner:   {OWNER}")
    print(f"Source:  {MONOREPO}")
    print()

    if not args.skip_common:
        sync_common_to_lra_common(args.token)

    if not args.skip_volumes:
        for vol in args.volumes:
            migrate_volume(args.token, vol)

    if args.lean:
        migrate_satellite(args.token, "lean", "lra-lean")

    if args.nurbs:
        migrate_satellite(args.token, "nurbs_dde", "lra-nurbs")

    print(f"\n{'='*60}")
    print("Migration complete!")
    print(f"{'='*60}")
    print()
    print("Reminder: add SYNC_PAT secret to lra-lean and lra-nurbs")
    print("  Settings → Secrets and variables → Actions → SYNC_PAT")
    print()


if __name__ == "__main__":
    main()
