"""
generators/proof.py
Generates a proof file. Returns raw LaTeX.
"""

from auditor import client, loader


def label_root(label: str) -> str:
    return label.split(":", 1)[1] if ":" in label else label


def proof_label_for(theorem_label: str) -> str:
    return f"prf:{label_root(theorem_label)}"


def generate_proof_stub_latex(
    theorem_label: str,
    theorem_name: str,
    theorem_statement: str,
) -> str:
    proof_label = proof_label_for(theorem_label)
    statement = theorem_statement.strip()
    return (
        "\\newpage\n"
        "\\phantomsection\n"
        f"\\label{{{proof_label}}}\n\n"
        "\\begin{remark*}[Return]\n"
        f"\\hyperref[{theorem_label}]{{Return to Theorem}}\n"
        "\\end{remark*}\n\n"
        f"\\begin{{theorem*}}[{theorem_name}]\n"
        f"{statement}\n"
        "\\end{theorem*}\n\n"
        "\\begin{proof}\n"
        "TODO\n"
        "\\end{proof}\n"
    )


def generate_proof(
    theorem_label: str,
    theorem_name: str,
    theorem_statement: str,
    mode: str = "stub",
    proof_content: str = "",
) -> str:
    base_prompt = loader.prompt("generate_proof")

    user = (
        f"## Theorem Label\n\n{theorem_label}\n\n"
        f"## Theorem Name\n\n{theorem_name}\n\n"
        f"## Mode\n\n{mode}\n\n"
        f"## Theorem Statement\n\n```latex\n{theorem_statement}\n```\n\n"
        f"## Proof Content\n\n{proof_content or '(none)'}"
    )

    return client.call(base_prompt, user, expect_json=False)
