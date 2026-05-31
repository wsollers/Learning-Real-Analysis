# Proof Standards

Source sections: `DESIGN.md` sections 8, 9, 10.4, 14, and proof lifecycle
rules after section 14.

## Proof Ownership

No proof file may be created for a statement unless the corresponding statement
label already exists in the notes file.

## Proof File Structure

Proof files live under the owning chapter's `proofs/` tree and use lowercase,
hyphen-separated ASCII filenames. Each notes proof file begins on a new page.

A full proof file contains:

1. proof-level label,
2. unnumbered theorem-like restatement,
3. theorem-side navigation link when applicable,
4. professional standard proof,
5. detailed learning proof,
6. proof structure remark,
7. dependencies remark.

## Two-Layer Proof Rule

The professional proof is compact and rigorous. The detailed learning proof
teaches the same proof with explicit step structure. Explanationless proof mode
is opt-in only and does not waive notation, label, dependency, or architecture
rules.

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
