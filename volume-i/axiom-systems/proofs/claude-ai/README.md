# Claude-AI Proof Stubs

These are blank proof worksheets generated for the student to complete
via Socratic dialogue with Claude.

## Naming convention
`RA-TAO-C{CHAPTER}-S{SECTION}-E{EXERCISE}.tex`

## How to add a new proof stub

1. Create `RA-TAO-C02-S{X}-E{N}.tex` in this folder using the template below.
2. Add `\input{axiom-systems/proofs/claude-ai/RA-TAO-...}` to `claude-ai/index.tex`.
3. Add a `\proofrow` line to `axiom-systems/worksheets/tao-registry.tex`.

That is all. The worksheet table and navigation links update automatically.

## Blank proof template

```latex
\clearpage
\phantomsection
\hypertarget{proof-RA-TAO-C02-SX-EN}{}

\section*{Proof --- RA-TAO-C02-SX-EN}

\noindent
\hyperlink{ws-RA-TAO-C02-SX-EN}{\textbf{← Back to worksheet}}

\bigskip

\noindent\textbf{Source.}
\srccite{TaoAnalysis1}{Chapter~2, \SX, Exercise~N}.

\vspace{0.75em}

\noindent\textbf{Goal.}
[State the proposition to be proved.]

\vspace{0.75em}

\noindent\textbf{Available toolkit.}
[List axioms, definitions, lemmas available at this point.]

\vspace{0.75em}

\noindent\textbf{Strategy.}

\vspace{3em}

\noindent\textbf{$P(n)$ chosen as} (if inducting):

\vspace{2em}

\noindent\rule{\textwidth}{0.4pt}
\noindent\textbf{Proof.}

\begin{tabular}{T S J}
\toprule
\textbf{Tag} & \multicolumn{1}{p{0.44\textwidth}}{\textbf{Step}} & \textbf{Justification} \\
\midrule
\addlinespace[4pt]
& & \\[120pt]
\bottomrule
\end{tabular}

\noindent
\hyperlink{ws-RA-TAO-C02-SX-EN}{\textbf{← Back to worksheet}}
```
