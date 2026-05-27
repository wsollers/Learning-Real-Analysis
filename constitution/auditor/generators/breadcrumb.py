"""
generators/breadcrumb.py
Generates a breadcrumb tcolorbox. Returns raw LaTeX string.
"""

import yaml
from auditor import client, loader


def generate_breadcrumb(
    chapter_subject: str,
    chapter_display_title: str,
    chapter_registry: list[dict],
    is_stub: bool = False,
) -> str:
    """
    Args:
        chapter_subject:       Repository identifier (e.g. "bounds").
        chapter_display_title: Display title (e.g. "Bounds and Extremals").
        chapter_registry:      Full volume registry in dependency order.
                               Each entry: {"subject": str, "display_title": str}.
        is_stub:               Whether to append a Status: Planned box.

    Returns:
        Raw LaTeX string.
    """
    base_prompt   = loader.prompt("generate_breadcrumb")
    registry_yaml = yaml.dump(chapter_registry, default_flow_style=False, allow_unicode=True)

    user = (
        f"## Chapter Subject\n\n{chapter_subject}\n\n"
        f"## Chapter Display Title\n\n{chapter_display_title}\n\n"
        f"## Is Stub\n\n{'YES' if is_stub else 'NO'}\n\n"
        f"## Chapter Registry\n\n```yaml\n{registry_yaml}\n```"
    )

    return client.call(base_prompt, user, expect_json=False)
