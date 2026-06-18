"""
generators/proof.py
Generates a proof file. Returns raw LaTeX.
"""

from auditor import client, loader


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
