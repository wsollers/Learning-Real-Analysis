\newpage
\phantomsection
\label{prf:ec-multiplication}

\begin{remark*}[Return]
\hyperref[thm:ec-multiplication]{Return to Theorem}
\end{remark*}

\begin{theorem*}[Multiplication of Equivalence Classes]
For every $[(a,b)],[(c,d)]\in\mathbb{Q}$,
\[
[(a,b)]\cdot[(c,d)] = [(ac,\,bd)].
\]
\end{theorem*}

\begin{proof}
\textbf{Professional Standard Proof.}~\\
Let $[(a,b)],[(c,d)]\in\mathbb{Q}$. By the definition of multiplication
on rational equivalence classes,
\[
[(a,b)]\cdot[(c,d)] := [(ac,\,bd)].
\]
Hence
\[
[(a,b)]\cdot[(c,d)] = [(ac,\,bd)].
\]
\end{proof}

\begin{proof}
\textbf{Detailed Learning Proof.}~\\

\textbf{Step 1.} Unfold the multiplication operation on rational equivalence classes.

The rational operations have already been defined on equivalence classes of
integer pairs. In particular, multiplication is defined by multiplying numerators
and multiplying denominators:
\[
[(a,b)]\cdot[(c,d)] := [(ac,\,bd)].
\]

\textbf{Step 2.} Record the defining formula as an equality.

Since the expression on the left is defined to be the equivalence class on the
right, it follows immediately that
\[
[(a,b)]\cdot[(c,d)] = [(ac,\,bd)].
\]
\end{proof}

\begin{remark*}[Proof structure]
The argument is a direct proof by unfolding a definition. The theorem does not
establish a new algebraic law; it records the defining multiplication rule for
rational equivalence classes as an explicit identity. The well-definedness result
is the prior justification that this rule is independent of the representatives
chosen for the two rational numbers.
\end{remark*}

\begin{remark*}[Dependencies]~\\
\begin{itemize}
  \item \hyperref[def:rational-operations]{Definition~\ref*{def:rational-operations}}
        \textnormal{(definition of multiplication on rational equivalence classes)}
  \item \hyperref[lem:rational-multiplication-well-defined]{Lemma~\ref*{lem:rational-multiplication-well-defined}}
        \textnormal{(well-definedness of multiplication)}
\end{itemize}
\end{remark*}

Do not use `topicbox` containers in proof files.
Do not use `exposition` environments in proof files.
Proof files must preserve the existing two-layer proof structure:
professional proof first, detailed learning proof second.
At the top of every proof file, emit:
- `\newpage`
- `\phantomsection`
- a proof label of the form `\label{prf-...}`
- `\begin{remark*}[Return] ... \end{remark*}` with a hyperref back to the
  source theorem/corollary/lemma/proposition
Use starred statement environments in proof files, e.g. `theorem*` or
`corollary*`, not the numbered statement environments from the note body.
