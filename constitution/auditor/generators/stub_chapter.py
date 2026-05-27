"""
generators/stub_chapter.py
Generates all files for a chapter stub. Returns dict of {filename: content}.
Does not write to disk — caller decides.
"""

import yaml
from auditor import client, loader


def generate_stub_chapter(
    volume_path: str,
    chapter_subject: str,
    chapter_display_title: str,
    chapter_registry: list[dict],
    sections_known: bool = False,
) -> dict[str, str]:
    """
    Args:
        volume_path:            e.g. "volume-iii".
        chapter_subject:        Repository identifier (e.g. "bounds").
        chapter_display_title:  Display title.
        chapter_registry:       Full volume registry in dependency order.
        sections_known:         Whether section stubs already exist.

    Returns:
        Dict mapping relative file paths to file contents.
        e.g. {"bounds/index.tex": "...", "bounds/chapter.yaml": "..."}
    """
    base_prompt   = loader.prompt("generate_stub_chapter")
    registry_yaml = yaml.dump(chapter_registry, default_flow_style=False, allow_unicode=True)

    user = (
        f"## Volume Path\n\n{volume_path}\n\n"
        f"## Chapter Subject\n\n{chapter_subject}\n\n"
        f"## Chapter Display Title\n\n{chapter_display_title}\n\n"
        f"## Sections Known\n\n{'YES' if sections_known else 'NO'}\n\n"
        f"## Chapter Registry\n\n```yaml\n{registry_yaml}\n```"
    )

    # Generator returns a structured response we parse into files.
    # The prompt instructs the model to label each file clearly.
    raw = client.call(base_prompt, user, expect_json=False)
    return _parse_file_blocks(raw)


def _parse_file_blocks(raw: str) -> dict[str, str]:
    """
    Parses the model output into a dict of {filename: content}.
    The generate-stub-chapter prompt instructs the model to clearly label
    each file with a line like:
        ### File: {chapter}/index.tex
    followed by a fenced code block.
    """
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
        # Fallback: return raw as a single block under a sentinel key
        files["_raw_output"] = raw

    return files
