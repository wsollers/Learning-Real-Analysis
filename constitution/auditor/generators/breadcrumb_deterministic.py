"""Deterministic breadcrumb generator (replaces the LLM-backed generate_breadcrumb).

The breadcrumb is a pure function of the registry: prior -> **current** -> next,
by dependency-order index, with first/last/single edge cases. No model call.

This is the streamlined replacement for generators/breadcrumb.py. Migration:
  1. point the breadcrumb call site here (render_breadcrumb);
  2. ensure common/breadcrumb-macros.tex is \input from the preamble;
  3. delete the generate-breadcrumb prompt and the old LLM path.
Functionality is preserved (same chain, same edge cases, same stub box) and made
deterministic; footprint drops by one prompt + one client.call path.
"""
from __future__ import annotations


def _tex(s: str) -> str:
    # minimal ASCII-safe escaping for display titles inside \text{...}
    return (s.replace("\\", r"\textbackslash{}").replace("&", r"\&")
             .replace("%", r"\%").replace("#", r"\#").replace("_", r"\_"))


def render_breadcrumb(subject: str, current_title: str,
                      registry: list[dict], is_stub: bool = False) -> str:
    """Return the raw LaTeX `\\breadcrumb{...}` call (plus `\\stubstatus` if stub).

    Args:
        subject:        chapter subject id (registry key for the current chapter).
        current_title:  display title fallback if subject is absent from registry.
        registry:       volume registry in dependency order; entries carry
                        {"subject": str, "display_title": str}.
        is_stub:        append the Status: Planned box.
    """
    subjects = [e["subject"] for e in registry]
    titles = [e["display_title"] for e in registry]
    if subject in subjects:
        i = subjects.index(subject)
        cur = titles[i]
        prior = titles[i - 1] if i > 0 else ""
        nxt = titles[i + 1] if i < len(titles) - 1 else ""
    else:  # subject not in registry: degrade to current-only
        cur, prior, nxt = current_title, "", ""
    out = (f"\\breadcrumb{{{_tex(subject)}}}{{{_tex(prior)}}}"
           f"{{{_tex(cur)}}}{{{_tex(nxt)}}}")
    if is_stub:
        out += "\n\\stubstatus"
    return out
