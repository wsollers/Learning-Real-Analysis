# Atomic Artifact Standards

These standards define repository-wide architectural invariants for
mathematical objects and figures. They apply to all volumes, split
repositories, future repositories, generators, extraction tools, and manual
authoring.

## Governing Principle

Every mathematical object that may appear as a node in the knowledge graph
shall exist as an independent repository artifact.

This means:

- one concept -> one definition;
- one theorem -> one theorem artifact;
- one proof -> one proof artifact;
- one figure -> one figure artifact;
- one capstone -> one capstone artifact;
- one graph node -> one stable label.

## Atomic Definitions

Every mathematical concept shall be introduced in its own definition
environment and shall possess its own unique label.

Canonical principle:

```text
One concept -> one definition -> one label.
```

Each mathematical concept introduced into the repository shall correspond to
exactly:

- one definition environment;
- one label;
- one knowledge-graph node;
- one extraction record.

Grouping multiple independent mathematical concepts into a single definition
environment is prohibited. This prohibition applies even when the concepts are
closely related, commonly taught together, or convenient to list together.

Allowed pattern:

```latex
\begin{definitionbox}{Reflexive Relation}
...
\label{def:reflexive-relation}
\end{definitionbox}

\begin{definitionbox}{Symmetric Relation}
...
\label{def:symmetric-relation}
\end{definitionbox}

\begin{definitionbox}{Transitive Relation}
...
\label{def:transitive-relation}
\end{definitionbox}
```

Forbidden pattern:

```latex
\begin{definitionbox}{Properties of Relations}
...
\label{def:relation-properties}
\end{definitionbox}
```

Reason: multiple independent concepts cannot share one repository identity.

When auditing existing content, multi-concept definitions shall be split into
atomic definitions, each new concept shall receive its own stable label, and
all references and extraction records shall be updated to the new labels.

## Atomic Figures

Every nontrivial TikZ figure shall exist as an independent figure source file.

Canonical principle:

```text
One figure -> one figure file.
```

All nontrivial TikZ figures shall be extracted into dedicated figure source
files. Embedded nontrivial `tikzpicture` environments in mathematical note,
proof, exercise, or exposition files are prohibited.

Figure source files shall contain only:

```latex
\begin{tikzpicture}
...
\end{tikzpicture}
```

Figure source files shall not contain:

- `figure` environments;
- captions;
- labels;
- surrounding explanatory text;
- document preambles;
- local color definitions.

Captions, labels, placement options, and explanatory prose belong at the
inclusion point.

Correct inclusion pattern:

```latex
\begin{figure}[htbp]
\centering
\input{figure-relation-dependency}
\caption{Dependency structure of relation properties.}
\label{fig:relation-dependency}
\end{figure}
```

## Trivial TikZ Exemption

The only permitted embedded `tikzpicture` environments are trivial diagrams.
A trivial diagram is a small, local visual mark that has no independent
mathematical identity, no caption, no label, no dependency-graph role, and no
reuse value.

Examples may include tiny inline arrows, ticks, or one-off visual separators.
Any diagram that explains a mathematical relation, construction, dependency,
map, commutative structure, graph, geometry, approximation, or proof idea is
nontrivial and shall be extracted.

When in doubt, extract the figure.

## Generator Requirements

Chapter, section, statement, and figure generators shall enforce these
invariants before emitting source:

- if requested definition content contains more than one independently
  nameable concept, generation shall stop and return a bundled-content notice;
- if requested figure content contains a nontrivial TikZ diagram, generation
  shall emit a dedicated figure source file and an inclusion block;
- generators shall not create shared labels for bundled concepts;
- generators shall not embed nontrivial TikZ diagrams in note bodies.

## Extraction Requirements

Extraction tools shall treat each atomic definition and each figure inclusion
as an individually addressable artifact. Extraction records shall not merge
independent mathematical concepts into a single graph node. Figure source files
provide figure bodies only; figure labels are extracted from the inclusion
point.
