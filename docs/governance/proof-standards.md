# Proof Standards

Source sections: `DESIGN.md` sections 8, 9, 10.4, 14, and proof lifecycle
rules after section 14.

## Proof Ownership

No proof file may be created for a statement unless the corresponding statement
label already exists in the notes file.

Leaf volume repositories own theorem and proof source files. The
`Learning-Real-Analysis` monorepo is an assembled integration target and must
not be edited as the canonical source for theorem/proof files.

Handwritten proof artifacts, reviewed handwritten proof attempts, and
proof-vault backlinks are governed by `handwritten-proof-vault-standards.md`.
The proof vault is archival support only; canonical proof content remains in
the volume repositories.

## Theorem Proof-Stub Invariant

Every top-level `theorem`, `proposition`, `lemma`, and `corollary` in a leaf
repo `notes/**/*.tex` file that is expected to have a proof must have a
compile-safe proof stub in that leaf repo's `proofs/**/*.tex` tree at creation
time.

Definitions and axioms are stable knowledge nodes, but they are not proof
obligations unless explicitly marked by a local workflow or governance task.
Local claims inside proof files do not create proof-stub obligations.

The proof-to-theorem association is the source-visible macro:

```latex
\LRAProofFor{thm:example-label}
```

`lra-common` owns this macro. It is render-inert and exists for validators and
knowledge extraction. Proof files should not rely on title or path inference as
the primary theorem association.

## Proof File Structure

Proof files live under the owning chapter's `proofs/` tree and use lowercase,
hyphen-separated ASCII filenames. Each notes proof file begins on a new page
and ends with `\clearpage`, so the next proof topic or proof section cannot
start on the same page as the previous proof.

The machine-readable authority for proof folder architecture, topic matching,
legacy refactor mode, and proof index reachability is
`docs/governance/volume-structure.schema.json`, enforced through
`tools/governance/validate_volume.py`. Use
`tools/governance/audit_proof_layout.py` for deterministic proof-layout
classification when a task needs compliant/non-compliant proof-file lists.

A full proof file contains:

1. proof-level label,
2. `\LRAProofFor{...}`,
3. theorem-side navigation link,
4. proof-vault backlink when the proof came from a memorialized handwritten
   proof artifact,
5. unnumbered theorem-like restatement,
6. professional standard proof,
7. detailed learning proof,
8. proof structure remark,
9. dependencies remark,
10. `\clearpage`.

Proof-vault backlinks must use the shared `\ProofVaultURL{...}` macro and
must be placed immediately after the `Return` remark and before the theorem
restatement. Do not use raw `\href` or ad hoc URL text for proof-vault links.
The macro argument must point to the sanitized proof-vault record, not to a raw
or unsanitized image.

## Proof Stub Structure

A proof stub is a compile-safe proof file that preserves the full proof-file
structure. It may replace the professional standard proof and detailed learning
proof bodies with TODO placeholders, but it must still include the proof-level
label, `\LRAProofFor{...}`, theorem restatement, navigation,
proof-structure remark, and dependency block.

Proof stubs are durable canonical containers. Later proof population must
modify the existing proof file in place. It must preserve labels, theorem
navigation, `\LRAProofFor{...}`, proof-vault backlinks, theorem restatement,
dependency remarks, proof-structure remarks, and extraction metadata unless an
explicit refactor task says otherwise.

## Two-Layer Proof Rule

The professional proof is compact and rigorous. The detailed learning proof
teaches the same proof with explicit step structure. Explanationless proof mode
is opt-in only and does not waive notation, label, dependency, or architecture
rules.

Proofs generated from handwritten proof images must still be converted into the
full two-layer proof-file format. The handwritten proof may inform the
professional and detailed layers, but it must not replace the required
structure with a single transcription block.

## Label Rule

Proof labels use `prf:` and align with the theorem label root. Proof files must
not put `\label{...}` inside theorem environments; proof labels sit at proof
file level according to the current compatibility rule.

## Dependency Rule

Dependency remarks record mathematical dependencies, not proof-file
dependencies.

A dependency item must target a mathematical statement label such as `def:`,
`ax:`, `thm:`, `lem:`, `prop:`, or `cor:`.

Dependency items should be human-readable in the PDF and machine-readable for
extraction. The preferred form is:

```latex
\begin{dependencies}
\begin{itemize}
  \item \hyperref[def:supremum]{Supremum}
  \item \hyperref[ax:least-upper-bound-property]{Least Upper Bound Property}
  \item \hyperref[thm:epsilon-characterization-of-supremum]{Epsilon Characterization of Supremum}
\end{itemize}
\end{dependencies}
```

The machine-readable dependency target is the label inside the `\hyperref[...]`
brackets.

Use the shared `dependencies` environment for dependency blocks. Do not write
dependency blocks as raw `remark*` environments with a `Dependencies` title;
that bypasses the shared alignment rule used by volume builds.

Proof labels such as `prf:` identify proof files or proof locations. They may
be used for theorem-proof navigation and theorem-proof association, but they
must not appear as mathematical dependency targets.

If a dependency cannot be linked because the target statement has not yet been
formalized, write a TODO dependency note rather than inventing a label.
