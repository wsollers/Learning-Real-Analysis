# Notation Standards

Source sections: `DESIGN.md` primary rules A-C, section 3, and the canonical
source rule after section 3.

## Layer-Gated Notation

Before editing or generating a formal block, classify its role. Definition,
theorem, lemma, proposition, corollary, axiom bodies, and ordinary quantified
statements use standard mathematical notation. Predicate names are reserved for
approved predicate-reading layers and related analysis layers.

## No Predicate Leakage

Canonical predicate names must not appear in definition bodies, theorem-like
bodies, standard quantified statements, negated quantified statements, or
contrapositive quantified statements unless a task explicitly requests a
predicate-language version.

## Canonical Sources

The source-of-truth files remain in `Learning-Real-Analysis`:

- `predicates.yaml`
- `notation.yaml`
- `relations.yaml`

They are not duplicated into volume repos and should be treated as read-only by
automated authoring processes unless the task is specifically to update the
canonical YAML source.

## Standard Notation Normalizations

Preserve mathematical meaning; normalize mathematical notation.

When source text, OCR text, or image-derived mathematical content is converted
into LRA-ready candidate LaTeX, rewrite common source/OCR notation into house
LaTeX unless the task explicitly requests source-faithful transcription.

| Source or OCR form                                                 | House LaTeX form                                           |
| ------------------------------------------------------------------ | ---------------------------------------------------------- |
| `R`, `Real`, `reals` when meaning the real numbers                 | `\mathbb{R}`                                               |
| `N`, `Z`, `Q` when meaning standard number systems                 | `\mathbb{N}`, `\mathbb{Z}`, `\mathbb{Q}`                   |
| `I(R)`, `I\((R)\)`, interval family over `R`                       | `\mathcal{I}(\mathbb{R})`                                  |
| `in` when used as membership                                       | `\in`                                                      |
| `notin`, `not in`, `∉`                                             | `\notin`                                                   |
| `subset`, `subseteq`, `⊆`                                          | `\subseteq`                                                |
| `proper subset`, `⊂` when source clearly means proper subset       | `\subsetneq`                                               |
| `forall`, `for all`, `∀`                                           | `\forall`                                                  |
| `exists`, `there exists`, `∃`                                      | `\exists`                                                  |
| `=>`, `implies`, `⇒`                                               | `\Rightarrow`                                              |
| `<=>`, `iff`, `⇔`                                                  | `\Longleftrightarrow`                                      |
| `*` used as multiplication                                         | `\cdot`                                                    |
| juxtaposed interval products such as `AB` when clarity requires it | `A\cdot B`                                                 |
| `A(B+C)`                                                           | `A\cdot(B+C)` when multiplication clarity is needed        |
| `t in R`                                                           | `t\in\mathbb{R}`                                           |
| `0 in B`                                                           | `0\in B`                                                   |
| `0 notin B`, `0 ∉ B`                                               | `0\notin B`                                                |
| `inf`, `sup` as operators                                          | `\inf`, `\sup`                                             |
| `min`, `max` as operators                                          | `\min`, `\max`                                             |
| `epsilon`, `eps`, `ε`                                              | `\varepsilon` unless source convention requires `\epsilon` |
| `delta`, `δ`                                                       | `\delta`                                                   |
| `bar a`, `a_bar`, upper endpoint of interval                       | `\overline a`                                              |
| `underline a`, `a_under`, lower endpoint of interval               | `\underline a`                                             |
| `[a,b]` closed interval                                            | `[a,b]`                                                    |
| `(a,b)` open interval                                              | `(a,b)`                                                    |
| `[a,b)` half-open interval                                         | `[a,b)`                                                    |
| `(a,b]` half-open interval                                         | `(a,b]`                                                    |

For interval arithmetic, use `\mathcal{I}(\mathbb{R})` for the set of bounded
closed real intervals. Use endpoint notation such as
`A=[\underline a,\overline a]` and `B=[\underline b,\overline b]`. Use
`\cdot` for interval multiplication in formal notes, explicit membership such
as `A,B\in\mathcal{I}(\mathbb{R})`, explicit conditions such as `0\notin B`,
and `\subseteq` for sub-distributivity.

Do not normalize notation when the task explicitly asks for exact
transcription, the source is introducing special notation, changing notation
would obscure a source distinction, or the notation is part of a named
convention being quoted or compared. When source notation is preserved
intentionally, add a brief review note if the notation differs from house
style.

## Missing Predicate Protocol

Do not invent predicates inline. If a needed logical structure cannot be
expressed with current canonical predicates, report the missing predicate need
instead of creating an ad hoc name.
