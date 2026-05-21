#!/usr/bin/env python3
"""
migrate_volumes.py  (v2 — fixes 422 on create_tree)
----------------------------------------------------
Migrates all volume content from Learning-Real-Analysis (monorepo) into
the corresponding lra-volume-N repos via the GitHub REST API.

Fix in v2:
  The 422 error on create_tree happens when the entry list is large.
  Solution: chunk tree entries into batches of <=200, chaining the
  resulting tree SHAs so each batch builds on the previous one.

Usage:
    pip install requests
    python3 migrate_volumes.py --token ghp_YOURTOKEN

    # Single volume only (e.g. retry iii after the 422):
    python3 migrate_volumes.py --token ghp_YOURTOKEN --volumes iii

    # Skip syncing common/ to lra-common (if already done):
    python3 migrate_volumes.py --token ghp_YOURTOKEN --skip-common

Requirements:
    pip install requests
"""

import argparse
import base64
import time
import requests


OWNER = "wsollers"
MONOREPO = "Learning-Real-Analysis"
BRANCH = "main"
VOLUMES = ["i", "ii", "iii", "iv", "v"]
TREE_CHUNK_SIZE = 200   # entries per create_tree call; keep well under GitHub limit


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
    """Get full recursive tree for a given tree SHA."""
    r = gh(token, "GET", f"/repos/{OWNER}/{repo}/git/trees/{sha}?recursive=1")
    r.raise_for_status()
    data = r.json()
    if data.get("truncated"):
        print("  WARNING: tree was truncated (>100k entries). Some files may be missing.")
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
    """
    Create one tree from a chunk of entries, built on top of base_tree_sha.
    Returns the new tree SHA.
    """
    payload: dict = {"tree": entries}
    if base_tree_sha:
        payload["base_tree"] = base_tree_sha

    r = gh(token, "POST", f"/repos/{OWNER}/{repo}/git/trees", json=payload)
    if not r.ok:
        print(f"  create_tree failed: {r.status_code} {r.text[:400]}")
        r.raise_for_status()
    return r.json()["sha"]


def create_tree_chunked(token: str, repo: str, base_tree_sha: str, entries: list[dict]) -> str:
    """
    Build a tree from potentially many entries by chunking into batches of
    TREE_CHUNK_SIZE. Each chunk uses the previous chunk's tree as base_tree,
    so the final result is a single tree containing all entries layered on top
    of the repo's current state.
    Returns the final tree SHA.
    """
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
        "force": False,
    })
    r.raise_for_status()


def migrate_volume(token: str, volume: str):
    vol_dir = f"volume-{volume}"
    target_repo = f"lra-volume-{volume}"
    print(f"\n{'='*60}")
    print(f"Migrating {vol_dir} → {target_repo}")
    print(f"{'='*60}")

    print("  Fetching monorepo HEAD...")
    mono_commit = get_latest_commit_sha(token, MONOREPO)
    mono_tree_sha = get_tree_sha_for_commit(token, MONOREPO, mono_commit)
    print(f"  Monorepo commit: {mono_commit[:10]}")

    print("  Fetching full monorepo tree (recursive)...")
    mono_tree = get_tree(token, MONOREPO, mono_tree_sha)

    target_files = [
        item for item in mono_tree
        if item["type"] == "blob" and item["path"].startswith(f"{vol_dir}/")
    ]
    print(f"  Found {len(target_files)} files in {vol_dir}/")

    if not target_files:
        print("  WARNING: no files found — skipping")
        return

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
        tree_entries.append({
            "path": item["path"],
            "mode": item.get("mode", "100644"),
            "type": "blob",
            "sha": new_blob_sha,
        })
        time.sleep(0.05)

    print(f"  Building tree in chunks of {TREE_CHUNK_SIZE}...")
    new_tree_sha = create_tree_chunked(token, target_repo, target_tree_sha, tree_entries)

    print("  Creating commit...")
    new_commit_sha = create_commit(
        token, target_repo, new_tree_sha, target_commit,
        f"feat: migrate {vol_dir} content from Learning-Real-Analysis monorepo"
    )
    print("  Updating main branch...")
    update_ref(token, target_repo, new_commit_sha)
    print(f"  ✓ Done! {len(target_files)} files in {target_repo}")


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

    target_repo = "lra-common"
    target_commit = get_latest_commit_sha(token, target_repo)
    target_tree_sha = get_tree_sha_for_commit(token, target_repo, target_commit)

    tree_entries = []
    for i, item in enumerate(target_files, 1):
        if i % 10 == 0 or i == 1 or i == len(target_files):
            print(f"    [{i}/{len(target_files)}] {item['path']}")
        content = get_blob_content(token, MONOREPO, item["sha"])
        new_blob_sha = create_blob(token, target_repo, content)
        tree_entries.append({
            "path": item["path"],
            "mode": item.get("mode", "100644"),
            "type": "blob",
            "sha": new_blob_sha,
        })
        time.sleep(0.05)

    print("  Building tree...")
    new_tree_sha = create_tree_chunked(token, target_repo, target_tree_sha, tree_entries)
    new_commit_sha = create_commit(
        token, target_repo, new_tree_sha, target_commit,
        "chore: sync common/ and bibliography/ from Learning-Real-Analysis monorepo"
    )
    update_ref(token, target_repo, new_commit_sha)
    print(f"  ✓ Done! {len(target_files)} files in lra-common")


def main():
    parser = argparse.ArgumentParser(
        description="Migrate LRA volume content from monorepo to split repos (v2)"
    )
    parser.add_argument("--token", required=True, help="GitHub PAT with repo scope")
    parser.add_argument("--volumes", nargs="+", default=VOLUMES, choices=VOLUMES,
                        help="Volumes to migrate (default: all)")
    parser.add_argument("--skip-common", action="store_true",
                        help="Skip syncing common/ to lra-common")
    args = parser.parse_args()

    print("LRA Volume Migration (v2)")
    print(f"Owner:   {OWNER}")
    print(f"Source:  {MONOREPO}")
    print(f"Volumes: {args.volumes}")
    print()

    if not args.skip_common:
        sync_common_to_lra_common(args.token)

    for vol in args.volumes:
        migrate_volume(args.token, vol)

    print(f"\n{'='*60}")
    print("Migration complete!")
    print(f"{'='*60}")
    print()
    print("Next steps:")
    print("  1. Add SYNC_PAT secret to lra-common → Settings → Secrets → Actions")
    print("  2. Link each lra-volume-N to Overleaf via Menu → GitHub")
    print("  3. Set Main Document to main.tex in Overleaf")
    print()


if __name__ == "__main__":
    main()
