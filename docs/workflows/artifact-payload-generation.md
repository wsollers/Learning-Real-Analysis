# Artifact Payload Generation

Use this workflow for chapter artifact generation from an ordered mathematical
payload. It is the default workflow for large generated chapter sections,
especially formal-language chapters such as Propositional Logic.

## Source Of Truth

Use machine-ingested payloads as the source of truth.

Do not hand-translate long prose prompts directly into YAML or LaTeX. The
preferred pipeline is:

```text
ChatGPT mathematical payload
-> JSON/JSONL payload, optionally using *_b64 text fields
-> tools/import_artifact_payload.py
-> appendable YAML registry
-> tools/chapter_artifact.py
-> generated LaTeX, notation page, proof stubs, manifests
-> deterministic local audit
```

When a payload is supplied, Codex should not reinterpret, rewrite, or invent
the mathematics unless explicitly asked. Codex should run the importer, fix
mechanical schema/import/build issues, report validation and audit findings,
and prefer patching the payload, importer, registry, or generator over
hand-editing generated LaTeX.

## Ordering

The order of artifacts in the payload and registry is pedagogical order.

- Render artifacts in payload order.
- Do not sort artifacts alphabetically inside a topic.
- Render notation pages in first-occurrence order unless explicitly configured
  otherwise.
- Render proof indexes and chapter manifests in occurrence order unless an
  audit requires another order.

For Propositional Logic and future formal-language chapters, place notation
before first use:

```text
1.1 Notation and Conventions
1.2 Syntax
1.3 Semantics
...
```

The notation page must appear before the first section that uses the notation.

## Payload Format

For large batches, create a payload writer or exact JSON/JSONL payload instead
of asking Codex to transcribe prose into YAML.

Use base64 fields for LaTeX/prose transfer when helpful:

- `body_b64`
- `quantified_form_b64`
- `predicate_reading_b64`
- `negated_form_b64`
- `interpretation_b64`
- `readable_decomposition_b64`
- `failure_mode_decomposition_b64`
- `examples_b64`
- `non_examples_b64`

The importer decodes payload fields to readable UTF-8 YAML block scalars.
Base64 is allowed for machine-ingestion payloads, not as the preferred
long-term registry format.

## Registry Output

Appendable YAML registries should keep multiline LaTeX/prose readable:

```yaml
body: |
  The set \(\WFF\) of well-formed formulas is defined recursively.
```

Do not store ordinary long-form LaTeX/prose as base64 in the YAML registry.

## Generated Artifacts

Generated LaTeX artifacts must remain wrapped by generated block markers:

```latex
% BEGIN GENERATED ARTIFACT: <label>
...
% END GENERATED ARTIFACT: <label>
```

If generated content needs correction, update one of:

- payload writer,
- JSON/JSONL payload,
- importer normalization,
- YAML registry,
- generator template.

Then regenerate. Do not patch generated LaTeX directly unless explicitly
performing an emergency repair.

## Proof Stubs

For theorem-like artifacts with `proof_required: true`, create proof stubs in
the required proof-layer format. Do not attempt long proofs during content
universe generation unless explicitly requested.

## Audits

Payload rehydration workflows use deterministic/local audits by default:

- `tools/chapter_artifact.py validate`
- chapter true-up
- box-color audit
- proof-layout audit
- `latexmk` build
- local registry/symbol checks if available without an AI provider

Do not run AI-backed audits by default. Commands using `-ai codex` are opt-in
only and must be run only when explicitly requested.

Importer flags:

- `--audit-local` runs the default deterministic audit suite.
- `--audit-ai` runs optional AI-backed review.
- `--audit-full` runs local audits followed by optional AI-backed review.
- legacy `--audit` means local-only unless explicitly changed.

## Notation And Predicate Rendering

When rendering formal predicate names, use operator/text formatting, not raw
math italics.

Correct:

```latex
\[
\operatorname{WellFormedFormula}(\varphi)
\]
```

Incorrect:

```latex
\[
WellFormedFormula(\varphi)
\]
```

Apply this to predicates such as:

- `PropositionalVariable`
- `WellFormedFormula`
- `AtomicFormula`
- `MolecularFormula`
- `MainConnective`
- `LogicalConnective`
- `UnaryConnective`
- `BinaryConnective`
- `ParseTree`
- `FormulaVariableSet`
- `Subformula`
- `FormulaDepth`
- `ConnectiveCount`

Use canonical truth-value notation:

```latex
\[
\mathbb{B}
\]
```

Do not use `\Bool`. Normalize variants such as `\mathbb B` to `\mathbb{B}`.

Deduplicate notation table rows by canonical notation key or normalized
symbol. For formula metavariables, prefer one row:

```latex
\[
\varphi,\psi,\chi,\theta
\]
```

Avoid separate duplicate rows for `\(\varphi\)`, `\(\psi\)`, or
`\(\varphi,\psi\)` unless a section introduces a genuinely distinct
convention.

## Formalization Guardrails

The well-formed formula set is the smallest formation-closed set. The formal
statement must use an intersection of all formation-closed supersets:

```latex
\[
\WFF
=
\bigcap
\left\{
S :
\Prop\subseteq S,\;
(\forall\varphi\in S)(\neg\varphi\in S),\;
(\forall\varphi,\psi\in S)
(\forall\circ\in\{\land,\lor,\to,\leftrightarrow\})
((\varphi\circ\psi)\in S)
\right\}.
\]
```

Do not write:

```latex
\[
\WFF=\{S:\cdots\}
\]
```

That describes the collection of closed supersets, not their intersection.

For generic comparison of binary connectives, use:

```latex
\[
(\varphi\circ\psi)=(\alpha\star\beta)
\]
```

Do not use a literal question mark in place of the connective:

```latex
\[
(\varphi\circ\psi)=(\alpha ? \beta)
\]
```

Ensure the renderer does not convert `\star` into `?`.

## Report Requirements

Every generation run should report:

- payload file used;
- artifacts imported;
- notation page generated;
- proof stubs generated;
- registry entries or patches;
- deterministic audit results;
- LaTeX build status;
- remaining findings.
