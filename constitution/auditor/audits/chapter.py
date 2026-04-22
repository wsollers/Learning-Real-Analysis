"""
audits/chapter.py
Orchestrates a full chapter audit:
  1. Reads chapter.yaml for the manifest.
  2. Audits every environment listed.
  3. Audits every proof file listed.
  4. Writes a chapter-level summary report.

Does not run the symbol audit (that is a separate explicit command).
"""

import traceback
from pathlib import Path
from time import perf_counter

import yaml

from auditor import client
from auditor.audits.statement import audit_statement_from_yaml_entry
from auditor.audits.proof import audit_proof_from_yaml_entry
from auditor.report import (
    _human_timestamp,
    _overall_result,
)
from auditor.run_state import (
    create_run,
    finalize_run,
    item_filename_prefix,
    load_item_report,
    load_run,
    mark_item_done,
    mark_item_error,
    mark_item_started,
    run_dir,
    save_run,
    write_item_json,
    write_summary,
)


# ---------------------------------------------------------------------------
# Chapter summary report
# ---------------------------------------------------------------------------

def _chapter_summary_markdown(
    chapter: str,
    env_reports: list[dict],
    proof_reports: list[dict],
) -> str:
    from datetime import datetime

    lines = [
        "# Chapter Audit Summary",
        f"- **Chapter:** {chapter}",
        f"- **Timestamp:** {_human_timestamp()}",
        "",
    ]

    total_violations = 0
    total_flags = 0

    # Environment results table
    lines += ["## Environment Audits", "", "| Label | Type | Result |", "|-------|------|--------|"]
    for r in env_reports:
        label  = r.get("label", "?")
        atype  = r.get("artifact_type", "?")
        result = _overall_result(r)
        s = r.get("summary", {})
        hard = (
            s.get("failed", 0)
            + s.get("noncompliant", 0)
            + s.get("conditional_violation", 0)
            + s.get("dependent_violation", 0)
            + s.get("forbidden_violation", 0)
        )
        total_violations += hard
        total_flags += len(r.get("special_flags", []))
        lines.append(f"| `{label}` | {atype} | {result} |")

    lines.append("")

    # Proof results table
    lines += ["## Proof File Audits", "", "| Label | Result |", "|-------|--------|"]
    for r in proof_reports:
        label  = r.get("label", "?")
        result = _overall_result(r)
        s = r.get("summary", {})
        hard = (
            s.get("failed", 0)
            + s.get("noncompliant", 0)
            + s.get("conditional_violation", 0)
            + s.get("dependent_violation", 0)
            + s.get("forbidden_violation", 0)
        )
        total_violations += hard
        total_flags += len(r.get("special_flags", []))
        lines.append(f"| `{label}` | {result} |")

    lines.append("")

    # Overall
    if total_violations == 0 and total_flags == 0:
        overall = "CHAPTER CLEAN"
    elif total_violations == 0:
        overall = f"PASS WITH FLAGS - {total_flags} flags across chapter"
    else:
        overall = f"CHAPTER FAIL - {total_violations} violations, {total_flags} flags"

    lines += [
        "## Overall",
        "",
        overall,
        "",
        "_Individual report files contain full check tables and violation details._",
    ]

    return "\n".join(lines)


def _item_result(report: dict) -> str:
    return _overall_result(report)


def _item_report_path(report: dict) -> str:
    return report.get("_report_path", "")


def _error_report(item: dict, message: str) -> dict:
    audit_type = "proof" if item.get("kind") == "proof" else "statement"
    return {
        "label": item.get("label", "?"),
        "artifact_type": item.get("artifact_type", "proof" if audit_type == "proof" else "?"),
        "audit_type": audit_type,
        "summary": {"failed": 1},
        "checks": [],
        "violations": [{"block_id": "ERROR", "status": "FAIL", "finding": message}],
        "special_flags": [],
    }


def _append_report(
    item: dict,
    report: dict,
    env_reports: list[dict],
    proof_reports: list[dict],
) -> None:
    if item.get("kind") == "proof":
        proof_reports.append(report)
    else:
        env_reports.append(report)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def audit_chapter(chapter_path: Path, resume: Path | str | None = None) -> None:
    """
    Runs a full structural audit of all environments and proof files
    listed in chapter.yaml.

    Writes individual reports for each item and a chapter summary report.
    Prints summary to terminal.
    """
    if resume:
        state = load_run(resume)
        chapter_root = (Path(state["repo_root"]) / state["chapter_path"]).resolve()
        chapter      = state["chapter"]
    else:
        chapter_root = chapter_path.resolve()
        chapter      = chapter_root.name

    # Load chapter.yaml
    yaml_path = chapter_root / "chapter.yaml"
    if not yaml_path.exists():
        raise FileNotFoundError(
            f"chapter.yaml not found at {yaml_path}. "
            f"Run `python -m auditor scan chapter {chapter_path}` first."
        )

    with open(yaml_path, encoding="utf-8") as f:
        chapter_yaml = yaml.safe_load(f)

    environments = chapter_yaml.get("environments", [])
    proof_files  = chapter_yaml.get("proof_files", [])

    if not environments and not proof_files:
        print(
            f"chapter.yaml for '{chapter}' has no environments or proof_files listed.\n"
            f"Run `python -m auditor scan chapter {chapter_path}` to populate it."
        )
        return

    settings = client.settings()
    if resume:
        state["status"] = "running"
        save_run(state)
        print(f"\nResuming audit run {state['run_id']}")
    else:
        state = create_run(
            chapter_path=chapter_root,
            environments=environments,
            proof_files=proof_files,
            provider=settings.provider,
            model=settings.model,
        )
        print(f"\nCreated audit run {state['run_id']}")

    print(f"Run directory: {run_dir(state)}")
    print(
        "Resume command: "
        f"python -m constitution.auditor --repoDir {state['repo_root']} "
        f"-ai {settings.provider.lower()} audit chapter {state['chapter_path']} "
        f"--resume {run_dir(state)}"
    )

    env_reports: list[dict] = []
    proof_reports: list[dict] = []
    total_items = len(state["items"])
    completed = 0
    run_started = perf_counter()

    env_total = sum(1 for item in state["items"] if item["kind"] == "statement")
    proof_total = sum(1 for item in state["items"] if item["kind"] == "proof")
    print(f"\nAuditing {env_total} environment(s) and {proof_total} proof file(s) in '{chapter}'...")

    for item in state["items"]:
        label = item.get("label", "?")
        kind = item["kind"]
        kind_label = "env" if kind == "statement" else "proof"
        entry = item["entry"]

        if item.get("status") == "done":
            report = load_item_report(state, item)
            if report is not None:
                completed += 1
                _append_report(item, report, env_reports, proof_reports)
                print(
                    f"[{item['kind_index']}/{item['kind_total']} {kind_label} | "
                    f"{completed}/{total_items} total] SKIP {label}: already done",
                    flush=True,
                )
                continue

            item["status"] = "pending"
            item["last_error"] = "Completed item had no readable report JSON; rerunning."
            save_run(state)

        started = perf_counter()
        print(
            f"[{item['kind_index']}/{item['kind_total']} {kind_label} | "
            f"{completed}/{total_items} total] START {label}",
            flush=True,
        )
        mark_item_started(state, item)
        try:
            output_dir = run_dir(state) / kind
            filename_prefix = item_filename_prefix(item)
            if kind == "statement":
                report = audit_statement_from_yaml_entry(
                    entry,
                    chapter_root,
                    chapter,
                    print_report=False,
                    output_dir=output_dir,
                    filename_prefix=filename_prefix,
                )
            else:
                report = audit_proof_from_yaml_entry(
                    entry,
                    chapter_root,
                    chapter,
                    print_report=False,
                    output_dir=output_dir,
                    filename_prefix=filename_prefix,
                )

            report_json = write_item_json(state, item, report)
            report_md = Path(_item_report_path(report)) if _item_report_path(report) else None
            mark_item_done(
                state,
                item,
                report_json=report_json,
                report_markdown=report_md,
            )
            _append_report(item, report, env_reports, proof_reports)
            elapsed = perf_counter() - started
            item_report_path = _item_report_path(report)
            path_note = f" -> {item_report_path}" if item_report_path else ""
            completed += 1
            print(
                f"[{item['kind_index']}/{item['kind_total']} {kind_label}] "
                f"DONE {label}: {_item_result(report)} in {elapsed:.1f}s{path_note}",
                flush=True,
            )
        except Exception as e:
            elapsed = perf_counter() - started
            error_path = mark_item_error(
                state,
                item,
                message=str(e),
                traceback_text=traceback.format_exc(),
            )
            _append_report(item, _error_report(item, str(e)), env_reports, proof_reports)
            completed += 1
            print(
                f"[{item['kind_index']}/{item['kind_total']} {kind_label}] "
                f"ERROR {label} after {elapsed:.1f}s: {e}",
                flush=True,
            )
            print(f"  Error details written to: {error_path}", flush=True)

    # --- Chapter summary ---
    summary_md = _chapter_summary_markdown(chapter, env_reports, proof_reports)
    summary_path = write_summary(state, summary_md)
    errored = [item for item in state["items"] if item.get("status") == "error"]
    if errored:
        state["status"] = "error"
        save_run(state)
        final_path = None
    else:
        final_path = finalize_run(state)
    elapsed_total = perf_counter() - run_started

    print("\n" + summary_md)
    print(f"\nChapter summary written to: {summary_path}")
    if final_path:
        print(f"Final report directory written to: {final_path}")
    else:
        print(f"Run has {len(errored)} execution error(s); resume from: {run_dir(state)}")
    print(f"Audited {completed}/{total_items} item(s) in {elapsed_total:.1f}s")
