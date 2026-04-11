#!/usr/bin/env python3
r"""
extract-knowledge.py -- Extract theorem/definition explorer data from LaTeX notes.

Primary goal:
    Build canonical theorem/definition nodes plus graph edges from \Flash* macros
    in note files under ../volume*/**/notes/**.tex.

Secondary goal:
    Derive flashcard-like records from the same canonical node data.

Default behavior:
    When run from a repo script directory, this script looks in sibling directories
    matching ../volume* and recurses only into note files contained in a `notes/`
    path component. Proof enrichment is pulled from sibling proofs/notes folders.

Typical usage:
    python scripts/extract-knowledge.py
    python scripts/extract-knowledge.py --output knowledge.json
    python scripts/extract-knowledge.py --cards-output cards.json --audit flash-audit.md
    python scripts/extract-knowledge.py --roots ../volume-iii ../volume-iv --verbose
"""

from __future__ import annotations

import argparse
import datetime as _dt
import base64
import json
import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

ENV_TYPES = {
    "definition": "Definition",
    "theorem": "Theorem",
    "lemma": "Lemma",
    "proposition": "Proposition",
    "corollary": "Corollary",
    "axiom": "Axiom",
}

SKIP_DIRS = {".git", "node_modules", "_minted", "_archive", ".obsidian", ".venv", "venv", "dist", "build"}
SKIP_FILES = {"main.tex", "preamble.tex", "macros.tex", "environments.tex", "boxes.tex", "colors.tex"}

FLASH_FIELDS = [
    "FlashQ",
    "FlashA",
    "FlashHint",
    "FlashNeg",
    "FlashImplies",
    "FlashProp",
    "FlashFormal",
    "FlashNegFormal",
    "FlashContra",
    "FlashDependsOn",
    "FlashUsedBy",
    "FlashPrereq",
    "FlashEquivalentTo",
    "FlashAliases",
    "FlashTags",
    "FlashSymbol",
    "FlashCategory",
    "FlashProofIdea",
    "FlashProofType",
    "FlashMethod",
    "FlashSourceText",
    "FlashSection",
    "FlashIgnore",
]

PROOF_FLASH_FIELDS = [
    "FlashProof",
    "FlashUses",
    "FlashImplies",
    "FlashDependsOn",
    "FlashUsedBy",
    "FlashPrereq",
    "FlashEquivalentTo",
    "FlashProofIdea",
    "FlashProofType",
    "FlashMethod",
]

EDGE_KINDS = {"depends_on", "used_by", "implies", "prereq", "equivalent_to"}


# ---------------------------------------------------------------------------
# Notation profile + predicate library
# ---------------------------------------------------------------------------

NOTATION_PROFILE = {
    "id": "em-real-analysis-v1",
    "set": {"symbol": "S", "latex": "S"},
    "subset": {"symbol": "A", "latex": "A"},
    "metric_space": {"symbol": "M", "latex": r"(M,d)"},
    "metric": {"symbol": "d", "latex": "d"},
    "sequence": {"symbol": "x_n", "latex": r"(x_n)"},
    "subsequence": {"symbol": "x_{n_k}", "latex": r"(x_{n_k})"},
    "limit": {"symbol": "L", "latex": "L"},
    "supremum": {"symbol": "s", "latex": "s"},
    "infimum": {"symbol": "t", "latex": "t"},
    "upper_bound": {"symbol": "u", "latex": "u"},
    "lower_bound": {"symbol": "l", "latex": "l"},
}

PREDICATES = {
    "NonemptySet": {"arity": 1, "args": ["S"], "latex": r"\operatorname{NonemptySet}(S)"},
    "BoundedAbove": {"arity": 1, "args": ["S"], "latex": r"\operatorname{BoundedAbove}(S)"},
    "BoundedBelow": {"arity": 1, "args": ["S"], "latex": r"\operatorname{BoundedBelow}(S)"},
    "BoundedSet": {"arity": 1, "args": ["S"], "latex": r"\operatorname{BoundedSet}(S)"},
    "UpperBound": {"arity": 2, "args": ["u", "S"], "latex": r"\operatorname{UpperBound}(u,S)"},
    "LowerBound": {"arity": 2, "args": ["l", "S"], "latex": r"\operatorname{LowerBound}(l,S)"},
    "Supremum": {"arity": 2, "args": ["s", "S"], "latex": r"\operatorname{Supremum}(s,S)"},
    "Infimum": {"arity": 2, "args": ["t", "S"], "latex": r"\operatorname{Infimum}(t,S)"},
    "ConvergentSequence": {"arity": 2, "args": ["x_n", "L"], "latex": r"\operatorname{ConvergentSequence}(x_n,L)"},
    "DivergentSequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{DivergentSequence}(x_n)"},
    "BoundedSequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{BoundedSequence}(x_n)"},
    "EventuallyBoundedSequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{EventuallyBoundedSequence}(x_n)"},
    "CauchySequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{CauchySequence}(x_n)"},
    "RapidlyCauchySequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{RapidlyCauchySequence}(x_n)"},
    "Subsequence": {"arity": 2, "args": ["x_{n_k}", "x_n"], "latex": r"\operatorname{Subsequence}(x_{n_k},x_n)"},
    "MonotoneIncreasingSequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{MonotoneIncreasingSequence}(x_n)"},
    "MonotoneDecreasingSequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{MonotoneDecreasingSequence}(x_n)"},
    "NondecreasingSequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{NondecreasingSequence}(x_n)"},
    "NonincreasingSequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{NonincreasingSequence}(x_n)"},
    "ConvergesToInfinity": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{ConvergesToInfinity}(x_n)"},
    "ConvergesToNegInfinity": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{ConvergesToNegInfinity}(x_n)"},
    "HasSubsequenceConvergingTo": {"arity": 2, "args": ["x_n", "L"], "latex": r"\operatorname{HasSubsequenceConvergingTo}(x_n,L)"},
    "HasConvergentSubsequence": {"arity": 1, "args": ["x_n"], "latex": r"\operatorname{HasConvergentSubsequence}(x_n)"},
    "ContractiveSequence": {"arity": 2, "args": ["x_n", "c"], "latex": r"\operatorname{ContractiveSequence}(x_n,c)"},
    "EventuallyProperty": {"arity": 2, "args": ["x_n", "P"], "latex": r"\operatorname{EventuallyProperty}(x_n,P)"},
}

def b64utf8_encode(s: str) -> str:
    return base64.b64encode((s or "").encode("utf-8")).decode("ascii")

def dual_field(plain: str, latex: str):
    return {"plain": clean_text(plain), "latex_b64": b64utf8_encode(latex or "")}

def _canon_latex(s: str) -> str:
    return re.sub(r"\s+", " ", (s or "").strip())

def parse_predicates_from_formal(formal: str, statement: str = "", proposition: str = ""):
    """Parse recognized mathematical forms into canonical predicate instances."""
    text = " ".join([formal or "", proposition or "", statement or ""])
    t = _canon_latex(text)
    preds = []

    def add(name, args):
        item = {"name": name, "args": args, "latex": PREDICATES.get(name, {}).get("latex", "")}
        if item not in preds:
            preds.append(item)

    for pname in PREDICATES:
        m = re.search(rf"\\operatorname\{{{re.escape(pname)}\}}\((.*?)\)", t)
        if m:
            args = [a.strip() for a in m.group(1).split(",") if a.strip()]
            add(pname, args)

    if re.search(r"\\forall\s+\\varepsilon\s*>\s*0.*\\exists\s+N\s*\\in\s*\\mathbb\{N\}.*\\forall\s+n\s*\\ge\s*N.*\|x_n\s*-\s*L\|\s*<\s*\\varepsilon", t):
        add("ConvergentSequence", ["x_n", "L"])
    if re.search(r"\\forall\s+\\varepsilon\s*>\s*0.*\\exists\s+N\s*\\in\s*\\mathbb\{N\}.*\\forall\s+m\s*,\s*n\s*\\ge\s*N.*\|x_n\s*-\s*x_m\|\s*<\s*\\varepsilon", t):
        add("CauchySequence", ["x_n"])
    if re.search(r"\\exists\s+M(?:\s*>\s*0)? .*\\forall\s+n\s*\\in\s*\\mathbb\{N\}.*\|x_n\|\s*\\le\s*M", t):
        add("BoundedSequence", ["x_n"])
    if re.search(r"\\exists\s+M(?:\s*>\s*0)? .*\\exists\s+N\s*\\in\s*\\mathbb\{N\}.*\\forall\s+n\s*\\ge\s*N.*\|x_n\|\s*\\le\s*M", t):
        add("EventuallyBoundedSequence", ["x_n"])
        add("EventuallyProperty", ["x_n", r"|x_n| \le M"])
    if re.search(r"n_k\s*<\s*n_\{k\+1\}", t) or re.search(r"strictly increasing.*n_k", t, re.I):
        add("Subsequence", ["x_{n_k}", "x_n"])
    if re.search(r"\\forall\s+n.*x_n\s*\\le\s*x_\{n\+1\}", t):
        add("MonotoneIncreasingSequence", ["x_n"])
        add("NondecreasingSequence", ["x_n"])
    if re.search(r"\\forall\s+n.*x_\{n\+1\}\s*\\le\s*x_n", t):
        add("MonotoneDecreasingSequence", ["x_n"])
        add("NonincreasingSequence", ["x_n"])
    if re.search(r"\\forall\s+M\s*\\in\s*\\mathbb\{R\}.*\\exists\s+N.*\\forall\s+n\s*\\ge\s*N.*x_n\s*>\s*M", t):
        add("ConvergesToInfinity", ["x_n"])
    if re.search(r"\\forall\s+M\s*\\in\s*\\mathbb\{R\}.*\\exists\s+N.*\\forall\s+n\s*\\ge\s*N.*x_n\s*<\s*M", t):
        add("ConvergesToNegInfinity", ["x_n"])
    if re.search(r"S\\neq\\varnothing|A\\neq\\varnothing", t):
        add("NonemptySet", ["S"])
    if re.search(r"\\exists\s+u\s*\\in\s*\\mathbb\{R\}.*\\forall\s+x\s*\\in\s*S.*x\s*\\le\s*u", t) or re.search(r"upper bound", t, re.I):
        add("BoundedAbove", ["S"])
        add("UpperBound", ["u", "S"])
    if re.search(r"\\exists\s+l\s*\\in\s*\\mathbb\{R\}.*\\forall\s+x\s*\\in\s*S.*l\s*\\le\s*x", t) or re.search(r"lower bound", t, re.I):
        add("BoundedBelow", ["S"])
        add("LowerBound", ["l", "S"])
    if "BoundedAbove" in [p["name"] for p in preds] and "BoundedBelow" in [p["name"] for p in preds]:
        add("BoundedSet", ["S"])
    if re.search(r"\\forall\s+u.*UpperBound.*s\s*\\le\s*u", t) or re.search(r"supremum", t, re.I):
        add("Supremum", ["s", "S"])
    if re.search(r"\\forall\s+l.*LowerBound.*l\s*\\le\s*t", t) or re.search(r"infimum", t, re.I):
        add("Infimum", ["t", "S"])

    st = _canon_latex(statement)
    if re.search(r"convergent sequence.*bounded|if .* converges .* then .* bounded", st, re.I):
        add("ConvergentSequence", ["x_n", "L"]); add("BoundedSequence", ["x_n"])
    if re.search(r"convergent sequence.*cauchy|if .* converges .* then .* cauchy", st, re.I):
        add("ConvergentSequence", ["x_n", "L"]); add("CauchySequence", ["x_n"])
    if re.search(r"cauchy sequence.*bounded", st, re.I):
        add("CauchySequence", ["x_n"]); add("BoundedSequence", ["x_n"])
    if re.search(r"bounded.*convergent subsequence|bolzano", st, re.I):
        add("BoundedSequence", ["x_n"]); add("HasConvergentSubsequence", ["x_n"])

    return preds

# ---------------------------------------------------------------------------
# Cleaning helpers
# ---------------------------------------------------------------------------


def clean_name(raw: str) -> str:
    if not raw:
        return ""
    m = re.search(r"\\texorpdfstring\s*\{[^}]*\}\s*\{([^}]+)\}", raw)
    if m:
        return m.group(1).strip()
    m = re.search(r"\\hyperref\[[^\]]*\]\{", raw)
    if m:
        start = m.end()
        depth, i = 1, start
        while i < len(raw) and depth:
            if raw[i] == "{":
                depth += 1
            elif raw[i] == "}":
                depth -= 1
            i += 1
        return raw[start : i - 1].strip()
    return raw.strip()


def clean_body(raw: str) -> str:
    text = re.sub(r"\\label\{[^}]+\}", "", raw)
    text = text.replace("\\'", "'").replace("\\`", "`")
    text = re.sub(r"\\hyperref\[[^\]]*\]\{([^}]+)\}", r"\1", text)
    text = re.sub(
        r"\\(?:c?ref|pageref)\{([^}]+)\}",
        lambda m: m.group(1).split(":")[-1].replace("-", " "),
        text,
    )
    text = re.sub(r"\\(?:Claim|Strategy|Let|Hence|AsReq|ByThm|prooftag)\b\s*", "", text)
    text = text.replace("---", "—").replace("--", "–")
    text = text.replace("\\ldots", "…").replace("\\dots", "…")
    text = text.replace("\\lq\\lq", "“").replace("''", "”")

    for _ in range(4):
        def repl_enum(m):
            items = re.split(r"\\item\b", m.group(1))
            lis = "".join(f"<li>{i.strip()}</li>" for i in items if i.strip())
            return f'<ol style="margin:.6em 0 .6em 1.5em">{lis}</ol>'

        def repl_item(m):
            items = re.split(r"\\item\b", m.group(1))
            lis = "".join(f"<li>{i.strip()}</li>" for i in items if i.strip())
            return f'<ul style="margin:.6em 0 .6em 1.5em">{lis}</ul>'

        new = re.sub(r"\\begin\{enumerate\}(?:\[[^\]]*\])?(.*?)\\end\{enumerate\}", repl_enum, text, flags=re.DOTALL)
        new = re.sub(r"\\begin\{itemize\}(.*?)\\end\{itemize\}", repl_item, new, flags=re.DOTALL)
        if new == text:
            break
        text = new

    text = re.sub(r"\\emph\{([^}]+)\}", r"<em>\1</em>", text)
    text = re.sub(r"\\textbf\{([^}]+)\}", r"<strong>\1</strong>", text)
    text = re.sub(r"\\textit\{([^}]+)\}", r"<em>\1</em>", text)
    text = re.sub(r"\\(no)?indent\b", "", text)
    text = re.sub(r"\\(med|small|big)skip\b", "<br>", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def clean_text(s: Optional[str]) -> str:
    if not s:
        return ""
    s = s.replace("\\'", "'").replace("\\`", "`")
    s = s.replace("---", "—").replace("--", "–")
    s = s.replace("\\ldots", "…").replace("\\dots", "…")
    return s.strip()


def slugish(s: str) -> str:
    return re.sub(r"\s+", " ", clean_text(s)).strip()


# ---------------------------------------------------------------------------
# Extraction helpers
# ---------------------------------------------------------------------------


def extract_flash_macro(text: str, macro: str):
    pattern = re.compile(r"\\" + re.escape(macro) + r"\{")
    m = pattern.search(text)
    if not m:
        return None if macro != "FlashIgnore" else (r"\FlashIgnore" in text)
    start = m.end()
    depth, i = 1, start
    while i < len(text) and depth:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
        i += 1
    return text[start : i - 1].strip()


def extract_flash_block(tex: str, end_pos: int) -> Optional[str]:
    tail = tex[end_pos:]
    pattern = re.compile(
        r"(?:(?!\\begin\{(?:theorem|lemma|proposition|corollary|definition|axiom)\b).){0,4000}"
        r"\\begin\{remark\*?\}\s*\[Flash\](.*?)\\end\{remark\*?\}",
        re.DOTALL,
    )
    m = pattern.match(tail)
    return m.group(1) if m else None


def extract_fully_quantified_form(tex: str, end_pos: int) -> str:
    fqf_pat = re.compile(
        r"(?:(?!\\begin\{(?:theorem|lemma|proposition|corollary|definition|axiom)\b)"
        r"(?!\\begin\{remark\*?\}\s*\[Flash\])"
        r".){0,1500}"
        r"\\begin\{remark\*?\}\s*\[Fully quantified form\](.*?)\\end\{remark\*?\}",
        re.DOTALL,
    )
    m = fqf_pat.match(tex[end_pos:])
    return clean_body(m.group(1)) if m else ""


def extract_proof_flash(tex: str) -> Dict[str, str]:
    result: Dict[str, str] = {}
    for macro in PROOF_FLASH_FIELDS:
        val = extract_flash_macro(tex, macro)
        if val:
            result[macro] = clean_body(val)
    return result


def split_csv_labels(raw: str) -> List[str]:
    if not raw:
        return []
    parts = [p.strip() for p in raw.split(",")]
    return [p for p in parts if p]


def split_csv_text(raw: str) -> List[str]:
    if not raw:
        return []
    return [clean_text(p) for p in raw.split(",") if clean_text(p)]


def discover_default_roots(script_path: Path) -> List[Path]:
    base = script_path.resolve().parent.parent
    roots = sorted(p.resolve() for p in base.glob("volume*") if p.is_dir())
    return roots


def file_is_note(tex_file: Path) -> bool:
    parts = set(tex_file.parts)
    return "notes" in parts and tex_file.name not in SKIP_FILES and tex_file.suffix.lower() == ".tex"


def find_proof_file(label_root: str, note_file: Path, roots: List[Path]) -> Tuple[Optional[Path], Optional[str]]:
    try:
        parts = note_file.parts
        notes_idx = next(i for i, p in enumerate(parts) if p == "notes")
        chapter_dir = Path(*parts[:notes_idx])
        proof_dir = chapter_dir / "proofs" / "notes"
        if not proof_dir.exists():
            return None, None
        for fname in sorted(proof_dir.glob("*.tex")):
            if fname.name == "index.tex":
                continue
            content = fname.read_text(encoding="utf-8", errors="replace")
            if f"\\label{{prf:{label_root}}}" in content:
                return fname, content
    except Exception:
        return None, None
    return None, None


def infer_deck(tex_file: Path, root: Path) -> str:
    rel = tex_file.relative_to(root)
    skip = {"notes", "proofs", "exercises"}
    rel_parts = [p for p in rel.parts[:-1] if p not in skip and not p.endswith(".tex")]

    def fmt(s: str) -> str:
        s = s.replace("-", " ").replace("_", " ").title()
        return re.sub(r"^Volume ([IVXivx]+)$", lambda m: "Vol " + m.group(1).upper(), s)

    formatted = [fmt(p) for p in rel_parts]
    if len(formatted) >= 3:
        return formatted[2]
    return " · ".join(formatted) if formatted else tex_file.stem


# ---------------------------------------------------------------------------
# Canonical node construction
# ---------------------------------------------------------------------------


def build_node(*, label: str, env_type: str, name: str, deck: str, source: str, body_raw: str,
               body_display: str, flash: Dict[str, str], proof_flash: Dict[str, str], fqf: str,
               proof_file: Optional[Path]) -> Dict:
    title = name or label
    is_thm_like = env_type in {"Theorem", "Lemma", "Proposition", "Corollary"}

    statement_display = clean_text(flash.get("FlashA")) or body_display
    statement_tex = flash.get("FlashA") or body_raw
    proposition = clean_text(flash.get("FlashProp"))
    formal = clean_text(flash.get("FlashFormal")) or fqf
    negation_display = clean_text(flash.get("FlashNeg"))
    negation_formal = clean_text(flash.get("FlashNegFormal"))
    contrapositive = clean_text(flash.get("FlashContra"))
    hint = clean_text(flash.get("FlashHint"))
    question = clean_text(flash.get("FlashQ")) or f"State the {env_type.lower()}: {title}."

    depends_on_ids = split_csv_labels(clean_text(flash.get("FlashDependsOn") or proof_flash.get("FlashDependsOn", "")))
    used_by_ids = split_csv_labels(clean_text(flash.get("FlashUsedBy") or proof_flash.get("FlashUsedBy", "")))
    prereq_ids = split_csv_labels(clean_text(flash.get("FlashPrereq") or proof_flash.get("FlashPrereq", "")))
    equivalent_to_ids = split_csv_labels(clean_text(flash.get("FlashEquivalentTo") or proof_flash.get("FlashEquivalentTo", "")))
    implies_ids = split_csv_labels(clean_text(flash.get("FlashImplies") or proof_flash.get("FlashImplies", "")))

    predicate_instances = parse_predicates_from_formal(formal=formal or fqf, statement=statement_tex, proposition=proposition)

    node = {
        "id": label,
        "kind": env_type,
        "name": title,
        "deck": deck,
        "source": source,
        "proof_source": str(proof_file) if proof_file else "",
        "question": question,
        "statement_display": statement_display,
        "statement_tex": statement_tex,
        "hint": hint,
        "proposition": proposition,
        "formal": formal,
        "fully_quantified_form": fqf,
        "negation_display": negation_display,
        "negation_formal": negation_formal,
        "contrapositive": contrapositive,
        "depends_on_ids": depends_on_ids,
        "used_by_ids": used_by_ids,
        "prereq_ids": prereq_ids,
        "equivalent_to_ids": equivalent_to_ids,
        "implies_ids": implies_ids,
        "aliases": split_csv_text(clean_text(flash.get("FlashAliases", ""))),
        "tags": split_csv_text(clean_text(flash.get("FlashTags", ""))),
        "symbol": clean_text(flash.get("FlashSymbol", "")),
        "category": clean_text(flash.get("FlashCategory", "")),
        "proof_sketch": clean_text(proof_flash.get("FlashProof", "")),
        "proof_idea": clean_text(flash.get("FlashProofIdea") or proof_flash.get("FlashProofIdea", "")),
        "proof_type": clean_text(flash.get("FlashProofType") or proof_flash.get("FlashProofType", "")),
        "method": clean_text(flash.get("FlashMethod") or proof_flash.get("FlashMethod", "")),
        "uses_text": clean_text(proof_flash.get("FlashUses", "")),
        "source_text": clean_text(flash.get("FlashSourceText", "")),
        "section": clean_text(flash.get("FlashSection", "")),
        "has_proof_file": bool(proof_file),
        "ignored": bool(flash.get("FlashIgnore", False)),
        "is_theorem_like": is_thm_like,
        "notation_profile": NOTATION_PROFILE["id"],
        "predicates": predicate_instances,
        "structured": {
            "statement": dual_field(statement_display, statement_tex),
            "formal": dual_field(formal or fqf, formal or fqf),
            "negation": dual_field(negation_formal or negation_display, negation_formal or negation_display),
            "contrapositive": dual_field(contrapositive, contrapositive),
            "flash_q": dual_field(question, flash.get("FlashQ") or question),
            "flash_a": dual_field(statement_display, flash.get("FlashA") or statement_tex),
        },
    }
    return node


def derive_cards_from_node(node: Dict) -> List[Dict]:
    cards: List[Dict] = []
    label = node["id"]
    env_type = node["kind"]
    name = node["name"]
    deck = node["deck"]
    source = node["source"]

    cards.append({
        "label": label,
        "type": env_type,
        "name": name,
        "card_type": "statement",
        "question": node["question"],
        "answer": node["statement_display"],
        "hint": node["hint"],
        "deck": deck,
        "source": source,
    })

    formal_front = node["formal"] or node["fully_quantified_form"]
    if formal_front:
        cards.append({
            "label": label,
            "type": env_type,
            "name": name,
            "card_type": "recognition",
            "question": formal_front,
            "answer": f"This is the <strong>{env_type}</strong>: <em>{name}</em>",
            "hint": "",
            "deck": deck,
            "source": source,
        })

    if node["negation_display"] or node["negation_formal"]:
        cards.append({
            "label": label,
            "type": env_type,
            "name": name,
            "card_type": "negation",
            "question": f"Give a failure case or negation for: {name}.",
            "answer": node["negation_formal"] or node["negation_display"],
            "hint": "",
            "deck": deck,
            "source": source,
        })

    if node["contrapositive"]:
        cards.append({
            "label": label,
            "type": env_type,
            "name": name,
            "card_type": "contrapositive",
            "question": f"State a contrapositive form of: {name}.",
            "answer": node["contrapositive"],
            "hint": "",
            "deck": deck,
            "source": source,
        })

    if node["implies_ids"]:
        cards.append({
            "label": label,
            "type": env_type,
            "name": name,
            "card_type": "implies",
            "question": f"What does {name} immediately give you?",
            "answer": ", ".join(node["implies_ids"]),
            "hint": "",
            "deck": deck,
            "source": source,
        })

    if node["proof_sketch"]:
        cards.append({
            "label": label,
            "type": env_type,
            "name": name,
            "card_type": "proof_sketch",
            "question": f"Sketch the proof of: {name}.",
            "answer": node["proof_sketch"],
            "hint": node["proof_idea"],
            "deck": deck,
            "source": source,
        })

    if node["uses_text"]:
        cards.append({
            "label": label,
            "type": env_type,
            "name": name,
            "card_type": "uses",
            "question": f"What does the proof of {name} use?",
            "answer": node["uses_text"],
            "hint": node["method"],
            "deck": deck,
            "source": source,
        })
    return cards


def edges_from_node(node: Dict) -> List[Dict]:
    out: List[Dict] = []
    src = node["id"]
    for tgt in node.get("depends_on_ids", []):
        out.append({"from": src, "to": tgt, "kind": "depends_on"})
    for tgt in node.get("used_by_ids", []):
        out.append({"from": src, "to": tgt, "kind": "used_by"})
    for tgt in node.get("implies_ids", []):
        out.append({"from": src, "to": tgt, "kind": "implies"})
    for tgt in node.get("prereq_ids", []):
        out.append({"from": src, "to": tgt, "kind": "prereq"})
    for tgt in node.get("equivalent_to_ids", []):
        out.append({"from": src, "to": tgt, "kind": "equivalent_to"})
    return out


# ---------------------------------------------------------------------------
# Main scan
# ---------------------------------------------------------------------------


def process_note_file(tex_file: Path, root: Path, env_filter: Iterable[str], roots: List[Path], verbose: bool):
    try:
        text = tex_file.read_text(encoding="utf-8", errors="replace")
    except Exception:
        return [], []

    nodes, audits = [], []
    deck = infer_deck(tex_file, root)

    for env_name in env_filter:
        begin_pat = re.compile(r"\\begin\{" + re.escape(env_name) + r"\}", re.IGNORECASE)
        end_pat = re.compile(r"\\end\{" + re.escape(env_name) + r"\}", re.IGNORECASE)

        for m_begin in begin_pat.finditer(text):
            after = m_begin.end()
            opt_raw, after_opt = "", after
            j = after
            while j < len(text) and text[j] in " \t\n":
                j += 1
            if j < len(text) and text[j] == "[":
                depth, k = 0, j + 1
                while k < len(text):
                    if text[k] == "[":
                        depth += 1
                    elif text[k] == "]":
                        if depth == 0:
                            opt_raw = text[j + 1 : k]
                            after_opt = k + 1
                            break
                        depth -= 1
                    k += 1

            m_end = end_pat.search(text, after_opt)
            if not m_end:
                continue

            body_raw = text[after_opt : m_end.start()]
            label_m = re.search(r"\\label\{([^}]+)\}", body_raw)
            if not label_m:
                continue

            label = label_m.group(1)
            label_root = re.sub(r"^[a-z]+:", "", label)
            name = clean_name(opt_raw) or label
            env_type = ENV_TYPES[env_name]
            body_display = clean_body(body_raw)
            fqf = extract_fully_quantified_form(text, m_end.end())

            flash_text = extract_flash_block(text, m_end.end()) or ""
            flash: Dict[str, str] = {}
            for field in FLASH_FIELDS:
                val = extract_flash_macro(flash_text, field)
                if val:
                    flash[field] = val if field == "FlashIgnore" else clean_text(val)

            proof_file, proof_text = find_proof_file(label_root, tex_file, roots)
            proof_flash = extract_proof_flash(proof_text) if proof_text else {}

            node = build_node(
                label=label,
                env_type=env_type,
                name=name,
                deck=deck,
                source=str(tex_file.relative_to(root)),
                body_raw=body_raw.strip(),
                body_display=body_display,
                flash=flash,
                proof_flash=proof_flash,
                fqf=fqf,
                proof_file=proof_file,
            )

            audits.append({
                "id": label,
                "kind": env_type,
                "source": str(tex_file.relative_to(root)),
                "has_flash": bool(flash_text),
                "has_prop": bool(node["proposition"]),
                "has_formal": bool(node["formal"]),
                "has_neg_formal": bool(node["negation_formal"]),
                "has_contra": bool(node["contrapositive"]),
                "has_depends_on": bool(node["depends_on_ids"]),
                "has_used_by": bool(node["used_by_ids"]),
                "has_tags": bool(node["tags"]),
                "has_proof": bool(node["proof_sketch"]),
                "has_uses": bool(node["uses_text"]),
                "ignored": node["ignored"],
            })

            if node["ignored"]:
                continue
            nodes.append(node)

    if verbose and nodes:
        print(f"  {tex_file.relative_to(root)}: {len(nodes)} nodes")
    return nodes, audits


# ---------------------------------------------------------------------------
# Reports
# ---------------------------------------------------------------------------


def generate_audit_report(audit_rows: List[Dict], output_path: Path):
    date = _dt.date.today().isoformat()
    lines = [f"# Knowledge Extraction Audit — {date}\n"]
    lines.append("Symbols: ✓ present · ✗ missing\n")
    by_source = defaultdict(list)
    for row in audit_rows:
        by_source[row["source"]].append(row)

    total = 0
    rich = 0
    for source in sorted(by_source):
        rows = by_source[source]
        lines.append(f"\n## {source}\n")
        lines.append("| ID | Kind | Flash | Prop | Formal | NegFormal | Contra | DependsOn | UsedBy | Tags | Proof | Uses |")
        lines.append("|---|---|---|---|---|---|---|---|---|---|---|---|")
        for r in rows:
            if r["ignored"]:
                lines.append(f"| {r['id']} | {r['kind']} | ignored |  |  |  |  |  |  |  |  |  |")
                continue
            total += 1
            richish = r["has_prop"] and r["has_formal"] and r["has_depends_on"]
            rich += int(richish)
            def sym(b): return "✓" if b else "✗"
            lines.append(
                f"| {r['id']} | {r['kind']} | {sym(r['has_flash'])} | {sym(r['has_prop'])} | {sym(r['has_formal'])} | {sym(r['has_neg_formal'])} | {sym(r['has_contra'])} | {sym(r['has_depends_on'])} | {sym(r['has_used_by'])} | {sym(r['has_tags'])} | {sym(r['has_proof'])} | {sym(r['has_uses'])} |"
            )
    lines.append(f"\n---\n\nGraph-ready core coverage (Prop + Formal + DependsOn): **{rich} / {total}**\n")
    output_path.write_text("\n".join(lines), encoding="utf-8")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main():
    here = Path(__file__).resolve().parent
    p = argparse.ArgumentParser(description="Extract knowledge graph data from LaTeX note files.")
    p.add_argument("--roots", nargs="*", type=Path, default=None, help="Override default scan roots. Default is ../volume* relative to this script.")
    p.add_argument("--output", "-o", type=Path, default=here / "knowledge.json")
    p.add_argument("--cards-output", type=Path, default=here / "cards.json")
    p.add_argument("--edges-output", type=Path, default=here / "graph-edges.json")
    p.add_argument("--audit", type=Path, default=here / "knowledge-audit.md")
    p.add_argument("--env", nargs="+", default=list(ENV_TYPES.keys()))
    p.add_argument("--verbose", "-v", action="store_true")
    args = p.parse_args()

    roots = [r.resolve() for r in args.roots] if args.roots else discover_default_roots(Path(__file__))
    if not roots:
        raise SystemExit("No scan roots found. Pass --roots explicitly or place this script in a repo scripts/ folder next to ../volume* directories.")

    print("Scanning roots:")
    for r in roots:
        print(f"  - {r}")

    env_filter = set(args.env)
    all_nodes: List[Dict] = []
    all_audits: List[Dict] = []

    for root in roots:
        if not root.exists():
            continue
        for tex_file in sorted(root.rglob("*.tex")):
            rel = tex_file.relative_to(root)
            if any(part in SKIP_DIRS for part in rel.parts):
                continue
            if not file_is_note(tex_file):
                continue
            nodes, audits = process_note_file(tex_file, root, env_filter, roots, args.verbose)
            all_nodes.extend(nodes)
            all_audits.extend(audits)

    # Deduplicate nodes by label, preferring richer later node only if old is sparse.
    node_map: Dict[str, Dict] = {}
    for node in all_nodes:
        old = node_map.get(node["id"])
        if old is None:
            node_map[node["id"]] = node
            continue
        old_score = sum(bool(old.get(k)) for k in ["proposition", "formal", "depends_on_ids", "proof_sketch", "uses_text"])
        new_score = sum(bool(node.get(k)) for k in ["proposition", "formal", "depends_on_ids", "proof_sketch", "uses_text"])
        if new_score >= old_score:
            node_map[node["id"]] = node

    nodes = sorted(node_map.values(), key=lambda n: (n["deck"], n["name"], n["id"]))

    # Resolve labels to titles for convenience.
    title_by_id = {n["id"]: n["name"] for n in nodes}
    for n in nodes:
        for field in ["depends_on_ids", "used_by_ids", "prereq_ids", "equivalent_to_ids", "implies_ids"]:
            n[field.replace("_ids", "_titles")] = [title_by_id.get(x, x) for x in n.get(field, [])]

    # Build edges.
    edges: List[Dict] = []
    seen_edges = set()
    for node in nodes:
        for e in edges_from_node(node):
            sig = (e["from"], e["to"], e["kind"])
            if sig not in seen_edges:
                seen_edges.add(sig)
                edges.append(e)
    edges.sort(key=lambda e: (e["kind"], e["from"], e["to"]))

    # Build derived cards.
    cards = []
    for node in nodes:
        cards.extend(derive_cards_from_node(node))
    decks = defaultdict(list)
    for c in cards:
        decks[c["deck"]].append(c)

    knowledge = {
        "metadata": {
            "generated_at": _dt.datetime.now().isoformat(timespec="seconds"),
            "roots": [str(r) for r in roots],
            "node_count": len(nodes),
            "edge_count": len(edges),
            "edge_kinds": sorted(EDGE_KINDS),
            "card_count": len(cards),
            "schema_version": "1.0-predicate-b64",
            "latex_encoding": "base64-utf8",
            "script": Path(__file__).name,
        },
        "nodes": nodes,
        "edges": edges,
        "notation_profile": NOTATION_PROFILE,
        "predicate_library": PREDICATES,
    }
    args.output.write_text(json.dumps(knowledge, indent=2, ensure_ascii=False), encoding="utf-8")
    args.edges_output.write_text(json.dumps({"metadata": {"schema_version": "1.0-predicate-b64", "latex_encoding": "base64-utf8"}, "edges": edges}, indent=2, ensure_ascii=False), encoding="utf-8")
    args.cards_output.write_text(json.dumps({"metadata": {"schema_version": "1.0-predicate-b64", "latex_encoding": "base64-utf8"}, "decks": dict(decks), "total": len(cards)}, indent=2, ensure_ascii=False), encoding="utf-8")
    generate_audit_report(all_audits, args.audit)

    print("\nDone.")
    print(f"  nodes     -> {args.output}")
    print(f"  edges     -> {args.edges_output}")
    print(f"  cards     -> {args.cards_output}")
    print(f"  audit     -> {args.audit}")

    kind_counts = Counter(n["kind"] for n in nodes)
    print("\nNode kinds:")
    for k, n in sorted(kind_counts.items()):
        print(f"  {k:<12} {n}")

    edge_counts = Counter(e["kind"] for e in edges)
    print("\nEdge kinds:")
    for k, n in sorted(edge_counts.items()):
        print(f"  {k:<12} {n}")


if __name__ == "__main__":
    main()
