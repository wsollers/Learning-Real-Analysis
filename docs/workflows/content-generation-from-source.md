# Content Generation From Source

Use this workflow whenever a source snippet of text or image is converted into
LRA-ready candidate LaTeX. This applies whether the snippet comes from a GUI,
plain chat paste, OCR, image paste, or an automated tool.

## Core Rule

Comprehend first, rewrite second, emit LaTeX third.

Do not mechanically wrap OCR fragments in LaTeX environments. Read the whole
visible mathematical passage, understand the math, rewrite it internally as a
clean mathematical note, and then generate LaTeX from that rewritten note.

## Extraction Pipeline

### 1. Convert input into editable text

- Determine whether the input is already text or an image.
- If the input is already text, use that text directly.
- If the input is an image, run OCR first.
- Expose OCR text for review before final extraction.
- Treat OCR text as provisional. Mathematical symbols, endpoints, quantifiers,
  subscripts, superscripts, and interval brackets require review.

Image input becomes text first, then follows the same extraction workflow as
pasted text.

### 2. Read the whole visible passage

Read the entire visible mathematical passage before deciding what to extract.

Do not anchor on:

- the first bolded label,
- the first explicit `Definition`, `Theorem`, or `Example`,
- the first displayed formula,
- the most visually prominent local block.

Default to extracting the full visible passage unless the user explicitly
chooses selected-item extraction.

### 3. Understand the mathematics

Before producing LaTeX, understand what the passage is doing mathematically.

Ask:

- What is the topic?
- What are the mathematical objects?
- What is being defined?
- What is being claimed?
- What is being illustrated?
- What fails?
- Is there a counterexample?
- Is there a replacement law or weaker statement?
- Is there a special case?
- Is notation being introduced?
- Are there computational formulas or rules?
- What formulas are central?

Common source content includes definitions, theorem-like claims, propositions,
laws, examples, counterexamples, notation, computational formulas, proof
sketches, warnings, pitfalls, special cases, and explanatory prose.

### 4. Write an internal bullet list

Before emitting LaTeX, write an internal bullet list of every mathematical
claim or displayed formula in the visible snippet.

The bullet list is the comprehension guardrail.

It should include:

- every displayed formula that states a definition, claim, law, example,
  counterexample, special case, or computation rule;
- every prose sentence that makes a mathematical claim;
- every warning, failure, exception, or special case;
- every example or counterexample;
- every introduced piece of notation.

If the bullet list omits an important visible formula or claim, redo the
summary before generating LaTeX.

Do not skip earlier claims because a later `Example`, `Definition`, or
`Theorem` label appears.

### 5. Rewrite into a clean mathematical note

Rewrite the passage internally into standard mathematical form before
generating LaTeX.

The rewritten note should be better organized than the raw source while
preserving its meaning.

Use the mathematical role of each item:

- definitions become `definition` environments;
- theorem-like claims, laws, and reusable facts become `proposition` or another
  theorem-like environment;
- examples and counterexamples become `example` environments;
- warnings, exposition, interpretation, and informal comments become `remark*`
  environments.

If the source gives one definition with several operation cases, emit a short
exposition remark and then one definition per operation or case.

If the source states several laws, emit propositions or grouped propositions.

If the source gives a counterexample, title it by the law it refutes.

If the source says a law fails and then gives a weaker replacement law, extract
both the failure/counterexample and the weaker law.

If the source gives a warning or practical comment, use `remark*`.

### 6. Emit LaTeX from the rewritten note

Generate LaTeX from the cleaned mathematical note, not directly from OCR
fragments.

Choose environments by mathematical role, not by raw visual layout.

For a single definition snippet, emit one definition.

For a definition cluster, emit clustered definitions.

For a mixed mathematical passage, emit the clean note that best represents the
full visible mathematical argument.

### 7. Check the output

Before returning the candidate LaTeX, check:

- No important visible formula or claim was dropped.
- The extraction did not accidentally start too late in the passage.
- The output preserves the mathematical flow of the source.
- Examples are classified as examples.
- Counterexamples are identified by the law they refute.
- Warnings and informal comments are not over-promoted into theorems.
- Labels are semantic and stable.
- Titles are meaningful mathematical titles, not OCR phrases.
- Display math delimiters are balanced.
- Environments are balanced.
- Citations and labels follow project style.

## User-Guided Checkboxes

Optional UI checkboxes may guide extraction:

- Contains definitions
- Contains theorems/propositions/laws
- Contains examples
- Contains counterexamples
- Contains notation
- Contains computational formulas
- Contains proof/proof sketch
- Contains warnings/pitfalls
- Contains special cases
- Rewrite into standard note form
- Preserve source order closely
- Generate explanatory remarks
- Generate small examples when helpful
- Extract full visible passage
- Extract selected highlighted item only

Default behavior:

- Extract full visible passage.
- Rewrite into standard note form.
- Generate explanatory remarks when useful.
- Do not extract only a local example, theorem, or definition unless the user
  explicitly selected `Extract selected highlighted item only`.

Checkboxes guide extraction, but they do not replace mathematical
comprehension.

## Labels and Citations

Do not use raw OCR phrases as titles.

Bad:

```latex
\begin{example}[As a Counter Example Consider]
\label{ex:as-a-counter-example-consider}
```

Good:

```latex
\begin{example}[Failure of distributivity]
\label{ex:failure-of-distributivity}
```

Labels must come from normalized mathematical titles, not from OCR spans,
section prose, page numbers, running headers, or truncated fragments.

For extracted definitions, use the project's canonical order:

```latex
\begin{definition}[Title]
\cite{SourceKey}
\label{def:semantic-label}
...
\end{definition}
```

Generated exposition and generated examples are explanatory additions, not
source quotations. Do not cite generated examples unless the example itself is
copied or directly adapted from the source.

## LaTeX Validity

Before returning output, check:

- every `\[` has a matching `\]`;
- every `\begin{...}` has a matching `\end{...}`;
- no `\cite` or `\label` appears inside unfinished display math;
- citations and labels use the project's canonical order;
- labels are semantic and stable.

## House Rules

Apply the current constitution and governance standards while generating:

- `constitution/master.md`
- `docs/governance/authoring-standards.md`
- `docs/governance/atomic-artifact-standards.md`
- `docs/governance/notation-standards.md`
- `docs/governance/extraction-standards.md`

Generated output is always staged candidate content for human review. It must
not be treated as final note insertion.
