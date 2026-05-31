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

## Missing Predicate Protocol

Do not invent predicates inline. If a needed logical structure cannot be
expressed with current canonical predicates, report the missing predicate need
instead of creating an ad hoc name.
