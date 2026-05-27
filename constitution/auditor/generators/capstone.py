"""
generators/capstone.py
Generates a capstone exercise file. Returns raw LaTeX string.
"""

import yaml
from auditor import client, loader


def generate_capstone(
    chapter_subject: str,
    chapter_display_title: str,
    chapter_registry: list[dict],
    chapter_environments: list[dict],
    mode: str = "stub",
) -> str:
    """
    Args:
        chapter_subject:        Repository identifier (e.g. "bounds").
        chapter_display_title:  Display title.
        chapter_registry:       Full volume registry in dependency order.
        chapter_environments:   Environments from chapter.yaml
                                [{"label": ..., "type": ..., "display_title": ...}].
        mode:                   "stub" (default) or "full".

    Returns:
        Raw LaTeX string (complete capstone file contents).
    """
    base_prompt      = loader.prompt("generate_capstone")
    registry_yaml    = yaml.dump(chapter_registry,    default_flow_style=False, allow_unicode=True)
    environments_yaml = yaml.dump(chapter_environments, default_flow_style=False, allow_unicode=True)

    user = (
        f"## Chapter Subject\n\n{chapter_subject}\n\n"
        f"## Chapter Display Title\n\n{chapter_display_title}\n\n"
        f"## Mode\n\n{mode.upper()}\n\n"
        f"## Chapter Registry\n\n```yaml\n{registry_yaml}\n```\n\n"
        f"## Chapter Environments\n\n```yaml\n{environments_yaml}\n```"
    )

    return client.call(base_prompt, user, expect_json=False)
