"""
generators/stub_volume.py
Generates files for a volume stub. Returns dict of {filename: content}.
Does not write to disk.
"""

import yaml
from auditor import client, loader


def generate_stub_volume(
    volume_identifier: str,
    volume_display_title: str,
    volume_scope: str,
    chapter_registry: list[dict],
) -> dict[str, str]:
    """
    Args:
        volume_identifier:    e.g. "volume-iii".
        volume_display_title: e.g. "Analysis and Topology".
        volume_scope:         Prose description of the volume's mathematical territory.
        chapter_registry:     Ordered list of {"subject": ..., "display_title": ...}.

    Returns:
        Dict mapping relative file paths to file contents.
    """
    base_prompt   = loader.prompt("generate_stub_volume")
    registry_yaml = yaml.dump(chapter_registry, default_flow_style=False, allow_unicode=True)

    user = (
        f"## Volume Identifier\n\n{volume_identifier}\n\n"
        f"## Volume Display Title\n\n{volume_display_title}\n\n"
        f"## Volume Scope\n\n{volume_scope}\n\n"
        f"## Chapter Registry\n\n```yaml\n{registry_yaml}\n```"
    )

    raw = client.call(base_prompt, user, expect_json=False)
    return _parse_file_blocks(raw)


def _parse_file_blocks(raw: str) -> dict[str, str]:
    import re
    files: dict[str, str] = {}

    pattern = re.compile(
        r"###\s+File:\s+(.+?)\n+```(?:latex|yaml|tex)?\n(.*?)```",
        re.DOTALL,
    )

    for match in pattern.finditer(raw):
        filename = match.group(1).strip()
        content  = match.group(2)
        files[filename] = content

    if not files:
        files["_raw_output"] = raw

    return files
