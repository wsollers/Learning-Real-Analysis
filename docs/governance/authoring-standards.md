# Authoring Standards

Source sections: `DESIGN.md` sections 1, 4, 5, 6, 6.1, 7, 12, and 15.

## Purpose

The notes are a long-term mathematical reference. They preserve definitions,
theorems, proof structures, dependencies, and canonical notation in a form that
remains readable, rigorous, and stable across years of revision.

## Voice

Use a precise reference voice. Do not write as a course transcript, workbook,
chatty explanation, or motivational aside. Exposition should state what a
definition or result does mathematically and how it fits the local structure.

## Boxes

Boxes are structural, not decorative.

Box a definition, axiom, or theorem only when it introduces a structural,
load-bearing concept or result. A boxed item should be central to the local
section, reused later, important for future learning, or expected to carry
dependency weight in the knowledge graph.

Do not box merely because an item is a first appearance.

Every mathematical concept shall be introduced in its own definition
environment and shall possess its own unique label. Grouping multiple
independent mathematical concepts into a single definition environment is
prohibited. This rule is architectural, not stylistic: one concept maps to one
definition, one label, one knowledge-graph node, and one extraction record.

Box-worthy examples include structural concepts and results such as
`Supremum`, `Least Upper Bound Property`, `Sequential Limit`, `Cauchy Sequence`,
`Continuity`, `Derivative`, `Partition`, and `Riemann Integrability`.

Minor auxiliary notions, routine variants, examples, remarks, lemmas,
propositions, corollaries, computational rules, and one-off conveniences are
normally unboxed unless the section explicitly treats them as structural and
load-bearing.

Each section begins with exactly one gray Toolkit box at the top. Chapter
entries use the required breadcrumb and roadmap structure.

## Figures

Every nontrivial TikZ figure shall exist as an independent figure source file.
Embedded nontrivial `tikzpicture` environments in note bodies, proof bodies,
exercise bodies, exposition blocks, and statement files are prohibited.

Figure source files contain only the `tikzpicture` environment. Captions,
labels, placement, and explanatory prose belong at the inclusion point.

## Chapter Entries

Chapter openings use the canonical entry pattern from `DESIGN.md`: breadcrumb,
status when needed, roadmap, and chapter structure. Breadcrumbs are structural
dependency statements, not motivational prose.

## Layered Exposition

Each layer has one job:

- formal environments state the mathematics,
- interpretation remarks explain meaning,
- logical blocks expose formal structure,
- dependency blocks support extraction,
- prose connects local context.

Do not over-symbolize exposition. Use prose where prose is the correct layer.
