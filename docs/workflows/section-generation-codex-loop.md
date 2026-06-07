# Section Generation Codex Loop

Use this workflow when planning and generating a new mathematical section in a
leaf volume repository through a disposable `codex-prompt` file.

This workflow separates mathematical planning from mechanical repository edits.
ChatGPT owns planning, topic decomposition, dependency routing, and prompt
construction. Codex owns repo-local execution, validation, commit, push, and
prompt deletion.

## Phase 1: Planning With User Approval

1. Inspect or pull the target repository context.
2. Generate or refresh the active definition/theorem label map for the relevant
   volume and chapter.
3. Identify the next section to generate from the approved volume/chapter spine.
4. Produce a detailed ASCII topic outline / table of contents for user approval.
5. Do not write a `codex-prompt` until the user approves the topic outline.

## Phase 2: Rule Refresh

Before writing the Codex prompt, refresh the relevant rules.

Required rule sources:

1. Target repo `AGENTS.md`.
2. `lra-governance/AGENTS.md` when available.
3. `docs/agent-task-index.md`.
4. The relevant repo overlay, especially
   `docs/governance/repo-overlays/lra-volume.md` for leaf volume repos.
5. Task-specific governance docs:
   - `docs/governance/atomic-artifact-standards.md`;
   - `docs/governance/dependency-standards.md`;
   - `docs/governance/proof-standards.md`;
   - `docs/governance/notation-standards.md`;
   - `docs/governance/extraction-standards.md` when dependency or knowledge
     routes matter.
6. Relevant schema files:
   - `constitution/schema/block-registry.yaml`;
   - `constitution/schema/artifact-matrix.yaml`;
   - `constitution/schema/file-schema.yaml`.

## Phase 3: Codex Prompt Creation

Create a disposable file named `codex-prompt` in the target repo.

The prompt must specify:

1. The target repo and exact task scope.
2. The rule files Codex must read before editing.
3. The exact file structure to create.
4. The exact index files to update.
5. The exact topic files to populate.
6. The file-splitting rule: for a new content section, create one section
   directory and one named `.tex` file per logical topic, such as
   `orientation.tex`, `literals-clauses.tex`, or `conversion-procedures.tex`.
   Do not create one subdirectory per topic unless explicitly requested.
7. The scope guard: Codex must not add extra definitions, theorems, labels,
   examples, exercises, figures, proof obligations, or sections beyond the
   prompt.
8. The requirement to generate one proof stub per theorem-like artifact.
9. The proof-stub requirements:
   - starts with `\newpage`;
   - includes `\phantomsection`;
   - includes a proof label `\label{prf:...}`;
   - includes `\LRAProofFor{...}`;
   - includes return navigation to the source theorem-like artifact;
   - restates the theorem, lemma, proposition, or corollary in a starred
     environment;
   - includes TODO professional proof;
   - includes TODO detailed learning proof;
   - includes a proof-structure remark;
   - includes a dependency block;
   - ends with `\clearpage`.
10. Validation and build commands.
11. The required final report format.
12. The instruction to delete `codex-prompt` before committing.
13. The commit message.

## Phase 4: Topic-by-Topic Content Generation

For each approved topic:

1. Generate the mathematical content for that topic only.
2. Append or spool that content into the corresponding topic-file instruction
   inside `codex-prompt`.
3. Keep topic files focused and atomic.
4. Use graph-aware dependency blocks from `dependency-standards.md`.
5. Use existing labels from the active label map whenever possible.
6. Do not invent dependency labels.
7. If a needed dependency target is missing, emit an unresolved dependency
   comment in the requested form rather than inventing a label.

## Phase 5: Codex Execution

The user tells Codex to process `codex-prompt` in the target repo.

Codex must:

1. Read the required rules.
2. Create the requested files.
3. Populate each topic file exactly as instructed.
4. Generate one required proof stub per theorem-like artifact.
5. Hook all notes into notes indexes.
6. Hook all proof stubs into proof indexes.
7. Verify note-side theorem-like artifacts have proof navigation.
8. Verify proof files have return navigation and `\LRAProofFor{...}`.
9. Run validators.
10. Run the LuaLaTeX build or the project build wrapper.
11. Delete `codex-prompt`.
12. Commit and push.
13. Report changed files, validation status, build status, commit hash, and any
    unresolved issues.

## Phase 6: Review and Continue

After Codex commits:

1. The user returns to ChatGPT with the commit or result.
2. ChatGPT pulls or inspects the updated repo.
3. ChatGPT verifies the generated section and dependency routes.
4. ChatGPT identifies the next section from the approved spine.
5. Repeat from Phase 1.

## Commit Message Convention

For section-generation commits, prefer:

```text
Add <volume> <chapter> <section> notes
```

Example:

```text
Add Volume I propositional normal forms notes
```
