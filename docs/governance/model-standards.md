# Model Standards

Model standards govern structural presentations: signatures, languages,
structures, models, syntactic objects, classification cards, structure
blueprints, and related model-theoretic or algebraic objects.

These standards do not replace theorem-like standards. The theorem-like
standards remain authoritative for definitions, axioms, theorems, lemmas,
propositions, and corollaries as formal mathematical statements. This document
governs the surrounding structural presentation layer and the special cases
where structural objects should not be forced into predicate-oriented
decoration.

## 1. Scope

Use this standard for mathematical objects whose main role is to describe
vocabulary, syntax, components, inheritance, taxonomy, or semantic
organization.

Examples include:

- signatures;
- languages;
- theories;
- structures;
- interpretations;
- models;
- variables;
- terms;
- formulas;
- sentences;
- groups;
- rings;
- fields;
- vector spaces;
- metric spaces;
- topological spaces;
- measure spaces;
- Peano systems.

The standard applies to first presentations, comparison tables, structure
blueprints, classification cards, syntactic presentations, formation rules,
worked examples, model-theoretic remarks, and metadata-bearing presentations
intended for extraction or future Knowledge Explorer views.

## 2. Structure Presentation Order

When applicable, present structural material in this preferred order:

1. Structure Blueprint
2. Signature Presentation
3. Classification
4. Definition
5. Interpretation

Omit layers that do not apply. For example, a vocabulary-only section may need
a signature presentation and vocabulary definitions but no classification
card. A group presentation may need a structure blueprint and classification
card but no signature table unless the language of groups is being made
explicit.

The presentation order is a local organization rule. It does not override the
block order inside a formal theorem-like environment when such an environment
is used.

## 3. Structural Toolkit Standard

Model-theoretic and signature-heavy chapters may begin with a compact toolkit
that lists the local structural vocabulary, symbols, and links to definitions.
Use exactly one toolkit box at the front of a section when a toolkit is
requested or locally required.

Structural toolkits should use the shared `toolkitbox` macro. Local files must
not hand-roll gray toolkit `tcolorbox` styling when `toolkitbox` is available.

Toolkits are navigational and structural summaries. They do not replace formal
definitions, signature presentations, interpretation remarks, or dependency
blocks.

## 4. Signature Presentation Standard

A signature presentation records formal vocabulary. It describes which symbols
exist and how many inputs the relevant symbols take. It does not assert truth,
choose axioms, or specify which structures satisfy those axioms.

A signature presentation should include:

- a Signature Box;
- a Symbol Table;
- arity information for function and relation symbols;
- interpretation notes explaining the intended reading of each symbol.

The symbol table should distinguish:

- constant symbols;
- function symbols;
- relation symbols;
- arity;
- intended reading.

Signature presentations should use shared structural macros when available,
such as `signaturebox` and `signaturetable`. Local files must not redefine
signature colors, table layout, or structural box palettes.

Interpretation notes inside a signature presentation are informal readings of
symbols. They are not semantic assignments, model definitions, or truth
conditions.

## 5. Structure Blueprint Standard

A structure blueprint is a compact structural summary of what an object adds,
inherits, contains, uses, or classifies as. It is descriptive metadata, not a
theorem-like statement.

Blueprints may include these panels:

- `NEW AT`: data, operations, axioms, or constraints first introduced at this
  structure level;
- `INHERITS`: parent structures or previously established structure;
- `HAS-A`: components included as part of the object;
- `USES-A`: auxiliary structures, sets, functions, relations, or ambient
  objects used by the definition;
- `CLASSIFICATION`: taxonomy, position in a hierarchy, or equivalent
  structural role.

Blueprint diagrams and panels should be generated from metadata whenever
possible. Manual diagrams are acceptable when metadata is not yet available,
but they should be written so that a later metadata source can reproduce the
same structure without changing the mathematical meaning.

Blueprints must not replace formal definitions. When the object is being
defined, the formal definition remains the authoritative mathematical statement.

## 6. Classification Standard

Classification cards present the taxonomic position of a structural object.
They are used for inheritance, cross-referencing, and comparison across
chapters.

Examples include:

- commutative monoid;
- group;
- field;
- ordered field;
- complete ordered field.

A classification card may record:

- parent structures;
- additional axioms or constraints;
- canonical examples;
- common equivalent names;
- downstream uses;
- links to related definitions or blueprints.

Classification cards should use shared classification macros when available.
They are extraction-friendly presentation artifacts, not theorem-like
environments, and do not require predicate readings or negation blocks.

## 7. Vocabulary Object Definitions

Vocabulary Object Definitions are formal definitions whose purpose is to name
a structural or semantic object rather than to introduce a predicate used in
ordinary proofs.

Examples include:

- Signature;
- Language;
- Constant Symbol;
- Function Symbol;
- Relation Symbol;
- Arity;
- Theory;
- Structure;
- Interpretation;
- Model.

Vocabulary Object Definitions require:

- a formal Definition environment with a stable `def:` label;
- an Interpretation block unless nearby structural exposition already performs
  the same work clearly.

Vocabulary Object Definitions may require:

- a formal characterization;
- a signature presentation;
- a structure blueprint;
- a classification card;
- a table or component display;
- a dependency block or `\NoLocalDependencies` marker.

When dependency information is present, it is governed by
`dependency-standards.md`.

Vocabulary Object Definitions do not require:

- predicate readings;
- negation predicate readings;
- contrapositive blocks.

Generate predicate or negation predicate readings for Vocabulary Object
Definitions only when they are explicitly useful, canonical, and not merely
mechanical.

## 8. Syntactic Object Definitions

Syntactic Object Definitions are formal definitions whose purpose is to name
objects built by formation rules inside a formal language.

Examples include:

- Variable;
- Term;
- Formula;
- Sentence.

Syntactic Object Definitions require:

- a formal Definition environment with a stable `def:` label;
- an Interpretation block unless nearby syntactic exposition already performs
  the same work clearly.

Syntactic Object Definitions often require:

- a `syntacticbox` presentation for compact object-level summaries;
- a `formationrules` block when the object is inductively generated;
- a `workedexamples` block when examples clarify grammatical boundaries;
- a dependency block or `\NoLocalDependencies` marker.

Formation rules should state the constructors and closure conditions that
generate the object. Worked examples should separate well-formed examples from
near misses when that distinction prevents later confusion.

Syntactic Object Definitions do not require:

- predicate readings;
- negation predicate readings;
- contrapositive blocks.

Generate predicate or negation predicate readings for Syntactic Object
Definitions only when the predicate is canonical and useful, not merely because
the object can be recognized by a grammar.

## 9. Structural Definitions

Structural Definitions introduce mathematical objects by components,
operations, relations, axioms, compatibility conditions, or ambient data.

Examples include:

- Group;
- Ring;
- Field;
- Vector Space;
- Metric Space;
- Topological Space;
- Peano System.

Structural Definitions may include:

- component displays;
- underbrace decomposition;
- signature boxes;
- structure blueprints;
- classification cards;
- inheritance summaries;
- compact tables of operations, relations, and laws.

The formal Definition environment remains the authoritative statement of the
object. Supplemental structural presentations should make the components and
inheritance easier to inspect without duplicating or contradicting the
definition.

When a Structural Definition includes axioms or laws, atomicity is mandatory:
independently nameable axioms, laws, operations, relations, and component
concepts shall receive their own formal environments and stable labels when
they are introduced as repository concepts. They shall not be hidden inside
one bundled definition when they must later be cited, extracted, or used as
graph nodes.

## 10. Model-Theoretic Remarks

Model-theoretic remarks may explain structural distinctions without requiring
full logical decomposition.

Allowed remarks include discussion of:

- same signature, different theories;
- same theory, different models;
- same vocabulary, different intended readings;
- inheritance relationships;
- component changes across related structures;
- taxonomy and classification boundaries.

These remarks should be precise and local. They may orient readers before full
model theory is available, but they must not introduce unsupported theorems or
unscoped semantic claims.

## 11. Knowledge Explorer Integration

Structural and syntactic presentations are first-class extraction targets.
Metadata should be represented in stable, machine-readable categories where
possible.

Intended metadata categories include:

- `structure`;
- `signature`;
- `classification`;
- `syntactic-object`;
- `proof-vault-url`.

Future generated artifacts may include:

- Knowledge Explorer cards;
- TikZ structure DAGs;
- blueprint diagrams;
- classification tables;
- syntactic formation cards.

Any generated TikZ structure DAG or blueprint diagram that is nontrivial shall
be emitted as a dedicated figure source file under the applicable section
directory. The note body shall input the figure source file from the figure
environment rather than embedding the `tikzpicture`.

Extraction tooling should treat structural and syntactic metadata as distinct
from theorem-like logical blocks. A structure blueprint, signature
presentation, classification card, syntactic presentation, formation-rules
block, or worked-examples block may create graph nodes or display records even
when it does not correspond to a theorem, lemma, proposition, corollary, axiom,
or formal definition.

## 12. Overrides

This document overrides theorem-like decoration rules only for structural or
syntactic presentation artifacts and for Vocabulary Object Definitions,
Syntactic Object Definitions, or Structural Definitions where predicate-oriented
blocks would be artificial.

Overrides:

- predicate readings are optional for Vocabulary Object Definitions,
  Syntactic Object Definitions, and Structural Definitions;
- negation predicate readings are normally omitted;
- negated quantified statements are normally omitted unless the negated form is
  a standard proof tool for the object;
- contrapositives are normally omitted and are not generated for definitions;
- classification cards, structure blueprints, signature presentations,
  syntactic presentations, formation-rules blocks, and worked-examples blocks
  do not require theorem-like logical blocks;
- interpretation blocks remain encouraged and are the preferred explanatory
  layer for structural and syntactic meaning;
- formal definitions still require stable labels and dependency handling under
  existing extraction standards;
- theorem-like standards remain authoritative whenever a structural or
  syntactic presentation contains an actual definition, axiom, theorem, lemma,
  proposition, or corollary.

If a conflict arises, apply the more specific rule: theorem-like standards
govern formal theorem-like environments, while this document governs structural
and syntactic presentation artifacts and the omission of non-useful
predicate-oriented decoration around structural and syntactic objects.

Decoration block shape and ordering, when such blocks are used, are governed by
`decoration-box-standards.md`.
