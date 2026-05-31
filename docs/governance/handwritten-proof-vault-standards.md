# Handwritten Proof Vault Standards

These standards apply to handwritten proof artifacts associated with the
Learning Real Analysis project.

## Purpose

The handwritten proof vault stores personal learning artifacts, proof attempts,
reviewed handwritten proofs, extracted proof records, and related metadata.

The proof vault is not the canonical source of mathematical content. Canonical
theorem statements and canonical proofs remain in the LRA volume repositories.

The proof vault repository is private by default because handwritten images may
contain unintended personal, device, location, or author information.

## Repository

The dedicated proof vault repository is:

```text
wsollers/lra-proof-vault
```

The local path is expected to be:

```text
F:\repos\lra-proof-vault
```

The repository must remain private unless a later task explicitly requests and
approves a visibility change.

## Repository Structure

The root structure is:

```text
lra-proof-vault/
  README.md
  index.json
  theorem-map.yaml
  metadata-template.yaml
  inbox/
  reviewed/
  extracted/
  rejected/
  volume-i/
  volume-ii/
  volume-iii/
  volume-iv/
```

Future memorialized proofs should live under the relevant volume, chapter, and
theorem directory:

```text
volume-i/
  chapter-01-propositional-logic/
    thm-unique-readability/
      proof-2026-05-31-001.jpg
      proof-2026-05-31-001.md
      metadata.yaml
```

Use lowercase, hyphen-separated, ASCII paths. Do not store raw mobile images in
the repository.

## Metadata Requirements

Each memorialized proof record should include a `metadata.yaml` file using the
current template fields:

```yaml
volume:
chapter:
chapter_slug:

theorem_label:
theorem_title:

proof_file:
image_file:

review_status:
reviewed_with_chatgpt:
review_date:
created_date:

canonical_repo:
canonical_path:

github_url:

notes_file:
```

Allowed metadata includes:

- source filename;
- review date;
- theorem label;
- review status.

Forbidden embedded image metadata includes:

- GPS coordinates;
- camera serial numbers;
- device identifiers;
- author metadata;
- phone model metadata;
- embedded thumbnails;
- EXIF timestamps.

Forbidden embedded metadata must be removed before an image enters git.

## Review Statuses

Allowed review statuses are:

- `inbox`;
- `needs-review`;
- `reviewed-correct`;
- `reviewed-needs-revision`;
- `extracted`;
- `rejected`.

Do not invent additional statuses without updating this standard and the proof
vault index.

## EXIF Stripping

EXIF stripping is mandatory.

Raw mobile images must never be committed. Only sanitized copies may enter git.

Required workflow:

```text
incoming image
       |
       v
staging area
       |
       v
metadata stripping
       |
       v
sanitized image
       |
       v
git commit
```

Acceptable sanitization methods include:

- `exiftool -all=`;
- ImageMagick `-strip`;
- Python Pillow re-save without metadata.

If sanitization cannot be verified, stop and report the issue instead of
committing the image.

## Future Memorialization Workflow

The future command:

```text
Memorialize proof image
```

should perform the following steps:

1. Store the incoming image in a staging area outside tracked git content.
2. Sanitize the image by removing embedded metadata.
3. Create proof-vault metadata.
4. Create a markdown record for the proof artifact.
5. Commit the proof vault repository.
6. Push the proof vault repository.
7. Add a `\ProofVaultURL{...}` backlink to the canonical theorem proof file.
8. Commit the canonical repository.
9. Push the canonical repository.

No raw image may be committed at any stage of this workflow.

## Backlink Rules

Backlinks from canonical proof files to proof-vault records are required for
proofs created from memorialized handwritten proof images.

The backlink must use the shared macro:

```latex
\ProofVaultURL{https://github.com/wsollers/lra-proof-vault/tree/master/path/to/sanitized-record}
```

Place the macro immediately after the `Return` remark and before the
unnumbered theorem restatement:

```latex
\begin{remark*}[Return]
\hyperref[lem:example]{Return to Lemma}
\end{remark*}

\ProofVaultURL{https://github.com/wsollers/lra-proof-vault/tree/master/...}

\begin{theorem*}[Example]
...
\end{theorem*}
```

The macro is extraction-visible. Knowledge extraction and theorem-explorer
pipelines should treat the argument of `\ProofVaultURL{...}` as the
proof-vault record URL for the owning proof label.

Backlinks must satisfy the following rules:

- it must point to a sanitized proof-vault record, not to a raw image;
- it must not replace the canonical proof text;
- it must not make the handwritten proof the source of truth;
- it must preserve existing labels, dependency blocks, and extraction-visible
  structure.

If a canonical theorem or proof file does not exist, report the missing target
instead of inventing one.

## Canonical Proof Format

When a proof is generated from a handwritten image, the canonical proof file
must still follow `proof-standards.md`.

The extracted proof content must be converted into:

- a professional standard proof;
- a detailed learning proof;
- a proof-structure remark;
- a dependencies block using extraction-visible `\hyperref[...]` targets.

A direct transcription alone is not a complete canonical proof file.
