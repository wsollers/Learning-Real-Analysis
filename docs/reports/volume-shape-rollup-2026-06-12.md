# Volume Shape Rollup

Date: 2026-06-12

This report captures the first cross-volume run of the unified validator:

```powershell
python F:\repos\lra-governance\tools\governance\validate_volume.py <volume-repo> --fail-on-errors
```

Every volume currently stops at the `volume_shape` fail-fast gate, so downstream
validators are not yet the bottleneck.

Update: `missing_matching_proofs_topic` now applies only when a note topic
contains a proof-bearing theorem-like environment. This removed false positives
for pure exposition/reference topics and lowered the total shape-gate count from
195 to 135.

## Global Totals

| Code | Count |
|---|---:|
| `missing_volume_shape_file` | 47 |
| `orphan_proofs_topic` | 31 |
| `legacy_chapter_path` | 23 |
| `flat_note_body` | 18 |
| `noncanonical_exercises_path` | 13 |
| `missing_matching_proofs_topic` | 3 |
| **Total** | **135** |

## Volume Totals

| Volume | Total | Shape codes |
|---|---:|---|
| `lra-volume-i` | 12 | `legacy_chapter_path=5`; `missing_matching_proofs_topic=1`; `noncanonical_exercises_path=3`; `orphan_proofs_topic=3` |
| `lra-volume-ii` | 18 | `flat_note_body=6`; `legacy_chapter_path=1`; `missing_volume_shape_file=4`; `noncanonical_exercises_path=3`; `orphan_proofs_topic=4` |
| `lra-volume-iii` | 35 | `flat_note_body=3`; `legacy_chapter_path=5`; `missing_volume_shape_file=18`; `orphan_proofs_topic=9` |
| `lra-volume-iv` | 31 | `flat_note_body=8`; `legacy_chapter_path=4`; `missing_matching_proofs_topic=2`; `missing_volume_shape_file=7`; `noncanonical_exercises_path=5`; `orphan_proofs_topic=5` |
| `lra-volume-v` | 16 | `legacy_chapter_path=2`; `missing_volume_shape_file=10`; `orphan_proofs_topic=4` |
| `lra-volume-vi` | 13 | `flat_note_body=1`; `legacy_chapter_path=3`; `missing_volume_shape_file=3`; `noncanonical_exercises_path=2`; `orphan_proofs_topic=4` |
| `lra-volume-vii` | 1 | `legacy_chapter_path=1` |
| `lra-volume-viii` | 9 | `legacy_chapter_path=2`; `missing_volume_shape_file=5`; `orphan_proofs_topic=2` |

## Remediation Order

1. Create missing proof topic routers only for note topics that contain
   proof-bearing theorem-like environments. Pure exposition/reference topics no
   longer require empty proof mirrors.
2. Remove or migrate legacy `chapter/exercises/` and `chapter/capstone.tex`
   paths into `proofs/exercises/`.
3. Move flat note bodies under `notes/{topic}/` and update parent routers.
4. Resolve `proofs/notes` and other orphan proof topics by renaming, moving, or
   deleting archived routing remnants.
5. Remove `CANDIDATES.md` and exercise proof files from active
   `proofs/exercises/`, or move them to a non-active planning/report location.
6. Re-run `validate_volume.py` after each volume reaches shape parity so
   downstream validators can run.

## Lowest-Error Pilot

`lra-volume-vii` is the smallest remediation target at 1 shape error:

- `legacy_chapter_path=1`

It is the best candidate for the first full pass through shape cleanup and
downstream validator discovery.
