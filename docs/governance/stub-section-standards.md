# Stub Section Standards

Stub section standards apply when a governance rebuild needs planned topic
locations inside an existing chapter before mathematical content has been
migrated or authored.

## Purpose

A section stub is used during governance rebuilds to establish the future
location of a topic before mathematical content is migrated.

A section stub is architecture, not content. It must not invent mathematical
material.

## Invocation Pattern

Typical instructions may be:

```text
Generate section stub:
  axioms
```

or:

```text
Generate section stubs:
  signatures
  theories
  models
  consistency
```

When such an instruction is encountered, create the required directory
structure and router wiring automatically, subject to local repo conventions
and idempotency rules.

## Required Notes Structure

Generating a notes section such as:

```text
axioms
```

inside a chapter should create:

```text
notes/
  axioms/
    index.tex
```

The file should contain a planned-section placeholder. Use the section title in
place of `Axioms`:

```text
Axioms Toolkit - Planned

This section is a rebuild placeholder.

This section is planned for the governance rebuild. No mathematical content has been restored here yet.
```

Do not add definitions, theorems, examples, dependencies, topic lists, or
labels unless they are already supplied by a canonical registry or by the task.

## Required Proof Structure

For every notes section created, create the matching proof section:

```text
proofs/
  axioms/
    index.tex
```

The file should contain:

```text
Proofs for this section are planned.

No proof files have been regenerated.
```

Do not create proof files for nonexistent statements. Proof file creation must
also satisfy `proof-standards.md`.

## Router Updates

After generating a section stub, update the chapter's notes router:

```text
notes/index.tex
```

to include the new section in the correct order, using the repository's
preferred routing style. A common form is:

```latex
\input{notes/axioms/index}
```

Then update the chapter's proofs router:

```text
proofs/index.tex
```

to include the matching proof section, using the repository's preferred
routing style. A common form is:

```latex
\input{proofs/axioms/index}
```

Inspect nearby chapters and follow the local routing convention. Do not invent
a new routing style.

## Ordering Rule

When multiple section stubs are generated, insert them in the order provided by
the request unless an explicit chapter roadmap, registry, or existing router
already defines a different order.

For example:

```text
signatures
axioms
theories
models
consistency
```

should be routed in that order unless local chapter structure says otherwise.

## Idempotency Rule

If:

```text
notes/<section>/index.tex
```

already exists:

- do not overwrite mathematical content;
- do not replace an existing section;
- only report that the section already exists.

If the notes section exists but the matching proof section or router entry is
missing, inspect the existing chapter structure before adding only the missing
architecture needed to restore consistency.

## Mathematical Content Rule

A section stub must not invent:

- definitions;
- theorems;
- proofs;
- examples;
- exercises;
- dependencies;
- labels for nonexistent content.

This restriction applies unless the task explicitly provides the content and
authorizes active content generation. Missing mathematical needs should be
reported instead of filled.

## Build Rule

After generating section stubs:

- update all affected routers;
- run the repo-local build command or documented build command;
- verify that all new sections resolve correctly.

Generated stubs must not break the build. If the build command is unavailable
or the repo lacks a build target, report that condition explicitly.

## Report Rule

Any section-stub generation task should report:

- sections created;
- notes files created;
- proof files created;
- routers updated;
- build result;
- warnings or errors.

## Example

A request such as:

```text
Generate section stubs:

signatures
axioms
theories
models
consistency
independence
```

should create:

```text
notes/
  signatures/index.tex
  axioms/index.tex
  theories/index.tex
  models/index.tex
  consistency/index.tex
  independence/index.tex

proofs/
  signatures/index.tex
  axioms/index.tex
  theories/index.tex
  models/index.tex
  consistency/index.tex
  independence/index.tex
```

and all parent index files should be updated automatically.

