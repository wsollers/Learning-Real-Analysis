"""
run_state.py
Resumable chapter-audit run state.

Chapter audits are the expensive path: they call an AI provider once per
statement/proof item. This module records progress after every item so an
interrupted run can resume without re-running completed calls.
"""

from __future__ import annotations

import json
import shutil
import uuid
from datetime import datetime
from pathlib import Path
from typing import Any

from auditor import config
from auditor.report import _timestamp


def _now() -> str:
    return datetime.now().isoformat(timespec="seconds")


def _run_id() -> str:
    return f"{datetime.now().strftime('%Y%m%d-%H%M%S')}-{uuid.uuid4().hex[:8]}"


def _safe_label(label: str) -> str:
    return label.replace(":", "-").replace("/", "-").replace("\\", "-")


def _chapter_path_for_state(chapter_path: Path) -> str:
    resolved = chapter_path.resolve()
    try:
        return str(resolved.relative_to(config.REPO_ROOT))
    except ValueError:
        return str(resolved)


def runs_root() -> Path:
    return config.REPORTS_DIR / "runs"


def run_dir(state: dict[str, Any]) -> Path:
    return Path(state["run_dir"])


def run_json_path(state: dict[str, Any]) -> Path:
    return run_dir(state) / "run.json"


def create_run(
    *,
    chapter_path: Path,
    environments: list[dict[str, Any]],
    proof_files: list[dict[str, Any]],
    provider: str,
    model: str,
) -> dict[str, Any]:
    run_id = _run_id()
    chapter = chapter_path.resolve().name
    directory = runs_root() / run_id
    directory.mkdir(parents=True, exist_ok=False)

    items: list[dict[str, Any]] = []
    total = len(environments) + len(proof_files)

    for index, entry in enumerate(environments, start=1):
        label = entry.get("label", f"statement-{index}")
        items.append({
            "index": index,
            "total": total,
            "kind": "statement",
            "kind_index": index,
            "kind_total": len(environments),
            "label": label,
            "artifact_type": entry.get("type", "?"),
            "entry": entry,
            "status": "pending",
            "attempts": 0,
            "started_at": None,
            "finished_at": None,
            "report_markdown": None,
            "report_json": None,
            "error_json": None,
            "last_error": None,
        })

    proof_offset = len(environments)
    for proof_index, entry in enumerate(proof_files, start=1):
        label = entry.get("label", f"proof-{proof_index}")
        items.append({
            "index": proof_offset + proof_index,
            "total": total,
            "kind": "proof",
            "kind_index": proof_index,
            "kind_total": len(proof_files),
            "label": label,
            "artifact_type": "proof",
            "entry": entry,
            "status": "pending",
            "attempts": 0,
            "started_at": None,
            "finished_at": None,
            "report_markdown": None,
            "report_json": None,
            "error_json": None,
            "last_error": None,
        })

    state = {
        "run_id": run_id,
        "status": "running",
        "created_at": _now(),
        "updated_at": _now(),
        "completed_at": None,
        "repo_root": str(config.REPO_ROOT),
        "chapter": chapter,
        "chapter_path": _chapter_path_for_state(chapter_path),
        "provider": provider,
        "model": model,
        "run_dir": str(directory),
        "final_dir": None,
        "items": items,
    }
    save_run(state)
    return state


def resolve_resume_path(resume: str | Path) -> Path:
    path = Path(resume)
    if str(resume).lower() == "latest":
        candidates = sorted(
            (p for p in runs_root().glob("*/run.json") if p.is_file()),
            key=lambda p: p.stat().st_mtime,
            reverse=True,
        )
        if not candidates:
            raise FileNotFoundError(f"No resumable runs found under {runs_root()}")
        return candidates[0]
    if path.is_dir():
        path = path / "run.json"
    if not path.is_absolute():
        path = (config.REPO_ROOT / path).resolve()
    return path


def load_run(resume: str | Path) -> dict[str, Any]:
    path = resolve_resume_path(resume)
    if not path.exists():
        raise FileNotFoundError(f"Run state not found: {path}")
    state = json.loads(path.read_text(encoding="utf-8"))
    state["run_dir"] = str(path.parent)
    return state


def save_run(state: dict[str, Any]) -> Path:
    state["updated_at"] = _now()
    path = run_json_path(state)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(state, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    return path


def item_filename_prefix(item: dict[str, Any]) -> str:
    return f"{item['index']:03d}_{_safe_label(item.get('label', 'unknown'))}"


def mark_item_started(state: dict[str, Any], item: dict[str, Any]) -> None:
    item["status"] = "started"
    item["attempts"] = int(item.get("attempts") or 0) + 1
    item["started_at"] = _now()
    item["finished_at"] = None
    item["last_error"] = None
    save_run(state)


def write_item_json(
    state: dict[str, Any],
    item: dict[str, Any],
    report: dict[str, Any],
) -> Path:
    directory = run_dir(state) / item["kind"]
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f"{item_filename_prefix(item)}.json"
    path.write_text(
        json.dumps(report, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    return path


def load_item_report(state: dict[str, Any], item: dict[str, Any]) -> dict[str, Any] | None:
    report_json = item.get("report_json")
    if not report_json:
        return None
    path = Path(report_json)
    if not path.is_absolute():
        path = run_dir(state) / path
    if not path.exists():
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def mark_item_done(
    state: dict[str, Any],
    item: dict[str, Any],
    *,
    report_json: Path,
    report_markdown: Path | None,
) -> None:
    item["status"] = "done"
    item["finished_at"] = _now()
    item["report_json"] = str(report_json)
    item["report_markdown"] = str(report_markdown) if report_markdown else None
    item["error_json"] = None
    item["last_error"] = None
    save_run(state)


def mark_item_error(
    state: dict[str, Any],
    item: dict[str, Any],
    *,
    message: str,
    traceback_text: str | None = None,
) -> Path:
    directory = run_dir(state) / "errors"
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f"{item_filename_prefix(item)}.error.json"
    payload = {
        "run_id": state["run_id"],
        "item": {
            "index": item.get("index"),
            "kind": item.get("kind"),
            "label": item.get("label"),
            "artifact_type": item.get("artifact_type"),
        },
        "message": message,
        "traceback": traceback_text,
        "timestamp": _now(),
    }
    path.write_text(
        json.dumps(payload, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    item["status"] = "error"
    item["finished_at"] = _now()
    item["error_json"] = str(path)
    item["last_error"] = message
    save_run(state)
    return path


def write_summary(state: dict[str, Any], summary_markdown: str) -> Path:
    path = run_dir(state) / "summary.md"
    path.write_text(summary_markdown, encoding="utf-8")
    save_run(state)
    return path


def finalize_run(state: dict[str, Any]) -> Path:
    """
    Copies the run directory to the chapter's final timestamped report folder.
    The live run directory remains available for audit trail/resume inspection.
    """
    chapter = state["chapter"]
    final_dir = (
        config.REPORTS_DIR
        / chapter
        / f"{_timestamp()}_audit-chapter_{chapter}"
    )
    counter = 2
    while final_dir.exists():
        final_dir = (
            config.REPORTS_DIR
            / chapter
            / f"{_timestamp()}_audit-chapter_{chapter}_{counter}"
        )
        counter += 1

    state["status"] = "complete"
    state["completed_at"] = _now()
    state["final_dir"] = str(final_dir)
    save_run(state)
    shutil.copytree(run_dir(state), final_dir)
    return final_dir
