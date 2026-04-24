#!/usr/bin/env python3
r"""
extract-cards.py -- Extract flashcard data from Learning Real Analysis LaTeX files.

Reads \Flash* macros from notes and proof files, generates cards.json and
flash-audit.md per DESIGN.md §10-11.

Usage:
    python scripts/extract-cards.py .
    python scripts/extract-cards.py . --output cards.json
    python scripts/extract-cards.py . --include volume-iii/analysis volume-iii/algebra
    python scripts/extract-cards.py . --verbose
"""

import re, json, argparse, sys, datetime, yaml
from pathlib import Path
from collections import defaultdict

# ── Configuration ─────────────────────────────────────────────────────────────

ENV_TYPES = {
    'definition':  'Definition',
    'theorem':     'Theorem',
    'lemma':       'Lemma',
    'proposition': 'Proposition',
    'corollary':   'Corollary',
    'axiom':       'Axiom',
}

LABEL_PREFIXES = {
    'definition': 'def:', 'theorem': 'thm:', 'lemma': 'lem:',
    'proposition': 'prop:', 'corollary': 'cor:', 'axiom': 'ax:',
}

DEFAULT_INCLUDE = ['volume-iii/algebra', 'volume-iii/analysis']
DEFAULT_EXCLUDE = ['volume-iv', 'computational-linear-algebra']

SKIP_DIRS  = {'.git', 'node_modules', '_minted', '_archive'}
SKIP_FILES = {'main.tex', 'preamble.tex', 'macros.tex',
              'environments.tex', 'boxes.tex', 'colors.tex'}

_CHAPTER_EXTRACTION_CACHE = {}

# Flash fields: (macro_name, required_for_theorem_like, required_for_definition)
FLASH_FIELDS = [
    ('FlashQ',       True,  True),
    ('FlashA',       True,  True),
    ('FlashHint',    False, False),
    ('FlashNeg',     False, False),
    ('FlashImplies', True,  False),   # required for thm-like, warning only
    ('FlashIgnore',  False, False),
]

PROOF_FLASH_FIELDS = [
    ('FlashProof',   True),
    ('FlashUses',    True),
    ('FlashImplies', False),
]

# ── Helpers ───────────────────────────────────────────────────────────────────

def chapter_extraction_enabled(tex_file, root):
    """Honor chapter.yaml extraction.enabled flags for notes under a chapter."""
    current = tex_file.parent.resolve()
    root = root.resolve()

    while True:
        yaml_path = current / 'chapter.yaml'
        if yaml_path.exists():
            if yaml_path in _CHAPTER_EXTRACTION_CACHE:
                return _CHAPTER_EXTRACTION_CACHE[yaml_path]
            try:
                data = yaml.safe_load(yaml_path.read_text(encoding='utf-8')) or {}
            except Exception:
                enabled = True
            else:
                extraction = data.get('extraction', {})
                if isinstance(extraction, dict) and 'enabled' in extraction:
                    enabled = bool(extraction.get('enabled'))
                else:
                    enabled = True
            _CHAPTER_EXTRACTION_CACHE[yaml_path] = enabled
            return enabled

        if current == root or current.parent == current:
            return True
        current = current.parent.resolve()

def clean_name(raw):
    if not raw: return ''
    m = re.search(r'\\texorpdfstring\s*\{[^}]*\}\s*\{([^}]+)\}', raw)
    if m: return m.group(1).strip()
    # hyperref display text may contain \mathbb{Q} etc — extract by brace counting
    m = re.search(r'\\hyperref\[[^\]]*\]\{', raw)
    if m:
        start = m.end()
        depth, i = 1, start
        while i < len(raw) and depth:
            if raw[i] == '{': depth += 1
            elif raw[i] == '}': depth -= 1
            i += 1
        return raw[start:i-1].strip()
    return raw.strip()

def clean_body(raw):
    text = re.sub(r'\\label\{[^}]+\}', '', raw)
    text = text.replace("\\'", "'").replace('\\`', '`')
    # Resolve \hyperref[label]{display text} → just the display text
    text = re.sub(r'\\hyperref\[[^\]]*\]\{([^}]+)\}', r'\1', text)
    # Resolve \cref, \ref → readable slug
    text = re.sub(r'\\(?:c?ref|pageref)\{([^}]+)\}', lambda m: m.group(1).split(':')[-1].replace('-',' '), text)
    # Strip proof-specific macros whose arguments should just show as-is
    text = re.sub(r'\\(?:Claim|Strategy|Let|Hence|AsReq|ByThm|prooftag)\b\s*', '', text)
    text = text.replace('---', '\u2014').replace('--', '\u2013')
    text = text.replace('\\ldots', '…').replace('\\dots', '…')
    text = text.replace('\\lq\\lq', '\u201c').replace("''", '\u201d')
    # Convert enumerate/itemize to HTML
    for _ in range(4):
        def repl_enum(m):
            items = re.split(r'\\item\b', m.group(1))
            lis = ''.join(f'<li>{i.strip()}</li>' for i in items if i.strip())
            return f'<ol style="margin:.6em 0 .6em 1.5em">{lis}</ol>'
        def repl_item(m):
            items = re.split(r'\\item\b', m.group(1))
            lis = ''.join(f'<li>{i.strip()}</li>' for i in items if i.strip())
            return f'<ul style="margin:.6em 0 .6em 1.5em">{lis}</ul>'
        new = re.sub(r'\\begin\{enumerate\}(?:\[[^\]]*\])?(.*?)\\end\{enumerate\}', repl_enum, text, flags=re.DOTALL)
        new = re.sub(r'\\begin\{itemize\}(.*?)\\end\{itemize\}', repl_item, new, flags=re.DOTALL)
        if new == text: break
        text = new
    text = re.sub(r'\\emph\{([^}]+)\}', r'<em>\1</em>', text)
    text = re.sub(r'\\textbf\{([^}]+)\}', r'<strong>\1</strong>', text)
    text = re.sub(r'\\textit\{([^}]+)\}', r'<em>\1</em>', text)
    text = re.sub(r'\\(no)?indent\b', '', text)
    text = re.sub(r'\\(med|small|big)skip\b', '<br>', text)
    text = re.sub(r'\n{3,}', '\n\n', text)
    return text.strip()

def clean_text(s):
    """Light cleanup for Flash macro strings (not full LaTeX bodies)."""
    if not s: return s
    s = s.replace("\\'", "'").replace('\\`', '`')
    s = s.replace('---', '\u2014').replace('--', '\u2013')
    s = s.replace('\\ldots', '…').replace('\\dots', '…')
    return s

def extract_flash_macro(text, macro):
    """Extract content of \\MacroName{...} handling nested braces."""
    pattern = re.compile(r'\\' + macro + r'\{')
    m = pattern.search(text)
    if not m:
        return None if macro != 'FlashIgnore' else (r'\FlashIgnore' in text)
    start = m.end()
    depth, i = 1, start
    while i < len(text) and depth:
        if text[i] == '{': depth += 1
        elif text[i] == '}': depth -= 1
        i += 1
    return text[start:i-1].strip()

def extract_flash_block(tex, end_pos):
    r"""Find \begin{remark*}[Flash]...\end{remark*} after end_pos."""
    tail = tex[end_pos:]
    pattern = re.compile(
        r'(?:(?!\\begin\{(?:theorem|lemma|proposition|corollary|definition|axiom)\b).){0,3000}'
        r'\\begin\{remark\*?\}\s*\[Flash\](.*?)\\end\{remark\*?\}',
        re.DOTALL
    )
    m = pattern.match(tail)
    return m.group(1) if m else None

def extract_proof_flash(tex):
    """Extract FlashProof and FlashUses from a proof file."""
    result = {}
    for macro in ['FlashProof', 'FlashUses', 'FlashImplies']:
        val = extract_flash_macro(tex, macro)
        if val:
            result[macro] = clean_body(val)
    return result

def find_proof_file(label_root, root, rel_path):
    """Find the corresponding proof file for a notes label."""
    # Proof file is in same chapter's proofs/notes/ directory
    # e.g. volume-iii/analysis/metric-spaces/notes/... -> proofs/notes/
    parts = Path(rel_path).parts
    try:
        notes_idx = next(i for i, p in enumerate(parts) if p == 'notes')
        chapter_parts = parts[:notes_idx]
        chapter_dir = root / Path(*chapter_parts)
        proof_dir = chapter_dir / 'proofs' / 'notes'
        # Try various filename patterns
        for fname in proof_dir.glob('*.tex'):
            if fname.name == 'index.tex': continue
            content = fname.read_text(errors='replace')
            if f'\\label{{prf:{label_root}}}' in content:
                return fname, content
    except (StopIteration, OSError):
        pass
    return None, None

def infer_deck(rel_path, root):
    """Build deck name from path: volume-iii/analysis/metric-spaces -> Metric Spaces"""
    parts = list(Path(rel_path).parts)
    # Remove root parts, file, and infrastructure folders
    skip = {'notes', 'proofs', 'exercises'}
    rel = [p for p in parts[:-1] if p not in skip and not p.endswith('.tex')]

    def fmt(s):
        s = s.replace('-', ' ').replace('_', ' ').title()
        s = re.sub(r'^Volume ([IVXivx]+)$', lambda m: 'Vol ' + m.group(1).upper(), s)
        return s

    formatted = [fmt(p) for p in rel]
    # Return just the chapter level (3rd component after volume/analysis)
    # e.g. Vol III > Analysis > Metric Spaces -> "Metric Spaces"
    if len(formatted) >= 3:
        return formatted[2]  # chapter name
    return ' · '.join(formatted)

# ── Main extractor ────────────────────────────────────────────────────────────

def process_file(tex_file, root, env_filter, verbose):
    """Process one notes .tex file, return list of card dicts and audit rows."""
    # Skip proof files
    rel = tex_file.relative_to(root)
    if 'proofs' in rel.parts:
        return [], []

    try:
        text = tex_file.read_text(encoding='utf-8', errors='replace')
    except Exception:
        return [], []

    cards, audit_rows = [], []
    deck = infer_deck(str(rel), root)

    for env_name in env_filter:
        pattern = re.compile(r'\\begin\{' + re.escape(env_name) + r'\}', re.IGNORECASE)
        end_pat = re.compile(r'\\end\{' + re.escape(env_name) + r'\}', re.IGNORECASE)

        for m_begin in pattern.finditer(text):
            after = m_begin.end()
            # Extract optional arg [Name]
            opt_raw, after_opt = '', after
            j = after
            while j < len(text) and text[j] in ' \t\n': j += 1
            if j < len(text) and text[j] == '[':
                depth, k = 0, j+1
                while k < len(text):
                    if text[k] == '[': depth += 1
                    elif text[k] == ']':
                        if depth == 0: opt_raw = text[j+1:k]; after_opt = k+1; break
                        depth -= 1
                    k += 1

            name = clean_name(opt_raw)
            m_end = end_pat.search(text, after_opt)
            if not m_end: continue

            body_raw = text[after_opt:m_end.start()]
            label_m = re.search(r'\\label\{([^}]+)\}', body_raw)
            if not label_m: continue
            label = label_m.group(1)
            label_root = re.sub(r'^[a-z]+:', '', label)  # strip prefix

            body = clean_body(body_raw)
            env_type = ENV_TYPES[env_name]
            is_thm_like = env_name in ('theorem', 'lemma', 'proposition', 'corollary')

            # Extract [Fully quantified form] remark — becomes the recognition card front.
            # Rules:
            #   - Must appear before the next theorem/lemma/proposition/corollary/definition/axiom
            #   - Must appear before any \begin{remark*}[Flash] block (Flash block always
            #     comes directly after the theorem body; FQF comes after Flash)
            #   - Gap capped at 800 chars to avoid reaching across unrelated remarks
            fqf = ''
            fqf_pat = re.compile(
                r'(?:(?!\\begin\{(?:theorem|lemma|proposition|corollary|definition|axiom)\b)'
                r'(?!\\begin\{remark\*?\}\s*\[Flash\])'
                r'.){0,800}'
                r'\\begin\{remark\*?\}\s*\[Fully quantified form\](.*?)\\end\{remark\*?\}',
                re.DOTALL
            )
            fqf_m = fqf_pat.match(text[m_end.end():])
            if fqf_m:
                fqf = clean_body(fqf_m.group(1))

            # Extract Flash block
            flash_text = extract_flash_block(text, m_end.end())
            flash = {}
            ignored = False
            if flash_text:
                if 'FlashIgnore' in flash_text:
                    ignored = True
                else:
                    for field_name, _, _ in FLASH_FIELDS:
                        if field_name == 'FlashIgnore': continue
                        val = extract_flash_macro(flash_text, field_name)
                        if val:
                            flash[field_name] = clean_text(val)

            # Find proof file
            proof_file, proof_text = find_proof_file(label_root, root, str(rel))
            proof_flash = extract_proof_flash(proof_text) if proof_text else {}

            # ── Audit row ──────────────────────────────────────────────────
            def status(present, required):
                if present: return '✓'
                if required: return '✗'
                return '—'

            audit = {
                'label': label,
                'type': env_type,
                'name': name or label,
                'source': str(rel),
                'deck': deck,
                'ignored': ignored,
                'FlashQ':       status('FlashQ' in flash, True),
                'FlashA':       status('FlashA' in flash, True),
                'FlashHint':    status('FlashHint' in flash, False),
                'FQF':          status(bool(fqf), False),
                'FlashNeg':     status('FlashNeg' in flash, False),
                'FlashImplies': status('FlashImplies' in flash, is_thm_like),
                'proof_file':   status(proof_file is not None, is_thm_like),
                'FlashProof':   status('FlashProof' in proof_flash, is_thm_like and proof_file is not None),
                'FlashUses':    status('FlashUses' in proof_flash, is_thm_like and proof_file is not None),
            }
            audit_rows.append(audit)

            if ignored:
                continue

            # ── Generate cards ─────────────────────────────────────────────
            q = flash.get('FlashQ') or f"State the {env_type.lower()}: {name or label}."
            a = flash.get('FlashA') or body
            # FlashHint = proof/intuition hint shown on the answer card
            # fqf       = fully quantified form → used as front of recognition card
            proof_hint = flash.get('FlashHint', '')

            # Statement card: "State X" → answer + proof hint
            cards.append({
                'label': label, 'type': env_type, 'name': name,
                'card_type': 'statement',
                'question': q, 'answer': a, 'hint': proof_hint,
                'deck': deck, 'source': str(rel),
            })

            # Recognition card: quantified form on front → name on back
            if fqf:
                cards.append({
                    'label': label, 'type': env_type, 'name': name,
                    'card_type': 'recognition',
                    'question': fqf,
                    'answer': f"This is the <strong>{env_type}</strong>: <em>{name or label}</em>",
                    'hint': '',
                    'deck': deck, 'source': str(rel),
                })

            # Negation card: failure / counterexample
            if 'FlashNeg' in flash:
                cards.append({
                    'label': label, 'type': env_type, 'name': name,
                    'card_type': 'negation',
                    'question': f"Give a counterexample or failure case for: {name or label}.",
                    'answer': flash['FlashNeg'], 'hint': '',
                    'deck': deck, 'source': str(rel),
                })

            # Implies card (theorem-like only — definitions don't "imply" in the proof-chain sense)
            implies_labels = flash.get('FlashImplies') or proof_flash.get('FlashImplies')
            if implies_labels and is_thm_like:
                cards.append({
                    'label': label, 'type': env_type, 'name': name,
                    'card_type': 'implies',
                    'question': f"What does {name or label} immediately give you?",
                    'answer': implies_labels, 'hint': '',
                    'deck': deck, 'source': str(rel),
                })

            # Proof sketch card
            if 'FlashProof' in proof_flash:
                cards.append({
                    'label': label, 'type': env_type, 'name': name,
                    'card_type': 'proof_sketch',
                    'question': f"Sketch the proof of: {name or label}.",
                    'answer': proof_flash['FlashProof'], 'hint': '',
                    'deck': deck, 'source': str(rel),
                })

            # Uses card
            if 'FlashUses' in proof_flash:
                cards.append({
                    'label': label, 'type': env_type, 'name': name,
                    'card_type': 'uses',
                    'question': f"What does the proof of {name or label} use?",
                    'answer': proof_flash['FlashUses'], 'hint': '',
                    'deck': deck, 'source': str(rel),
                })

    return cards, audit_rows

# ── Flash audit report ────────────────────────────────────────────────────────

def generate_audit(audit_rows, output_path):
    date = datetime.date.today().isoformat()
    lines = [f"# Flash Audit — {date}\n"]
    lines.append("Symbols: ✓ present · ✗ missing (required) · — optional/not applicable\n")

    # Group by deck then source file
    by_deck = defaultdict(list)
    for row in audit_rows:
        by_deck[row['deck']].append(row)

    total_complete = 0
    total_all = 0

    for deck in sorted(by_deck):
        rows = by_deck[deck]
        lines.append(f"\n## {deck}\n")
        lines.append("| Label | Type | Q | A | Hint | Neg | Implies | Proof | FProof | FUses |")
        lines.append("|---|---|---|---|---|---|---|---|---|---|")

        complete = 0
        for r in rows:
            if r['ignored']:
                lines.append(f"| {r['label']} | {r['type']} | ∅ ignored | | | | | | | |")
                continue
            all_req = all(v != '✗' for v in [
                r['FlashQ'], r['FlashA'], r['proof_file'],
                r['FlashProof'], r['FlashUses']
            ])
            if all_req: complete += 1
            lines.append(
                f"| {r['label']} | {r['type']} "
                f"| {r['FlashQ']} | {r['FlashA']} | {r['FlashHint']} "
                f"| {r['FlashNeg']} | {r['FlashImplies']} "
                f"| {r['proof_file']} | {r['FlashProof']} | {r['FlashUses']} |"
            )

        total_complete += complete
        total_all += len([r for r in rows if not r['ignored']])
        lines.append(f"\n_{complete}/{len([r for r in rows if not r['ignored']])} complete_\n")

    lines.append(f"\n---\n## Total: {total_complete} / {total_all} environments fully annotated\n")

    Path(output_path).write_text('\n'.join(lines), encoding='utf-8')
    return total_complete, total_all

# ── CLI ───────────────────────────────────────────────────────────────────────

def main():
    # Default output paths relative to this script's directory (scripts/)
    HERE = Path(__file__).parent
    p = argparse.ArgumentParser(description='Extract flashcards and generate flash audit.')
    p.add_argument('root', type=Path)
    p.add_argument('--output', '-o', type=Path, default=HERE / 'cards.json')
    p.add_argument('--audit',  '-a', type=Path, default=HERE / 'flash-audit.md')
    p.add_argument('--include', '-i', nargs='+', default=None)
    p.add_argument('--exclude', '-x', nargs='+', default=None)
    p.add_argument('--env', '-e', nargs='+', default=list(ENV_TYPES.keys()))
    p.add_argument('--verbose', '-v', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    includes = [s.lower() for s in (args.include or DEFAULT_INCLUDE)]
    excludes = [s.lower() for s in (args.exclude or DEFAULT_EXCLUDE)]
    env_filter = set(args.env)

    print(f"Scanning {root}")
    print(f"Include: {includes}")
    print(f"Exclude: {excludes}")

    def path_ok(rel):
        s = Path(rel).as_posix().lower()
        return any(inc in s for inc in includes) and not any(exc in s for exc in excludes)

    all_cards, all_audit = [], []
    file_count = 0

    for tex_file in sorted(root.rglob('*.tex')):
        rel = tex_file.relative_to(root)
        if any(p in SKIP_DIRS for p in rel.parts): continue
        if tex_file.name in SKIP_FILES: continue
        if not path_ok(rel): continue
        if not chapter_extraction_enabled(tex_file, root):
            if args.verbose:
                print(f"  skip {rel} (chapter extraction disabled)")
            continue
        file_count += 1
        cards, audit = process_file(tex_file, root, env_filter, args.verbose)
        if cards and args.verbose:
            print(f"  {rel}: {len(cards)} cards")
        all_cards.extend(cards)
        all_audit.extend(audit)

    # Build label->name index, then resolve FlashImplies answers to human-readable names.
    # Use clean_name on the stored name, and fall back to the label slug with clean_body
    # so that math in theorem names (e.g. \mathbb{Q}) renders correctly.
    label_names = {}
    for c in all_cards:
        if c['label'] not in label_names:
            # Prefer a non-empty name; clean_body handles any residual LaTeX
            display = c['name'] if c['name'] else c['label']
            label_names[c['label']] = clean_body(display)

    for c in all_cards:
        if c['card_type'] == 'implies' and c['answer']:
            parts = [p.strip() for p in c['answer'].split(',')]
            c['answer'] = ', '.join(label_names.get(p, clean_body(p)) for p in parts)

    # Deduplicate cards by label+card_type
    seen = set()
    unique_cards = []
    for c in all_cards:
        key = (c['label'], c['card_type'])
        if key not in seen:
            seen.add(key)
            unique_cards.append(c)

    # Organise into decks
    decks = defaultdict(list)
    for c in unique_cards:
        decks[c['deck']].append(c)

    output = {'decks': dict(decks), 'total': len(unique_cards)}
    args.output.write_text(json.dumps(output, indent=2, ensure_ascii=False), encoding='utf-8')

    complete, total = generate_audit(all_audit, args.audit)

    print(f"\nDone.")
    print(f"  {file_count} files scanned")
    print(f"  {len(unique_cards)} cards generated")
    print(f"  {total} environments audited · {complete} fully annotated · {total-complete} need work")
    print(f"  cards.json  -> {args.output}")
    print(f"  audit       -> {args.audit}")

    # Summary by card type
    from collections import Counter
    type_counts = Counter(c['card_type'] for c in unique_cards)
    print("\n  Card types:")
    for t, n in sorted(type_counts.items()):
        print(f"    {t:<15} {n}")

if __name__ == '__main__':
    main()
