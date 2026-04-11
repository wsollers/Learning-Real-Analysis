import base64, json, re, html, hashlib
from collections import defaultdict, Counter
from pathlib import Path
from datetime import datetime, timezone

BASE = Path(__file__).resolve().parent
KNOWLEDGE_PATH = BASE / 'knowledge.json'
EDGES_PATH = BASE / 'graph-edges.json'
CARDS_PATH = BASE / 'cards.json'
OUT_DIR = BASE / 'knowledge_v3'
OUT_DIR.mkdir(exist_ok=True)


def read_json(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def write_json(path: Path, obj):
    path.write_text(json.dumps(obj, indent=2, ensure_ascii=False), encoding='utf-8')

def b64utf8_encode(s: str) -> str:
    return base64.b64encode((s or '').encode('utf-8')).decode('ascii')

def dual_field(plain: str, latex: str):
    return {'plain': plain or '', 'latex_b64': b64utf8_encode(latex or '')}


def normalize_edges_payload(obj):
    if isinstance(obj, list):
        return obj
    if isinstance(obj, dict):
        if isinstance(obj.get('edges'), list):
            return obj['edges']
        # tolerate accidental mapping keyed by edge id or numeric strings
        vals = list(obj.values())
        if vals and all(isinstance(v, dict) and {'from','to','kind'}.issubset(v.keys()) for v in vals):
            return vals
    raise TypeError(f"Unsupported edge payload type: {type(obj).__name__}")


def normalize_cards_payload(obj):
    if isinstance(obj, dict) and isinstance(obj.get('decks'), dict):
        return obj
    raise TypeError(f"Unsupported cards payload type: {type(obj).__name__}")

# ---------------------------------------------------------------------------
# Stable canonical registries
# ---------------------------------------------------------------------------
PREDICATE_REGISTRY = {
    'Nonempty': {
        'arity': 1,
        'arguments': ['set'],
        'latex': r'\\operatorname{Nonempty}(A)',
        'definition_latex': r'A \\neq \\varnothing',
        'aliases': ['nonempty'],
        'tags': ['core']
    },
    'Subset': {
        'arity': 2,
        'arguments': ['set','set'],
        'latex': r'\\operatorname{Subset}(A,B)',
        'definition_latex': r'A \\subseteq B',
        'aliases': ['subset'],
        'tags': ['core','sets']
    },
    'UpperBound': {
        'arity': 2,
        'arguments': ['real','set'],
        'latex': r'\\operatorname{UpperBound}(u,A)',
        'definition_latex': r'\\forall a \\in A\\,(a \\le u)',
        'aliases': ['upper bound'],
        'tags': ['bounding','order']
    },
    'LowerBound': {
        'arity': 2,
        'arguments': ['real','set'],
        'latex': r'\\operatorname{LowerBound}(l,A)',
        'definition_latex': r'\\forall a \\in A\\,(l \\le a)',
        'aliases': ['lower bound'],
        'tags': ['bounding','order']
    },
    'BoundedAbove': {
        'arity': 1,
        'arguments': ['set'],
        'latex': r'\\operatorname{BoundedAbove}(A)',
        'definition_latex': r'\\exists M \\in \\mathbb{R}\\,\\forall a \\in A\\,(a \\le M)',
        'aliases': ['bounded above'],
        'tags': ['bounding','order']
    },
    'BoundedBelow': {
        'arity': 1,
        'arguments': ['set'],
        'latex': r'\\operatorname{BoundedBelow}(A)',
        'definition_latex': r'\\exists m \\in \\mathbb{R}\\,\\forall a \\in A\\,(m \\le a)',
        'aliases': ['bounded below'],
        'tags': ['bounding','order']
    },
    'Bounded': {
        'arity': 1,
        'arguments': ['set'],
        'latex': r'\\operatorname{Bounded}(A)',
        'definition_latex': r'\\operatorname{BoundedAbove}(A) \\land \\operatorname{BoundedBelow}(A)',
        'aliases': ['bounded'],
        'tags': ['bounding','order']
    },
    'Supremum': {
        'arity': 2,
        'arguments': ['real','set'],
        'latex': r'\\operatorname{Supremum}(s,A)',
        'definition_latex': r'\\operatorname{UpperBound}(s,A) \\land \\forall u\\,(\\operatorname{UpperBound}(u,A) \\to s \\le u)',
        'aliases': ['supremum', 'least upper bound'],
        'tags': ['bounding','order']
    },
    'Infimum': {
        'arity': 2,
        'arguments': ['real','set'],
        'latex': r'\\operatorname{Infimum}(t,A)',
        'definition_latex': r'\\operatorname{LowerBound}(t,A) \\land \\forall l\\,(\\operatorname{LowerBound}(l,A) \\to l \\le t)',
        'aliases': ['infimum', 'greatest lower bound'],
        'tags': ['bounding','order']
    },
    'Maximum': {
        'arity': 2,
        'arguments': ['real','set'],
        'latex': r'\\operatorname{Maximum}(m,A)',
        'definition_latex': r'm \\in A \\land \\forall a \\in A\\,(a \\le m)',
        'aliases': ['maximum'],
        'tags': ['bounding','order']
    },
    'Minimum': {
        'arity': 2,
        'arguments': ['real','set'],
        'latex': r'\\operatorname{Minimum}(m,A)',
        'definition_latex': r'm \\in A \\land \\forall a \\in A\\,(m \\le a)',
        'aliases': ['minimum'],
        'tags': ['bounding','order']
    },
    'Dense': {
        'arity': 2,
        'arguments': ['set','ambient_set'],
        'latex': r'\\operatorname{Dense}(D,X)',
        'definition_latex': r'\\forall x \\in X\\,\\forall \\varepsilon > 0\\,\\exists d \\in D\\,(|d-x|<\\varepsilon)',
        'aliases': ['dense', 'dense subset'],
        'tags': ['topology','analysis']
    },
    'ArchimedeanProperty': {
        'arity': 0,
        'arguments': [],
        'latex': r'\\operatorname{ArchimedeanProperty}',
        'definition_latex': r'\\forall x \\in \\mathbb{R}\\,\\exists n \\in \\mathbb{N}\\,(n>x)',
        'aliases': ['archimedean property'],
        'tags': ['analysis','order']
    },
    'LeastUpperBoundProperty': {
        'arity': 0,
        'arguments': [],
        'latex': r'\\operatorname{LeastUpperBoundProperty}',
        'definition_latex': r'\\forall A\\subseteq \\mathbb{R}\\, (\\operatorname{Nonempty}(A) \\land \\operatorname{BoundedAbove}(A) \\to \\exists s \\in \\mathbb{R}\\, \\operatorname{Supremum}(s,A))',
        'aliases': ['lub property', 'completeness axiom', 'least upper bound property'],
        'tags': ['analysis','completeness']
    }
}


NOTATION_PROFILE = {
    'id': 'em-real-analysis-v1',
    'set': {'symbol': 'S', 'latex': 'S'},
    'subset': {'symbol': 'A', 'latex': 'A'},
    'metric_space': {'symbol': 'M', 'latex': r'(M,d)'},
    'metric': {'symbol': 'd', 'latex': 'd'},
    'sequence': {'symbol': 'x_n', 'latex': r'(x_n)'},
    'subsequence': {'symbol': 'x_{n_k}', 'latex': r'(x_{n_k})'},
    'limit': {'symbol': 'L', 'latex': 'L'},
    'supremum': {'symbol': 's', 'latex': 's'},
    'infimum': {'symbol': 't', 'latex': 't'},
}
PREDICATE_REGISTRY.update({
    'ConvergentSequence': {
        'arity': 2, 'arguments': ['sequence','real'],
        'latex': r'\\operatorname{ConvergentSequence}(x_n,L)',
        'definition_latex': r'\\forall \\varepsilon > 0\\,\\exists N \\in \\mathbb{N}\\,\\forall n \\ge N\\,(|x_n-L|<\\varepsilon)',
        'aliases': ['convergent sequence','sequence converges'],
        'tags': ['sequences','analysis']
    },
    'CauchySequence': {
        'arity': 1, 'arguments': ['sequence'],
        'latex': r'\\operatorname{CauchySequence}(x_n)',
        'definition_latex': r'\\forall \\varepsilon > 0\\,\\exists N \\in \\mathbb{N}\\,\\forall m,n \\ge N\\,(|x_n-x_m|<\\varepsilon)',
        'aliases': ['cauchy sequence'],
        'tags': ['sequences','analysis']
    },
    'BoundedSequence': {
        'arity': 1, 'arguments': ['sequence'],
        'latex': r'\\operatorname{BoundedSequence}(x_n)',
        'definition_latex': r'\\exists M>0\\,\\forall n\\in\\mathbb{N}\\,(|x_n|\\le M)',
        'aliases': ['bounded sequence'],
        'tags': ['sequences','analysis']
    },
    'Subsequence': {
        'arity': 2, 'arguments': ['sequence','parent_sequence'],
        'latex': r'\\operatorname{Subsequence}(x_{n_k},x_n)',
        'definition_latex': r'\\exists (n_k)\\subseteq\\mathbb{N}\\,(n_k<n_{k+1})',
        'aliases': ['subsequence'],
        'tags': ['sequences','analysis']
    },
    'MonotoneIncreasingSequence': {
        'arity': 1, 'arguments': ['sequence'],
        'latex': r'\\operatorname{MonotoneIncreasingSequence}(x_n)',
        'definition_latex': r'\\forall n\\in\\mathbb{N}\\,(x_n \\le x_{n+1})',
        'aliases': ['increasing sequence','nondecreasing sequence'],
        'tags': ['sequences','analysis']
    },
    'MonotoneDecreasingSequence': {
        'arity': 1, 'arguments': ['sequence'],
        'latex': r'\\operatorname{MonotoneDecreasingSequence}(x_n)',
        'definition_latex': r'\\forall n\\in\\mathbb{N}\\,(x_{n+1} \\le x_n)',
        'aliases': ['decreasing sequence','nonincreasing sequence'],
        'tags': ['sequences','analysis']
    },
    'EventuallyProperty': {
        'arity': 2, 'arguments': ['sequence','predicate'],
        'latex': r'\\operatorname{EventuallyProperty}(x_n,P)',
        'definition_latex': r'\\exists N\\in\\mathbb{N}\\,\\forall n\\ge N\\,P(n)',
        'aliases': ['eventually'],
        'tags': ['sequences','analysis']
    },
})

SCHEMA_REGISTRY = {
    'SupremumCharacterization': {
        'kind': 'proposition_schema',
        'statement_latex': r'\\operatorname{Supremum}(s,A) \\iff \\operatorname{UpperBound}(s,A) \\land \\forall \\varepsilon>0\\,\\exists a \\in A\\,(a>s-\\varepsilon)',
        'predicate_ids': ['Supremum','UpperBound'],
        'chapter_hint': 'analysis/bounding'
    },
    'InfimumCharacterization': {
        'kind': 'proposition_schema',
        'statement_latex': r'\\operatorname{Infimum}(t,A) \\iff \\operatorname{LowerBound}(t,A) \\land \\forall \\varepsilon>0\\,\\exists a \\in A\\,(a<t+\\varepsilon)',
        'predicate_ids': ['Infimum','LowerBound'],
        'chapter_hint': 'analysis/bounding'
    },
    'BoundedDecomposition': {
        'kind': 'proposition_schema',
        'statement_latex': r'\\operatorname{Bounded}(A) \\iff \\operatorname{BoundedAbove}(A) \\land \\operatorname{BoundedBelow}(A)',
        'predicate_ids': ['Bounded','BoundedAbove','BoundedBelow'],
        'chapter_hint': 'analysis/bounding'
    },
    'ArchimedeanSchema': {
        'kind': 'proposition_schema',
        'statement_latex': r'\\forall x \\in \\mathbb{R}\\,\\exists n \\in \\mathbb{N}\\,(n>x)',
        'predicate_ids': ['ArchimedeanProperty'],
        'chapter_hint': 'analysis/bounding'
    },
    'CongruenceEquivalence': {
        'kind': 'proposition_schema',
        'statement_latex': r'\\equiv_n\\text{ is an equivalence relation on }\\mathbb{Z}',
        'predicate_ids': [],
        'chapter_hint': 'algebra/abstract-algebra'
    },
    'BezoutSchema': {
        'kind': 'proposition_schema',
        'statement_latex': r'\\forall a,b\\in\\mathbb{Z}\\,\\exists x,y\\in\\mathbb{Z}\\,(ax+by=\\gcd(a,b))',
        'predicate_ids': [],
        'chapter_hint': 'algebra/abstract-algebra'
    }
}

PROPOSITION_OVERRIDES = {
    'def:upper-bound': r'\\operatorname{UpperBound}(u,A)',
    'def:lower-bound': r'\\operatorname{LowerBound}(l,A)',
    'def:bounded-above-below': r'\\operatorname{Bounded}(A) \\leftrightarrow \\operatorname{BoundedAbove}(A) \\land \\operatorname{BoundedBelow}(A)',
    'def:supremum': r'\\operatorname{Supremum}(s,A)',
    'def:infimum': r'\\operatorname{Infimum}(t,A)',
    'def:maximum': r'\\operatorname{Maximum}(m,A)',
    'def:minimum': r'\\operatorname{Minimum}(m,A)',
    'def:dense-subset': r'\\operatorname{Dense}(D,X)',
    'ax:completeness': r'\\operatorname{LeastUpperBoundProperty}',
    'def:archimedean-property': r'\\operatorname{ArchimedeanProperty}',
    'prop:eps-char': r'\\operatorname{Supremum}(s,A) \\leftrightarrow \\operatorname{UpperBound}(s,A) \\land \\forall \\varepsilon>0\\,\\exists a\\in A\\,(a>s-\\varepsilon)',
    'cor:eps-approx': r'\\forall \\varepsilon>0\\,\\exists a\\in A\\,(a>s-\\varepsilon)',
    'thm:monotonicity': r'\\operatorname{Subset}(A,B) \\to (\\sup A \\le \\sup B) \\land (\\inf B \\le \\inf A)',
    'thm:archimedean-property': r'\\operatorname{ArchimedeanProperty}',
    'cor:density-of-irrationals': r'\\operatorname{Dense}(\\mathbb{R}\\setminus\\mathbb{Q},\\mathbb{R})',
    'cor:irrationals-are-dense': r'\\operatorname{Dense}(\\mathbb{R}\\setminus\\mathbb{Q},\\mathbb{R})',
}

SCHEMA_OVERRIDES = {
    'prop:eps-char': 'SupremumCharacterization',
    'def:bounded-above-below': 'BoundedDecomposition',
    'def:archimedean-property': 'ArchimedeanSchema',
    'thm:archimedean-property': 'ArchimedeanSchema',
    'ax:completeness': 'LeastUpperBoundProperty',
    'thm:congruence-equivalence': 'CongruenceEquivalence',
    'thm:bezout-identity': 'BezoutSchema',
}

MANUAL_EQUIVALENT = {
    ('cor:density-of-irrationals', 'cor:irrationals-are-dense'),
    ('lem:landau-add-one-comm', 'lem:landau-succ-comm'),
    ('prop:add-comm', 'thm:landau-plus-comm'),
    ('prop:add-assoc', 'thm:landau-plus-assoc'),
    ('cor:bezout-lemma', 'thm:relatively-prime-equiv'),
}

MANUAL_SPECIALIZES = {
    ('lem:landau-succ-comm', 'prop:add-comm'),
    ('lem:landau-add-one-comm', 'prop:add-comm'),
    ('lem:landau-one-plus-x', 'thm:landau-addition-exists-unique'),
}

CANONICAL_EDGE_KINDS = ['defines','depends_on','uses','implies','equivalent_to','specializes']

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def strip_html(s: str) -> str:
    return re.sub(r'<[^>]+>', ' ', html.unescape(s or ''))

def clean_tex(s: str) -> str:
    s = s or ''
    s = re.sub(r'\\label\{[^}]+\}', '', s)
    s = re.sub(r'\\hyperref\[[^\]]+\]\{([^}]*)\}', r'\1', s)
    s = re.sub(r'\\texorpdfstring\{([^}]*)\}\{([^}]*)\}', r'\1', s)
    return s.strip()

def tex_to_plain(s: str) -> str:
    s = strip_html(clean_tex(s))
    # preserve common math words for indexing; not a renderer
    replacements = {
        r'\\mathbb\{R\}': 'ℝ',
        r'\\mathbb\{Q\}': 'ℚ',
        r'\\mathbb\{N\}': 'ℕ',
        r'\\subseteq': '⊆',
        r'\\subset': '⊂',
        r'\\leq': '≤',
        r'\\geq': '≥',
        r'\\neq': '≠',
        r'\\iff': 'iff',
        r'\\Rightarrow': '⇒',
        r'\\to': '→',
        r'\\land': '∧',
        r'\\lor': '∨',
        r'\\varepsilon': 'ε',
        r'\\infty': '∞',
    }
    for k,v in replacements.items():
        s = re.sub(k, v, s)
    s = re.sub(r'\\[a-zA-Z]+\*?(\[[^\]]*\])?(\{([^}]*)\})?', lambda m: (m.group(3) or ' '), s)
    s = s.replace('$', '')
    s = re.sub(r'\s+', ' ', s).strip()
    return s

def norm_text(s: str) -> str:
    s = tex_to_plain(s).lower()
    s = re.sub(r'\s+', ' ', s)
    return s.strip(' .;:,')

def chapter_from_source(src: str) -> str:
    src = (src or '').replace('\\','/')
    parts = src.split('/')
    if 'analysis' in parts:
        i = parts.index('analysis')
        return '/'.join(parts[i:i+2]) if i+1 < len(parts) else 'analysis'
    if 'algebra' in parts:
        i = parts.index('algebra')
        return '/'.join(parts[i:i+2]) if i+1 < len(parts) else 'algebra'
    if 'natural-numbers' in parts:
        return 'natural-numbers/addition'
    return parts[0] if parts else ''

def stable_edge_id(frm: str, to: str, kind: str) -> str:
    digest = hashlib.md5(f'{frm}|{kind}|{to}'.encode()).hexdigest()[:12]
    return f'edge:{digest}'

def infer_predicates(text: str):
    text = text or ''
    t = text
    out = []

    def add(pid: str):
        if pid not in out:
            out.append(pid)

    for pid in PREDICATE_REGISTRY:
        if re.search(rf'\\operatorname\{{{re.escape(pid)}\}}', t):
            add(pid)

    if re.search(r'\\forall\s+\\varepsilon\s*>\s*0.*\\exists\s+N\s*\\in\s*\\mathbb\{N\}.*\\forall\s+n\s*\\ge\s*N.*\|x_n\s*-\s*L\|\s*<\s*\\varepsilon', t):
        add('ConvergentSequence')
    if re.search(r'\\forall\s+\\varepsilon\s*>\s*0.*\\exists\s+N\s*\\in\s*\\mathbb\{N\}.*\\forall\s+m\s*,\s*n\s*\\ge\s*N.*\|x_n\s*-\s*x_m\|\s*<\s*\\varepsilon', t):
        add('CauchySequence')
    if re.search(r'\\exists\s+M(?:\s*>\s*0)?.*\\forall\s+n\s*(?:\\in\s*\\mathbb\{N\})?.*\|x_n\|\s*\\le\s*M', t):
        add('BoundedSequence')
    if re.search(r'n_k\s*<\s*n_\{k\+1\}', t) or re.search(r'subsequence', t, re.I):
        add('Subsequence')
    if re.search(r'\\forall\s+n.*x_n\s*\\le\s*x_\{n\+1\}', t):
        add('MonotoneIncreasingSequence')
    if re.search(r'\\forall\s+n.*x_\{n\+1\}\s*\\le\s*x_n', t):
        add('MonotoneDecreasingSequence')
    if re.search(r'\\exists\s+N.*\\forall\s+n\s*\\ge\s*N', t):
        add('EventuallyProperty')

    for pid, pats in {
        'Nonempty': [r'Nonempty', r'nonempty', r'\\neq \\varnothing'],
        'Subset': [r'Subset', r'\\subseteq'],
        'UpperBound': [r'UpperBound', r'upper bound'],
        'LowerBound': [r'LowerBound', r'lower bound'],
        'BoundedAbove': [r'BoundedAbove', r'bounded above'],
        'BoundedBelow': [r'BoundedBelow', r'bounded below'],
        'Bounded': [r'Bounded\(', r'bounded\b'],
        'Supremum': [r'Supremum', r'\\sup', r'supremum'],
        'Infimum': [r'Infimum', r'\\inf', r'infimum'],
        'Maximum': [r'Maximum', r'maximum'],
        'Minimum': [r'Minimum', r'minimum'],
        'Dense': [r'Dense', r'dense'],
        'ArchimedeanProperty': [r'ArchimedeanProperty', r'archimedean property'],
        'LeastUpperBoundProperty': [r'LeastUpperBoundProperty', r'least upper bound property', r'completeness axiom'],
    }.items():
        for p in pats:
            if re.search(p, t, re.I):
                add(pid)
                break

    return sorted(out)

# ---------------------------------------------------------------------------
# Load source artifacts
# ---------------------------------------------------------------------------
knowledge = read_json(KNOWLEDGE_PATH)
raw_edges = normalize_edges_payload(read_json(EDGES_PATH))
cards = normalize_cards_payload(read_json(CARDS_PATH))
node_map = {n['id']: n for n in knowledge['nodes']}

# ---------------------------------------------------------------------------
# Normalize nodes
# ---------------------------------------------------------------------------
normalized_nodes = []
for n in knowledge['nodes']:
    statement_latex = clean_tex(n.get('statement_tex') or n.get('statement_display') or '')
    formal_latex = clean_tex(n.get('proposition') or PROPOSITION_OVERRIDES.get(n['id']) or n.get('formal') or n.get('fully_quantified_form') or '')
    negation_latex = clean_tex(n.get('negation_formal') or n.get('negation_display') or '')
    contrapositive_latex = clean_tex(n.get('contrapositive') or '')
    name_latex = clean_tex(n.get('name') or '')
    proof_uses = clean_tex(n.get('uses_text') or '')
    text_for_predicates = ' '.join([formal_latex, statement_latex, name_latex])
    predicate_ids = infer_predicates(text_for_predicates)
    schema_id = SCHEMA_OVERRIDES.get(n['id'])
    if schema_id == 'LeastUpperBoundProperty':
        schema_id = None  # predicate, not schema
    normalized_nodes.append({
        'id': n['id'],
        'kind': n.get('kind',''),
        'name_latex': name_latex,
        'name_plain': tex_to_plain(name_latex),
        'name': dual_field(tex_to_plain(name_latex), name_latex),
        'statement_latex': statement_latex,
        'statement_plain': tex_to_plain(statement_latex),
        'statement': dual_field(tex_to_plain(statement_latex), statement_latex),
        'formal_latex': formal_latex,
        'formal_plain': tex_to_plain(formal_latex),
        'formal_structured': dual_field(tex_to_plain(formal_latex), formal_latex),
        'negation_latex': negation_latex,
        'negation_plain': tex_to_plain(negation_latex),
        'negation': dual_field(tex_to_plain(negation_latex), negation_latex),
        'contrapositive_latex': contrapositive_latex,
        'contrapositive_plain': tex_to_plain(contrapositive_latex),
        'contrapositive': dual_field(tex_to_plain(contrapositive_latex), contrapositive_latex),
        'question': n.get('question',''),
        'question_structured': dual_field(tex_to_plain(n.get('question','')), n.get('question','')),
        'hint': n.get('hint',''),
        'proof_sketch': n.get('proof_sketch',''),
        'uses_text': proof_uses,
        'source': n.get('source',''),
        'proof_source': n.get('proof_source',''),
        'chapter': chapter_from_source(n.get('source','')),
        'notation_profile': NOTATION_PROFILE['id'],
        'schema_id': schema_id,
        'predicate_ids': predicate_ids,
        'tags': sorted(set((n.get('tags') or []) + infer_predicates(name_latex))),
        'has_proof_file': bool(n.get('proof_source')),
        'is_theorem_like': bool(n.get('is_theorem_like')),
        'ignored': bool(n.get('ignored')),
        'legacy': {
            'deck': n.get('deck',''),
            'aliases': n.get('aliases',[]),
            'symbol': n.get('symbol',''),
            'category': n.get('category',''),
            'proof_idea': n.get('proof_idea',''),
            'proof_type': n.get('proof_type',''),
            'method': n.get('method','')
        }
    })

# ---------------------------------------------------------------------------
# Canonicalize edges (layout edges flow prerequisite -> dependent)
# ---------------------------------------------------------------------------
edge_seen = set()
normalized_edges = []

for e in raw_edges:
    if not isinstance(e, dict) or not {'from','to','kind'}.issubset(e.keys()):
        continue
    frm, to, kind = e['from'], e['to'], e['kind']
    if frm not in node_map or to not in node_map:
        continue
    if kind in ('depends_on', 'prereq'):
        c_from, c_to, c_kind = to, frm, 'depends_on'
    elif kind == 'used_by':
        c_from, c_to, c_kind = frm, to, 'uses'
    elif kind == 'implies':
        c_from, c_to, c_kind = frm, to, 'implies'
    elif kind == 'equivalent_to':
        a,b = sorted([frm,to])
        c_from, c_to, c_kind = a, b, 'equivalent_to'
    else:
        continue
    key = (c_from,c_to,c_kind)
    if key in edge_seen:
        continue
    edge_seen.add(key)
    normalized_edges.append({
        'id': stable_edge_id(c_from, c_to, c_kind),
        'from': c_from,
        'to': c_to,
        'kind': c_kind,
        'directed': c_kind != 'equivalent_to',
        'label_latex': '',
        'label_plain': ''
    })

# derives defines edges from dependency/uses relation to definitions
for e in list(normalized_edges):
    if e['kind'] not in ('depends_on','uses'):
        continue
    src = node_map.get(e['from'])
    dst = node_map.get(e['to'])
    if not src or not dst:
        continue
    if src.get('kind') == 'Definition':
        key = (e['from'], e['to'], 'defines')
        if key not in edge_seen:
            edge_seen.add(key)
            normalized_edges.append({
                'id': stable_edge_id(e['from'], e['to'], 'defines'),
                'from': e['from'],
                'to': e['to'],
                'kind': 'defines',
                'directed': True,
                'label_latex': r'\\text{introduces vocabulary}',
                'label_plain': 'introduces vocabulary'
            })

# add manual / inferred equivalent_to edges
# conservative: equivalent_to is seeded manually for stability
for a,b in sorted(MANUAL_EQUIVALENT):
    if a in node_map and b in node_map:
        aa,bb = sorted([a,b])
        key = (aa,bb,'equivalent_to')
        if key not in edge_seen:
            edge_seen.add(key)
            normalized_edges.append({
                'id': stable_edge_id(aa, bb, 'equivalent_to'),
                'from': aa,
                'to': bb,
                'kind': 'equivalent_to',
                'directed': False,
                'label_latex': r'\\text{same mathematical content}',
                'label_plain': 'same mathematical content'
            })

for a,b in sorted(MANUAL_SPECIALIZES):
    if a in node_map and b in node_map:
        key = (a,b,'specializes')
        if key not in edge_seen:
            edge_seen.add(key)
            normalized_edges.append({
                'id': stable_edge_id(a,b,'specializes'),
                'from': a,
                'to': b,
                'kind': 'specializes',
                'directed': True,
                'label_latex': r'\\text{special case of}',
                'label_plain': 'special case of'
            })

# stable sort
normalized_edges.sort(key=lambda e: (e['kind'], e['from'], e['to']))
normalized_nodes.sort(key=lambda n: n['id'])

# ---------------------------------------------------------------------------
# Normalize cards from normalized node data and normalized edges
# ---------------------------------------------------------------------------
node_norm_map = {n['id']: n for n in normalized_nodes}
edge_out = defaultdict(list)
for e in normalized_edges:
    edge_out[(e['from'], e['kind'])].append(e['to'])

normalized_cards = {'decks': {}}
for deck, deck_cards in cards.get('decks', {}).items():
    out = []
    seen = set()
    for c in deck_cards:
        label = c.get('label')
        n = node_norm_map.get(label)
        card_type = c.get('card_type')
        q = c.get('question','')
        a = c.get('answer','')
        if n:
            if card_type == 'recognition' and n['formal_latex']:
                q = n['formal_latex']
                a = f"This is the <strong>{n['kind']}</strong>: <em>{n['name_plain']}</em>"
            elif card_type == 'statement' and n['statement_latex']:
                a = n['statement_latex']
            elif card_type == 'implies':
                a = ', '.join(edge_out.get((label,'implies'), []))
            elif card_type == 'uses':
                a = ', '.join(edge_out.get((label,'uses'), [])) or c.get('answer','')
            elif card_type == 'equivalent_to':
                a = ', '.join(edge_out.get((label,'equivalent_to'), []))
            elif card_type == 'specializes':
                a = ', '.join(edge_out.get((label,'specializes'), []))
        key=(label,card_type,q,a)
        if key in seen:
            continue
        seen.add(key)
        out.append({
            'label': label,
            'type': c.get('type'),
            'name_latex': n['name_latex'] if n else c.get('name',''),
            'name_plain': n['name_plain'] if n else tex_to_plain(c.get('name','')),
            'card_type': card_type,
            'question_latex': q,
            'question_plain': tex_to_plain(q),
            'answer_latex': a,
            'answer_plain': tex_to_plain(a),
            'hint': c.get('hint',''),
            'deck': deck,
            'source': c.get('source','')
        })
    normalized_cards['decks'][deck] = out

# ---------------------------------------------------------------------------
# Compute rooted DAG suggestions for bounding
# ---------------------------------------------------------------------------
# layout graph excludes equivalent_to and specializes
layout_edges = [e for e in normalized_edges if e['kind'] in ('defines','depends_on','uses','implies')]
layout_in = defaultdict(int)
layout_out = defaultdict(int)
for e in layout_edges:
    layout_in[e['to']] += 1
    layout_out[e['from']] += 1
roots = sorted([n['id'] for n in normalized_nodes if layout_in[n['id']] == 0])

bounding_root = 'ax:completeness'

# ---------------------------------------------------------------------------
# Persist artifacts
# ---------------------------------------------------------------------------
metadata = {
    'generated_at': datetime.now(timezone.utc).isoformat(timespec='seconds').replace('+00:00', 'Z'),
    'source_artifacts': ['knowledge.json','graph-edges.json','cards.json'],
    'schema_version': '0.3',
    'script': 'extract-knowledge-v3.py',
    'latex_encoding': 'base64-utf8',
    'canonical_edge_kinds': CANONICAL_EDGE_KINDS,
    'edge_direction': 'prerequisite_to_dependent for defines/depends_on/uses/implies; literal for specializes; undirected for equivalent_to',
    'node_count': len(normalized_nodes),
    'edge_count': len(normalized_edges),
    'predicate_count': len(PREDICATE_REGISTRY),
    'schema_count': len(SCHEMA_REGISTRY),
    'roots': roots,
    'chapter_roots': {'analysis/bounding': bounding_root}
}

write_json(OUT_DIR / 'knowledge.normalized.json', {'metadata': metadata, 'notation_profile': NOTATION_PROFILE, 'nodes': normalized_nodes})
write_json(OUT_DIR / 'graph-edges.normalized.json', {'metadata': {'schema_version': '0.3', 'edge_count': len(normalized_edges), 'edge_kinds': CANONICAL_EDGE_KINDS}, 'edges': normalized_edges})
write_json(OUT_DIR / 'predicates.json', {'metadata': {'schema_version': '0.3', 'predicate_count': len(PREDICATE_REGISTRY), 'latex_encoding': 'base64-utf8'}, 'notation_profile': NOTATION_PROFILE, 'predicates': [dict({'id': k}, **v, latex_b64=b64utf8_encode(v.get('latex','')), definition_latex_b64=b64utf8_encode(v.get('definition_latex',''))) for k,v in sorted(PREDICATE_REGISTRY.items())]})
write_json(OUT_DIR / 'schemas.json', {'metadata': {'schema_version': '0.3', 'schema_count': len(SCHEMA_REGISTRY), 'latex_encoding': 'base64-utf8'}, 'schemas': [dict({'id': k}, **v, statement_latex_b64=b64utf8_encode(v.get('statement_latex',''))) for k,v in sorted(SCHEMA_REGISTRY.items())]})
write_json(OUT_DIR / 'cards.normalized.json', normalized_cards)



summary = {
    'node_count': len(normalized_nodes),
    'edge_counts': Counter(e['kind'] for e in normalized_edges),
    'roots': roots[:20],
    'bounding_root': bounding_root,
    'equivalent_examples': [e for e in normalized_edges if e['kind']=='equivalent_to'][:10],
    'specialization_examples': [e for e in normalized_edges if e['kind']=='specializes'][:10],
}
write_json(OUT_DIR / 'summary.json', summary)

print(json.dumps(summary, indent=2, ensure_ascii=False, default=lambda o: dict(o)))
