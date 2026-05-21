#!/usr/bin/env python3
"""
migrate_volumes.py
------------------
Migrates all volume content from Learning-Real-Analysis (monorepo) into
the corresponding lra-volume-N repos via the GitHub REST API.

Usage:
    pip install requests
    python3 migrate_volumes.py --token ghp_YOURTOKEN

    # Single volume only:
    python3 migrate_volumes.py --token ghp_YOURTOKEN --volumes i

    # Skip syncing common/ to lra-common (if already done):
    python3 migrate_volumes.py --token ghp_YOURTOKEN --skip-common

What it does:
    1. Reads every file recursively from volume-N/ in the monorepo
       using the Git Trees API (single call per volume, recursive=1)
    2. Fetches file content (blob) for each file
    3. Pushes all files to the corresponding lra-volume-N repo in a
       single commit via the Git Data API
    4. Also syncs common/ and bibliography/ to lra-common

This is safe to re-run: it creates a new commit with current content.

Requirements:
    pip install requests
"""

import argparse
import base64
import sys
import time
import requests


OWNER = "wsollers"
MONOREPO = "Learning-Real-Analysis"
BRANCH = "main"

VOLUMES = ["i", "ii", "iii", "iv", "v"]


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
        print(f"  Rate limited \u2014 sleeping {wait:.0f}s")
        time.sleep(wait)
        r = requests.request(method, url, headers=headers, **kwargs)
    return r


def get_tree(token: str, repo: str, sha: str) -> list[dict]:
    """Get full recursive tree for a given tree SHA."""
    r = gh(token, "GET", f"/repos/{OWNER}/{repo}/git/trees/{sha}?recursive=1")
    r.raise_for_status()
    data = r.json()
    if data.get("truncated"):
        print(f"  WARNING: tree was truncated (>100k files). Some files may be missing.")
    return data["tree"]


def get_blob_content(token: str, repo: str, sha: str) -> bytes:
    """Fetch raw blob bytes by SHA."""
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
    """Upload a blob and return its SHA."""
    encoded = base64.b64encode(content_bytes).decode()
    r = gh(token, "POST", f"/repos/{OWNER}/{repo}/git/blobs", json={
        "content": encoded,
        "encoding": "base64",
    })
    r.raise_for_status()
    return r.json()["sha"]


def create_tree(token: str, repo: str, base_tree_sha: str, entries: list[dict]) -> str:
    """Create a new tree on top of base_tree_sha and return new tree SHA."""
    r = gh(token, "POST", f"/repos/{OWNER}/{repo}/git/trees", json={
        "base_tree": base_tree_sha,
        "tree": entries,
    })
    r.raise_for_status()
    return r.json()["sha"]


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
    print(f"Migrating {vol_dir} \u2192 {target_repo}")
    print(f"{'='*60}")

    # Get monorepo HEAD tree (recursive)
    print("  Fetching monorepo HEAD...")
    mono_commit = get_latest_commit_sha(token, MONOREPO)
    mono_tree_sha = get_tree_sha_for_commit(token, MONOREPO, mono_commit)
    print(f"  Monorepo commit: {mono_commit[:10]}")

    print("  Fetching full monorepo tree (recursive)...")
    mono_tree = get_tree(token, MONOREPO, mono_tree_sha)

    # Filter to just the files we want: volume-N/**
    target_files = [
        item for item in mono_tree
        if item["type"] == "blob" and item["path"].startswith(f"{vol_dir}/")
    ]

    print(f"  Found {len(target_files)} files in {vol_dir}/")

    if not target_files:
        print(f"  WARNING: No files found in {vol_dir}/ \u2014 skipping")
        return

    # Get target repo HEAD
    print(f"  Fetching {target_repo} HEAD...")
    target_commit = get_latest_commit_sha(token, target_repo)
    target_tree_sha = get_tree_sha_for_commit(token, target_repo, target_commit)
    print(f"  Target commit: {target_commit[:10]}")

    # Upload blobs to target repo and build tree entries
    print(f"  Uploading {len(target_files)} blobs to {target_repo}...")
    tree_entries = []
    for i, item in enumerate(target_files, 1):
        src_path = item["path"]
        # Keep path identical — volume-i/index.tex stays at volume-i/index.tex
        dest_path = src_path

        if i % 25 == 0 or i == 1 or i == len(target_files):
            print(f"    [{i}/{len(target_files)}] {src_path}")

        content = get_blob_content(token, MONOREPO, item["sha"])
        new_blob_sha = create_blob(token, target_repo, content)

        tree_entries.append({
            "path": dest_path,
            "mode": item.get("mode", "100644"),
            "type": "blob",
            "sha": new_blob_sha,
        })

        time.sleep(0.05)  # gentle rate limiting

    # Create new tree, commit, update ref
    print(f"  Creating new tree in {target_repo}...")
    new_tree_sha = create_tree(token, target_repo, target_tree_sha, tree_entries)

    print(f"  Creating commit...")
    new_commit_sha = create_commit(
        token, target_repo, new_tree_sha, target_commit,
        f"feat: migrate {vol_dir} content from Learning-Real-Analysis monorepo"
    )

    print(f"  Updating main branch...")
    update_ref(token, target_repo, new_commit_sha)

    print(f"  \u2713 Done! {len(target_files)} files committed to {target_repo}")


def sync_common_to_lra_common(token: str):
    """Sync common/ and bibliography/ from monorepo to lra-common."""
    print(f"\n{'='*60}")
    print(f"Syncing common/ + bibliography/ \u2192 lra-common")
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

    print(f"  Found {len(target_files)} files in common/ + bibliography/")

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

    new_tree_sha = create_tree(token, target_repo, target_tree_sha, tree_entries)
    new_commit_sha = create_commit(
        token, target_repo, new_tree_sha, target_commit,
        "chore: sync common/ and bibliography/ from Learning-Real-Analysis monorepo"
    )
    update_ref(token, target_repo, new_commit_sha)
    print(f"  \u2713 Done! {len(target_files)} files committed to lra-common")


def main():
    parser = argparse.ArgumentParser(
        description="Migrate LRA volume content from monorepo to split repos"
    )
    parser.add_argument("--token", required=True,
                        help="GitHub PAT with repo scope (github.com/settings/tokens)")
    parser.add_argument("--volumes", nargs="+", default=VOLUMES,
                        choices=VOLUMES,
                        help="Volumes to migrate (default: all)")
    parser.add_argument("--skip-common", action="store_true",
                        help="Skip syncing common/ to lra-common")
    args = parser.parse_args()

    print(f"LRA Volume Migration")
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
    print("  1. Add SYNC_PAT secret to lra-common \u2192 Settings \u2192 Secrets \u2192 Actions")
    print("  2. Link each lra-volume-N to Overleaf via Menu \u2192 GitHub")
    print("  3. In Overleaf, set Main Document to main.tex")
    print()


if __name__ == "__main__":
    main()
