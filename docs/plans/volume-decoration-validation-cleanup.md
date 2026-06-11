# Volume Decoration Validation Cleanup

Baseline recorded: 2026-06-11.

Latest baseline refresh: 2026-06-11, after excluding top-level figures,
migrating hand-rolled toolkit/formal `tcolorbox` chrome to shared macros, and
starring note-body subsection headings, and adding mechanical proof-navigation
links. `\NoLocalDependencies` is now an allowed top-level marker, and
post-toolkit `\vspace{1em}` spacing has been moved into `toolkitbox`.
Legacy proof-layer text headings have been normalized to titled `proof`
environments with `\LRAProofBodyStart`.

This plan tracks the repo-wide cleanup needed before strict decoration
validation can become a normal success gate for every `lra-volume-*` repo.

## Success Gates

For each volume repo:

```powershell
python F:\repos\lra-governance\tools\governance\validate_decoration.py --root <volume-repo> --fail-on-errors
latexmk -lualatex main.tex -interaction=nonstopmode -halt-on-error
```

The cleanup is complete when both commands pass for all eight volume repos.
Generated proof stubs must also keep the current proof-environment shape:

- old text-heading TODO stubs: `0`;
- new proof-environment TODO stubs are allowed until populated.

## Current Baseline

Files scanned: `1878`.

Validation records:

| Severity | Count |
| --- | ---: |
| error | 5079 |
| warning | 1175 |
| review | 33 |
| total | 6287 |

By volume:

| Repo | Files | Total | Errors | Warnings | Review |
| --- | ---: | ---: | ---: | ---: | ---: |
| `lra-volume-i` | 329 | 1510 | 1322 | 188 | 0 |
| `lra-volume-ii` | 488 | 1458 | 1117 | 332 | 9 |
| `lra-volume-iii` | 549 | 1365 | 1108 | 252 | 5 |
| `lra-volume-iv` | 201 | 1198 | 964 | 233 | 1 |
| `lra-volume-v` | 55 | 418 | 312 | 96 | 10 |
| `lra-volume-vi` | 178 | 229 | 161 | 62 | 6 |
| `lra-volume-vii` | 24 | 62 | 52 | 9 | 1 |
| `lra-volume-viii` | 54 | 47 | 43 | 3 | 1 |

Top validation codes:

| Code | Count |
| --- | ---: |
| `unexpected_top_level_environment` | 1379 |
| `top_level_prose` | 1181 |
| `plain_remark_or_example` | 680 |
| `unwrapped_math_env` | 504 |
| `missing_interpretation` | 374 |
| `unknown_decoration_block` | 342 |
| `missing_dependencies` | 297 |
| `missing_standard_quantified_statement` | 294 |
| `weak_label_slug` | 186 |
| `formal_reading_missing` | 154 |
| `raw_tcolorbox_wrapper` | 141 |
| `missing_section_router_heading` | 108 |

## Current Stub State

The old generated proof-stub shape has been removed from active volume content.
Archives and exercises were intentionally skipped.

| Repo | Old-shape TODO stubs | New-shape TODO stubs |
| --- | ---: | ---: |
| `lra-volume-i` | 0 | 112 |
| `lra-volume-ii` | 0 | 142 |
| `lra-volume-iii` | 0 | 227 |
| `lra-volume-iv` | 0 | 17 |
| `lra-volume-v` | 0 | 4 |
| `lra-volume-vi` | 0 | 4 |
| `lra-volume-vii` | 0 | 0 |
| `lra-volume-viii` | 0 | 1 |
| total | 0 | 507 |

The new migration path is the supported path:

- `tools/migration/create_missing_proofs.py`;
- `tools/migration/identify_missing_proofs.py`.

Do not patch the retired proof generator to fix generated stubs. If stubs are
wrong, fix the new migration tool, delete generated stubs, and regenerate them.

## Cleanup Order

1. Mechanical wrapper cleanup:
   - `unexpected_top_level_environment`;
   - `top_level_prose`;
   - `plain_remark_or_example`;
   - `raw_tcolorbox_wrapper`;
   - `unwrapped_math_env`.

2. Required semantic blocks:
   - `missing_interpretation`;
   - `missing_dependencies`;
   - `missing_standard_quantified_statement`;
   - `formal_reading_missing`.

3. Routing and labels:
   - `missing_section_router_heading`;
   - `missing_label`;
   - `weak_label_slug`;
   - `ocr_like_label`;
   - `wrong_label_prefix`.

4. Dependency hygiene:
   - `invalid_dependency_target`;
   - `dependency_without_hyperref`;
   - `missing_dependent_parent_block`.

5. Chapter and breadcrumb cleanup:
   - `breadcrumb_missing`;
   - `breadcrumb_hand_rolled`;
   - `breadcrumb_misplaced`;
   - `missing_chapter_heading`;
   - `missing_chapter_label`.

6. Review-only legacy content:
   - `structural_roadmap_present`;
   - `structural_role_present`.

## Generator Versus Authored Cleanup

Fix generators when a repeated generated shape is invalid or when regenerated
stubs fail to build or validate.

Fix authored content when the problem is local mathematical prose, old note
layout, hand-authored theorem decoration, labels, dependency links, or chapter
organization.

Every cleanup batch should end with:

1. the relevant validator command;
2. the target volume build;
3. an updated count summary if repo-wide counts materially changed.
