#!/usr/bin/env python3
"""
inject_flash.py — Insert \begin{remark*}[Flash]...\end{remark*} blocks
after labeled environments across the LRA project.

Run: python3 /home/claude/inject_flash.py
"""
import re
from pathlib import Path

ROOT = Path('/home/claude/lra')

# ── helpers ──────────────────────────────────────────────────────────────────

def find_env_end(text, label):
    env_names = ['theorem','lemma','proposition','corollary','definition','axiom']
    for env in env_names:
        bp = re.compile(r'\\begin\{' + env + r'\}', re.IGNORECASE)
        ep = re.compile(r'\\end\{'   + env + r'\}', re.IGNORECASE)
        for mb in bp.finditer(text):
            me = ep.search(text, mb.end())
            if not me: continue
            if f'\\label{{{label}}}' in text[mb.start():me.end()]:
                return me.end()
    return None

def flash_block(q, a, hint='', neg='', implies=''):
    lines = [r'\begin{remark*}[Flash]',
             f'\\FlashQ{{{q}}}',
             f'\\FlashA{{{a}}}']
    if hint:    lines.append(f'\\FlashHint{{{hint}}}')
    if neg:     lines.append(f'\\FlashNeg{{{neg}}}')
    if implies: lines.append(f'\\FlashImplies{{{implies}}}')
    lines.append(r'\end{remark*}')
    return '\n'.join(lines)

def inject(filepath, patches):
    """patches: dict label -> (q,a,hint,neg,implies)"""
    path = ROOT / filepath
    if not path.exists():
        print(f'  MISSING FILE: {filepath}'); return 0
    text = path.read_text(encoding='utf-8')
    count = 0
    for label, (q,a,hint,neg,implies) in patches.items():
        pos = find_env_end(text, label)
        if pos is None:
            print(f'  WARN label not found: {label} in {filepath}'); continue
        tail = text[pos:pos+500]
        if re.search(r'\\begin\{remark\*?\}\s*\[Flash\]', tail):
            continue  # already done
        block = '\n\n' + flash_block(q, a, hint, neg, implies) + '\n'
        text = text[:pos] + block + text[pos:]
        count += 1
        print(f'  +{label}')
    if count:
        path.write_text(text, encoding='utf-8')
        print(f'  → wrote {filepath} ({count} blocks)')
    return count

# ── ABSTRACT ALGEBRA ─────────────────────────────────────────────────────────

inject('volume-iii/algebra/abstract-algebra/notes/notes-integers.tex', {
    'thm:division-algorithm': (
        'State the Division Algorithm.',
        r'For integers $a$, $b>0$: unique $q,r$ with $a=bq+r$ and $0\le r<b$.',
        r'Unique quotient and remainder; remainder lies in $[0,b)$.',
        '', 'thm:bezout-identity'),
    'thm:bezout-identity': (
        "State Bézout's Identity.",
        r'For $a,b\in\mathbb{Z}$ not both zero, $\gcd(a,b)=as+bt$ for some $s,t\in\mathbb{Z}$, and $\gcd(a,b)$ is the smallest positive such linear combination.',
        r'Three engines: Well-Ordering $\Rightarrow$ minimal $d$; Division Algorithm $\Rightarrow$ $d\mid a,b$; linear combo $\Rightarrow$ greatest-ness.',
        '', 'cor:bezout-lemma, cor:euclids-lemma-coprimality, thm:euclids-lemma-prime'),
    'lem:div-bound': (
        'State the Divisibility Bound lemma.',
        r'If $a\neq 0$ and $b\mid a$, then $|b|\le|a|$.',
        r'A divisor cannot exceed the absolute value of what it divides.',
        r'Fails when $a=0$: excluded by hypothesis.', ''),
    'lem:div-linear-combo': (
        'State the Divisibility of Linear Combinations lemma.',
        r'If $t\mid a$ and $t\mid b$ then $t\mid(ua+vb)$ for all $u,v\in\mathbb{Z}$.',
        r'If $t$ fits both $a$ and $b$ exactly it fits any scaling-and-combination of them.',
        '', 'thm:bezout-identity'),
    'lem:gcd-remainder': (
        'State the GCD Remainder Lemma.',
        r'If $a=bq+r$ then $\gcd(a,b)=\gcd(b,r)$.',
        r'Engine of the Euclidean Algorithm: replace $\gcd(a,b)$ with $\gcd(b,r)$ until $r=0$.',
        '', 'thm:bezout-identity'),
    'cor:bezout-lemma': (
        "State Bézout's Lemma (coprimality form).",
        r'$\gcd(a,b)=1$ iff $1=ma+nb$ for some $m,n\in\mathbb{Z}$.',
        r'A linear combination equalling 1 leaves no room for any common factor $>1$.',
        '', 'cor:euclids-lemma-coprimality'),
    'cor:euclids-lemma-coprimality': (
        "State Euclid's Lemma (coprimality form).",
        r'If $a\mid bc$ and $\gcd(a,b)=1$ then $a\mid c$.',
        r'Since $a,b$ share no factor, $a$\'s divisibility of $bc$ must come entirely from $c$.',
        '', 'thm:euclids-lemma-prime'),
})

# ── ALGEBRAIC STRUCTURES ─────────────────────────────────────────────────────

inject('volume-iii/algebra/algebraic-structures/notes/rings/notes-rings-theorems.tex', {
    'prop:ring-mult-zero': (
        r'State: multiplication by zero in a ring.',
        r'In any ring $R$: $a\cdot 0=0\cdot a=0$ for all $a\in R$.',
        r'Distributivity: $a\cdot 0=a(0+0)=a\cdot 0+a\cdot 0$; cancel by additive inverse.',
        '', ''),
})

inject('volume-iii/algebra/algebraic-structures/notes/fields/notes-fields-theorems.tex', {
    'prop:field-is-domain': (
        r'State: every field is an integral domain.',
        r'Every field is an integral domain: $ab=0\Rightarrow a=0$ or $b=0$.',
        r'If $a\neq 0$, multiply by $a^{-1}$ to get $b=0$.',
        r'$\mathbb{Z}/6\mathbb{Z}$ is a ring but not a domain: $2\cdot 3=0$.', ''),
    'prop:field-zero-product': (
        r'State the zero-product property in a field.',
        r'In field $\mathbb{F}$: $ab=0\Rightarrow a=0$ or $b=0$.',
        r'If $a\neq 0$ multiply by $a^{-1}$.',
        r'Fails in $\mathbb{Z}/6\mathbb{Z}$: $2\cdot 3=0$.', ''),
    'prop:field-inverse-exists': (
        r'State uniqueness of multiplicative inverses in a field.',
        r'For $a\neq 0$ in field $\mathbb{F}$, there is a unique $a^{-1}$ with $a\cdot a^{-1}=1$.',
        r'If $ax=ay=1$ then $x=x(ay)=(xa)y=y$.', '', ''),
})

inject('volume-iii/algebra/algebraic-structures/notes/groups/notes-groups-theorems.tex', {
    'prop:group-identity-unique': (
        r'State uniqueness of the group identity.',
        r'In any group $G$, the identity element is unique.',
        r'If $e,e\'$ are both identities: $e=e\cdot e\'=e\'$.',
        '', ''),
    'prop:group-inverse-unique': (
        r'State uniqueness of group inverses.',
        r'In any group $G$, each element has a unique inverse.',
        r'If $b$ and $c$ are both inverses of $a$: $b=b(ac)=(ba)c=c$.',
        '', ''),
    'prop:group-cancellation': (
        r'State the cancellation laws in a group.',
        r'In group $G$: $ab=ac\Rightarrow b=c$ (left) and $ba=ca\Rightarrow b=c$ (right).',
        r'Multiply on left by $a^{-1}$ (or right by $a^{-1}$).', '', ''),
    'prop:group-socks-shoes': (
        r'State the socks-shoes property.',
        r'In group $G$: $(ab)^{-1}=b^{-1}a^{-1}$.',
        r'To undo putting on socks then shoes, take shoes off first.', '', ''),
})

inject('volume-iii/algebra/algebraic-structures/notes/rings/notes-rings-integral-domains.tex', {
    'prop:domain-cancellation': (
        r'State cancellation in integral domains.',
        r'In integral domain $R$ with $a\neq 0$: $ab=ac\Rightarrow b=c$.',
        r'$ab=ac\Rightarrow a(b-c)=0$; since $a\neq 0$ and $R$ is a domain, $b-c=0$.', '', ''),
})

# ── BOUNDING ─────────────────────────────────────────────────────────────────

inject('volume-iii/analysis/bounding/notes/notes-bounding-monotonicty.tex', {
    'thm:monotonicity': (
        r'State Monotonicity of Supremum and Infimum.',
        r'If $A\subseteq B$ (nonempty, bounded): $\sup A\le\sup B$ and $\inf B\le\inf A$.',
        r'Every upper bound of $B$ is also an upper bound of $A$.',
        r'$A\subseteq B$ does not force strict inequality: equality is possible.',
        'cor:diam-monotone'),
    'thm:monotonicity-functions': (
        r'State Monotonicity of Supremum for functions.',
        r'If $A\subseteq B\subseteq X$: $\sup_{A}f\le\sup_{B}f$ and $\inf_{B}f\le\inf_{A}f$.',
        r'Restricting the domain can only shrink or preserve extrema.', '', ''),
    'cor:diam-monotone': (
        r'State Diameter Monotonicity.',
        r'In metric space $(X,d)$: $A\subseteq B\Rightarrow\operatorname{diam}(A)\le\operatorname{diam}(B)$.',
        r'Diameter = supremum of distances; monotonicity of sup applies.', '', ''),
})

inject('volume-iii/analysis/bounding/notes/notes-bounds-arithmetic.tex', {
    'thm:neg-rule': (
        r'State the Negation Rule for sup/inf.',
        r'For nonempty bounded $A\subseteq\mathbb{R}$: $\sup(-A)=-\inf A$ and $\inf(-A)=-\sup A$.',
        r'Negation flips the order; the new least upper bound is the negation of the old greatest lower bound.',
        '', 'thm:scale-neg'),
    'thm:sup-add': (
        r'State Additivity of Supremum.',
        r'$\sup(A+B)=\sup A+\sup B$ for nonempty bounded $A,B\subseteq\mathbb{R}$.',
        r'Show $\sup A+\sup B$ is an upper bound; use $\varepsilon$-characterisation to show tightness.',
        '', ''),
    'thm:inf-add': (
        r'State Additivity of Infimum.',
        r'$\inf(A+B)=\inf A+\inf B$ for nonempty bounded $A,B\subseteq\mathbb{R}$.',
        r'Negate: $\inf(A+B)=-\sup(-(A+B))=-(\sup(-A)+\sup(-B))=\inf A+\inf B$.', '', ''),
    'thm:scale-pos': (
        r'State scalar multiplication for sup/inf ($c>0$).',
        r'For $c>0$: $\sup(cA)=c\sup A$ and $\inf(cA)=c\inf A$.',
        r'Scaling by $c>0$ preserves order; extrema scale by $c$.', '', ''),
    'thm:scale-neg': (
        r'State scalar multiplication for sup/inf ($c<0$).',
        r'For $c<0$: $\sup(cA)=c\inf A$ and $\inf(cA)=c\sup A$.',
        r'Scaling by $c<0$ reverses order; sup and inf swap roles.', '', ''),
})

inject('volume-iii/analysis/bounding/notes/notes-bounds-extremal-values.tex', {
    'prop:sup-unique': (
        r'State uniqueness of supremum and infimum.',
        r'If $\sup A$ exists it is unique; if $\inf A$ exists it is unique.',
        r'If $s,s\'$ are both suprema: $s\le s\'$ and $s\'\le s$, so $s=s\'$.', '', ''),
    'prop:eps-char': (
        r'State the $\varepsilon$-characterisation of supremum.',
        r'$s=\sup A$ iff: (i) $s$ is an upper bound of $A$; and (ii) $\forall\varepsilon>0\,\exists a\in A: a>s-\varepsilon$.',
        r'The $\varepsilon$-test replaces abstract least-ness with a constructive approximation condition.',
        '', 'cor:eps-approx'),
})

inject('volume-iii/analysis/bounding/notes/notes-completeness.tex', {
    'thm:nested-interval-property': (
        r'State the Nested Interval Property.',
        r'If $[a_1,b_1]\supseteq[a_2,b_2]\supseteq\cdots$ are nonempty closed intervals, then $\bigcap_{n=1}^\infty[a_n,b_n]\neq\varnothing$.',
        r'$(a_n)$ is increasing and bounded above; $s=\sup\{a_n\}$ lies in every interval.',
        r'Fails for open intervals: $\bigcap_n(0,1/n)=\varnothing$.', ''),
    'thm:archimedean-property': (
        r'State the Archimedean Property.',
        r'For any $x\in\mathbb{R}$ there exists $n\in\mathbb{N}$ with $n>x$. Equivalently, $\mathbb{N}$ is unbounded in $\mathbb{R}$.',
        r'If $\mathbb{N}$ were bounded, $\sup\mathbb{N}$ would exist; but $\sup\mathbb{N}-1$ is not an upper bound, contradiction.',
        '', 'lem:floor-lemma, thm:density-of-rationals'),
    'lem:floor-lemma': (
        r'State the Floor Lemma.',
        r'For every $x\in\mathbb{R}$ there is a unique $m\in\mathbb{Z}$ with $m\le x<m+1$.',
        r'Existence from Archimedean; uniqueness since two consecutive integers cannot both satisfy the condition.',
        '', 'thm:density-of-rationals'),
    'thm:density-of-rationals': (
        r'State density of $\mathbb{Q}$ in $\mathbb{R}$.',
        r'$\mathbb{Q}$ is dense in $\mathbb{R}$: for any $a<b$ there exists $q\in\mathbb{Q}$ with $a<q<b$.',
        r'Use Archimedean to find $n$ with $1/n<b-a$; then $m=\lfloor na\rfloor+1$ gives the numerator $q=m/n$.',
        '', 'cor:density-of-irrationals'),
})

# ── METRIC SPACES ─────────────────────────────────────────────────────────────

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-metric-space.tex', {
    'def:metric': (
        r'State the four axioms defining a metric $d$ on $X$.',
        r'(M1) $d(x,y)\ge 0$; (M2) $d(x,y)=0\iff x=y$; (M3) $d(x,y)=d(y,x)$; (M4) $d(x,z)\le d(x,y)+d(y,z)$.',
        r'Non-negativity, identity of indiscernibles, symmetry, triangle inequality.',
        r'$d(x,y)=|x-y|^2$ fails M4: $(0-2)^2=4 > (0-1)^2+(1-2)^2=2$.', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-properties-metric-space.tex', {
    'prop:distance-to-itself': (
        r'Prove: $d(x,x)=0$ follows from the metric axioms.',
        r'$d(x,x)=0$ for all $x\in X$ (follows from M2: $d(x,x)=0\iff x=x$, which is always true).',
        '', '', ''),
    'prop:positivity': (
        r'Prove: $x\neq y\Rightarrow d(x,y)>0$.',
        r'If $x\neq y$ then $d(x,y)>0$: by M2 $d(x,y)=0\iff x=y$, so $x\neq y$ forces $d(x,y)\neq 0$; M1 gives $d(x,y)>0$.',
        '', '', ''),
    'prop:reverse-triangle': (
        r'State the Reverse Triangle Inequality.',
        r'$|d(x,z)-d(y,z)|\le d(x,y)$ for all $x,y,z\in X$.',
        r'Apply M4 twice: $d(x,z)\le d(x,y)+d(y,z)$ and $d(y,z)\le d(x,y)+d(x,z)$.',
        '', ''),
    'prop:generalised-triangle': (
        r'State the Generalised Triangle Inequality.',
        r'$d(x_0,x_n)\le\sum_{k=0}^{n-1}d(x_k,x_{k+1})$ for any $x_0,\ldots,x_n\in X$.',
        r'Induction on $n$ using M4.', '', ''),
    'prop:continuity-of-distance': (
        r'State continuity of the distance function.',
        r'For fixed $z\in X$, the function $f(x):=d(x,z)$ is continuous (Lipschitz with constant 1).',
        r'Reverse triangle inequality: $|d(x,z)-d(y,z)|\le d(x,y)$.', '', ''),
    'prop:triangle-three-forms': (
        r'State the three forms of the triangle inequality.',
        r'Each side of a triangle is at most the sum of the other two: $d(x,z)\le d(x,y)+d(y,z)$, $d(x,y)\le d(x,z)+d(z,y)$, $d(y,z)\le d(y,x)+d(x,z)$.',
        r'All three follow by relabelling from M4.', '', ''),
    'prop:reverse-triangle': (
        r'State the Reverse Triangle Inequality.',
        r'$|d(x,z)-d(y,z)|\le d(x,y)$.',
        r'Apply M4 to both $d(x,z)\le d(x,y)+d(y,z)$ and $d(y,z)\le d(x,y)+d(x,z)$.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/balls/notes-balls.tex', {
    'def:open-ball': (
        r'Define the open ball $B_r(x)$.',
        r'$B_r(x):=\{y\in X:d(x,y)<r\}$ for $r>0$.',
        r'Strict inequality: boundary not included.',
        '', 'def:open-closed-set'),
})

inject('volume-iii/analysis/metric-spaces/notes/balls/notes-working-with-balls.tex', {
    'prop:open-as-union-closed': (
        r'State: open ball as a union of closed balls.',
        r'$B_r(x)=\bigcup_{0<s<r}\overline{B}_s(x)$.',
        r'Any $y$ with $d(x,y)<r$ lies in $\overline{B}_s(x)$ for some $s<r$, e.g.\ $s=(d(x,y)+r)/2$.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/open-closed-sets/open-closed-sets.tex', {
    'def:open-closed-set': (
        r'Define open and closed sets in a metric space.',
        r'$U$ is open if $\forall x\in U\,\exists r>0:B_r(x)\subseteq U$. $F$ is closed if $X\setminus F$ is open.',
        r'Open = every point has breathing room inside. Closed = complement is open.',
        r'$[0,1)$ is neither open nor closed in $\mathbb{R}$.',
        'thm:closed-equiv, prop:balls-open-closed'),
    'def:interior': (
        r'Define the interior $\operatorname{int}(A)$.',
        r'$\operatorname{int}(A):=\{x\in A:\exists r>0,B_r(x)\subseteq A\}$ — largest open set inside $A$.',
        '', '', 'thm:ibe-decomp'),
    'def:exterior': (
        r'Define the exterior $\operatorname{ext}(A)$.',
        r'$\operatorname{ext}(A):=\operatorname{int}(X\setminus A)$ — points with a ball entirely outside $A$.',
        '', '', 'thm:ibe-decomp'),
    'def:boundary': (
        r'Define the boundary $\partial A$.',
        r'$\partial A:=\{x\in X:\forall r>0,B_r(x)\cap A\neq\varnothing\text{ and }B_r(x)\cap(X\setminus A)\neq\varnothing\}$.',
        r'Boundary points cannot be pushed into either $A$ or its complement.',
        '', 'thm:ibe-decomp, thm:boundary-closure'),
    'def:adherent-point': (
        r'Define an adherent point of $A$.',
        r'$x$ is adherent to $A$ if $\forall r>0,B_r(x)\cap A\neq\varnothing$.',
        r'Compare: limit point requires $B_r(x)\cap(A\setminus\{x\})\neq\varnothing$.', '', ''),
    'def:closure': (
        r'Define the closure $\overline{A}$.',
        r'$\overline{A}:=\{x\in X:\forall r>0,B_r(x)\cap A\neq\varnothing\}$ — the set of all adherent points.',
        r'Smallest closed set containing $A$; equals $A\cup A\'$.',
        '', 'thm:closure-formula, thm:boundary-closure'),
    'prop:trivial-open-closed': (
        r'State: $\varnothing$ and $X$ are both open and closed.',
        r'In any metric space $\varnothing$ and $X$ are clopen: open because the condition is vacuously true; closed because their complements ($X$ and $\varnothing$) are open.',
        '', '', ''),
    'prop:balls-open-closed': (
        r'State: open balls are open; closed balls are closed.',
        r'$B_r(x)$ is open; $\overline{B}_r(x)$ is closed.',
        r'For open ball: for any $y\in B_r(x)$ take $\delta=r-d(x,y)>0$; then $B_\delta(y)\subseteq B_r(x)$.', '', ''),
    'prop:closed-owns-boundary': (
        r'State: closed sets contain their boundary.',
        r'$F$ is closed iff $\partial F\subseteq F$.',
        r'Boundary points are adherent to $F$; if $F$ is closed (sequentially) it must contain them.', '', ''),
    'thm:ibe-decomp': (
        r'State the Interior-Boundary-Exterior Decomposition.',
        r'$X=\operatorname{int}(A)\sqcup\partial A\sqcup\operatorname{ext}(A)$ — a disjoint partition.',
        r'Every point has a ball entirely in $A$ (interior), entirely outside $A$ (exterior), or neither (boundary).', '', ''),
    'thm:boundary-closure': (
        r'State: boundary via closure.',
        r'$\partial A=\overline{A}\cap\overline{X\setminus A}$ and $\overline{A}=A\cup\partial A$.',
        '', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/convergence/notes-sequential-characterizations.tex', {
    'def:closed-sequential': (
        r'State the sequential definition of a closed set.',
        r'$F\subseteq X$ is closed if: whenever $(x_n)\subseteq F$ and $x_n\to x$ in $X$, then $x\in F$.',
        r'Closed = limits of sequences in $F$ stay in $F$.',
        '', 'thm:closed-equiv'),
    'def:open-sequential': (
        r'State the sequential definition of an open set.',
        r'$U\subseteq X$ is open if: whenever $x_n\to x\in U$, the sequence is eventually in $U$.',
        r'Open = convergence to a point of $U$ forces eventual membership.', '', ''),
    'thm:closed-equiv': (
        r'State the three equivalent characterisations of closed sets.',
        r'TFAE: (i) $X\setminus F$ is open; (ii) $F$ contains all its limit points ($F\supseteq F\'$); (iii) $F$ is sequentially closed.',
        r'(i)$\Rightarrow$(iii): if $x_n\in F\to x\notin F$ then $x\in X\setminus F$ open, so $x_n$ is eventually in $X\setminus F$, contradicting $x_n\in F$.',
        '', ''),
    'prop:sequential-duality': (
        r'State Sequential Duality of open and closed sets.',
        r'$F=X\setminus U$ is closed iff for every sequence $x_n\in F$ with $x_n\to x$, we have $x\in F$; equivalently $U$ captures limits of sequences converging into it.',
        '', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/limit-points/notes-limit-points.tex', {
    'def:limit-point': (
        r'Define a limit point of $E$.',
        r'$x$ is a limit point of $E$ if $\forall r>0,B_r(x)\cap(E\setminus\{x\})\neq\varnothing$.',
        r'Every ball around $x$ must contain a point of $E$ other than $x$ itself.',
        '', 'thm:seq-char-limit'),
    'def:isolated-point': (
        r'Define an isolated point.',
        r'$a\in E$ is isolated if $a\notin E\'$: some ball $B_r(a)$ contains no other point of $E$.',
        r'Isolated = no sequence in $E\setminus\{a\}$ approaches $a$.', '', ''),
    'thm:seq-char-limit': (
        r'State the Sequential Characterisation of Limit Points.',
        r'$x\in E\'$ iff there exists a sequence $(x_n)\subseteq E$ with all $x_n\neq x$ and $x_n\to x$.',
        r'Forward: take $x_n\in B_{1/n}(x)\cap(E\setminus\{x\})$. Backward: any sequence converging to $x$ witnesses every ball is met.', '', ''),
    'cor:inf-nbhd': (
        r'State the Infinite Neighbourhood Characterisation of limit points.',
        r'$x\in E\'$ iff $\forall r>0$, $B_r(x)\cap E$ is infinite.',
        r'If some ball met $E$ in only finitely many points, the smallest distance to those finitely many points gives a ball missing $E\setminus\{x\}$.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/limit-points/notes-derived-set-closure.tex', {
    'def:derived-set': (
        r"Define the derived set $E'$.",
        r"$E':=\{x\in X:x\text{ is a limit point of }E\}$.",
        r"$E'$ is always closed; $E$ is closed iff $E'\subseteq E$; $\overline{E}=E\cup E'$.",
        '', "thm:derived-closed, thm:closed-derived, thm:closure-formula"),
    'def:perfect-set': (
        r'Define a perfect set.',
        r"$E$ is perfect if $E$ is closed and $E=E'$ (every point is a limit point).",
        r'Perfect sets are uncountable if nonempty — e.g.\ the Cantor set.', '', ''),
    'thm:derived-closed': (
        r"State: the derived set $E'$ is always closed.",
        r"For any $E\subseteq X$, $E'$ is closed.",
        r"Take a limit point $x$ of $E'$; a sequence of limit points of $E$ approaching $x$ and a diagonal argument shows $x\in E'$.", '', ''),
    'thm:closed-derived': (
        r"State: $E$ is closed iff $E'\subseteq E$.",
        r"$E$ is closed iff $E'\subseteq E$.",
        r"$E$ closed $\Rightarrow$ sequentially closed $\Rightarrow$ limits of sequences stay in $E$ $\Rightarrow$ limit points in $E$.", '', ''),
    'thm:closure-formula': (
        r"State the Closure Formula.",
        r"$\overline{E}=E\cup E'$.",
        r"Adherent points are either in $E$ (trivially) or are limit points of $E$ (approached by sequences in $E$).", '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/cauchy/notes-cauchy.tex', {
    'def:cauchy-sequence': (
        r'Define a Cauchy sequence in a metric space.',
        r'$(x_n)$ is Cauchy if $\forall\varepsilon>0\,\exists N\,\forall m,n\ge N:d(x_m,x_n)<\varepsilon$.',
        r'Terms eventually become arbitrarily close to each other — no limit needed in the definition.',
        r'In $\mathbb{Q}$: $(1,1.4,1.41,\ldots)$ is Cauchy but has no limit in $\mathbb{Q}$.',
        'prop:convergent-implies-cauchy, prop:cauchy-bounded'),
    'prop:convergent-implies-cauchy': (
        r'State: convergent implies Cauchy.',
        r'Every convergent sequence in a metric space is Cauchy.',
        r'If $x_n\to x$: for $\varepsilon>0$ pick $N$ so $d(x_n,x)<\varepsilon/2$; then $d(x_m,x_n)\le d(x_m,x)+d(x,x_n)<\varepsilon$.',
        '', 'prop:cauchy-bounded'),
    'prop:cauchy-bounded': (
        r'State: Cauchy sequences are bounded.',
        r'Every Cauchy sequence in a metric space is bounded.',
        r'Take $\varepsilon=1$, find $N$; all terms after $N$ lie in $B_1(x_N)$; finitely many terms before $N$ contribute a finite bound.',
        '', ''),
    'prop:cauchy-subsequence': (
        r'State: Cauchy sequence with a convergent subsequence converges.',
        r'If $(x_n)$ is Cauchy and $(x_{n_k})\to x$, then $x_n\to x$.',
        r'Triangle: $d(x_n,x)\le d(x_n,x_{n_k})+d(x_{n_k},x)$; first term is small by Cauchy, second by subsequence convergence.', '', ''),
    'prop:cauchy-unique-limit': (
        r'State: Cauchy sequences have at most one limit.',
        r'If $(x_n)$ is Cauchy and $x_n\to x$ and $x_n\to y$, then $x=y$.',
        r'$d(x,y)\le d(x,x_n)+d(x_n,y)\to 0$.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/completeness/notes-completeness.tex', {
    'def:complete-metric-space': (
        r'Define a complete metric space.',
        r'$(X,d)$ is complete if every Cauchy sequence converges to a point of $X$.',
        r'$\mathbb{R}$ is complete; $\mathbb{Q}$ is not.',
        r'$(1,1.4,1.41,\ldots)\to\sqrt{2}\notin\mathbb{Q}$ so $\mathbb{Q}$ is incomplete.',
        'prop:closed-subspace-complete, prop:complete-subspace-closed'),
    'def:completion': (
        r'Define the completion of a metric space.',
        r'A completion is a complete metric space $(\tilde{X},\tilde{d})$ with an isometric embedding $\iota:X\hookrightarrow\tilde{X}$ such that $\iota(X)$ is dense in $\tilde{X}$.',
        r'$\mathbb{R}$ is the completion of $\mathbb{Q}$.',
        '', 'thm:completion'),
    'prop:closed-subspace-complete': (
        r'State: closed subspace of a complete space is complete.',
        r'If $(X,d)$ is complete and $F\subseteq X$ is closed, then $(F,d|_F)$ is complete.',
        r'A Cauchy sequence in $F$ is Cauchy in $X$; converges in $X$; limit is in $F$ since $F$ is closed.', '', ''),
    'prop:complete-subspace-closed': (
        r'State: complete subspace is closed.',
        r'If $(F,d|_F)$ is complete then $F$ is closed in $X$.',
        r'If $x_n\in F$ with $x_n\to x$ in $X$: $(x_n)$ is Cauchy in $F$; completeness gives limit in $F$; uniqueness of limits gives $x\in F$.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/isometries/notes-isometries.tex', {
    'def:isometry': (
        r'Define an isometry.',
        r'$f:X\to Y$ is an isometry if $d_Y(f(x),f(y))=d_X(x,y)$ for all $x,y\in X$. A bijective isometry is an isometric isomorphism.',
        r'Isometries always injective; preserve all metric structure.',
        '', 'prop:isometry-injective, prop:isometry-preserves-balls'),
    'def:subspace-metric': (
        r'Define the subspace metric.',
        r'For $A\subseteq X$: $d|_A(x,y):=d(x,y)$ for $x,y\in A$. The pair $(A,d|_A)$ is a metric subspace.',
        '', '', ''),
    'def:rigid-motion': (
        r'Define a rigid motion of $\mathbb{R}^n$.',
        r'A rigid motion is a bijective isometry $f:\mathbb{R}^n\to\mathbb{R}^n$ w.r.t.\ the Euclidean metric.',
        r'Every rigid motion is of the form $f(x)=Rx+a$ with $R\in\mathrm{O}(n)$ and $a\in\mathbb{R}^n$.',
        '', 'thm:classification-rigid-motions'),
    'prop:isometry-injective': (
        r'State: every isometry is injective.',
        r'Every isometry $f:X\to Y$ is injective.',
        r'If $f(x)=f(y)$ then $d_X(x,y)=d_Y(f(x),f(y))=0$ so $x=y$.', '', ''),
    'prop:inverse-isometry': (
        r'State: the inverse of a bijective isometry is a bijective isometry.',
        r'If $f:X\to Y$ is a bijective isometry then $f^{-1}:Y\to X$ is also a bijective isometry.',
        '', '', ''),
    'prop:isometry-preserves-balls': (
        r'State: isometries preserve open balls.',
        r'If $f:X\to Y$ is an isometry: $f(B_r(x))=B_r(f(x))$ for all $x\in X$, $r>0$.',
        '', '', ''),
    'prop:isometry-preserves-diameter': (
        r'State: isometries preserve diameter.',
        r'If $f:X\to Y$ is an isometry then $\operatorname{diam}(f(A))=\operatorname{diam}(A)$ for all $A\subseteq X$.',
        '', '', ''),
    'prop:isometry-preserves-open': (
        r'State: bijective isometries preserve open/closed sets.',
        r'If $f:X\to Y$ is a bijective isometry: $U\subseteq X$ is open iff $f(U)\subseteq Y$ is open.',
        '', '', ''),
    'prop:isometry-preserves-convergence': (
        r'State: isometries preserve convergence and Cauchy sequences.',
        r'$f$ bijective isometry: $x_n\to x$ iff $f(x_n)\to f(x)$; $(x_n)$ Cauchy iff $(f(x_n))$ Cauchy.',
        '', '', ''),
    'prop:isometry-preserves-bounded': (
        r'State: isometries preserve boundedness.',
        r'$f$ isometry: $A$ is bounded iff $f(A)$ is bounded.',
        '', '', ''),
    'prop:isometry-group': (
        r'State: isometries form a group.',
        r'$\mathrm{Isom}(X,d):=\{f:X\to X\mid f\text{ bijective isometry}\}$ is a group under composition.',
        '', '', ''),
    'prop:rotations-are-isometries': (
        r'State: orthogonal maps are isometries.',
        r'If $R:\mathbb{R}^n\to\mathbb{R}^n$ is orthogonal ($R^\top R=I$) then $R$ is an isometry.',
        r'$\|Rx-Ry\|_2=\|R(x-y)\|_2=\|x-y\|_2$ since orthogonal maps preserve norms.', '', ''),
    'prop:translations-are-isometries': (
        r'State: translations are isometries.',
        r'$T_a(x):=x+a$ is an isometry of $\mathbb{R}^n$ for any $a\in\mathbb{R}^n$.',
        r'$\|T_a(x)-T_a(y)\|=\|(x+a)-(y+a)\|=\|x-y\|$.', '', ''),
    'prop:reflections-are-isometries': (
        r'State: reflections are isometries.',
        r'Reflection across any hyperplane in $\mathbb{R}^n$ is an isometry.',
        r'Reflection is an orthogonal map (matrix with determinant $-1$), hence an isometry.', '', ''),
    'thm:classification-rigid-motions': (
        r'State the Classification of Euclidean Rigid Motions.',
        r'Every rigid motion $f:\mathbb{R}^n\to\mathbb{R}^n$ has the form $f(x)=Rx+a$ where $R\in\mathrm{O}(n)$ and $a\in\mathbb{R}^n$.',
        '', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-nowhere-dense-baire.tex', {
    'def:nowhere-dense': (
        r'Define a nowhere-dense set.',
        r'$E\subseteq X$ is nowhere-dense if $\operatorname{int}(\overline{E})=\varnothing$: the closure has empty interior.',
        r'Nowhere-dense = cannot swallow any open ball.',
        r'$\mathbb{Z}$ is nowhere-dense in $\mathbb{R}$; $\mathbb{Q}$ is NOT (its closure $\mathbb{R}$ has nonempty interior).',
        'prop:nowhere-dense-equiv, def:meager'),
    'def:meager': (
        r'Define a meager (first category) set.',
        r'$A\subseteq X$ is meager if $A=\bigcup_{n=1}^\infty E_n$ where each $E_n$ is nowhere-dense.',
        r'Meager = topologically small: countable union of nowhere-dense pieces.',
        '', 'thm:baire-preview'),
    'thm:baire-preview': (
        r'State the Baire Category Theorem.',
        r'In a complete metric space $X$: $X$ is not meager; i.e.\ if $X=\bigcup_{n=1}^\infty E_n$ then at least one $\overline{E_n}$ has nonempty interior.',
        r'Equivalently: a countable intersection of open dense sets is dense.',
        '', ''),
    'prop:nowhere-dense-equiv': (
        r'State the complement characterisation of nowhere-dense.',
        r'$E$ is nowhere-dense iff $X\setminus\overline{E}$ is dense in $X$.',
        r'$\operatorname{int}(\overline{E})=\varnothing$ iff the complement of $\overline{E}$ meets every open ball.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-totally-bounded.tex', {
    'def:totally-bounded': (
        r'Define a totally bounded set.',
        r'$A\subseteq X$ is totally bounded if $\forall\varepsilon>0$ there exist finitely many points $x_1,\ldots,x_n\in X$ with $A\subseteq\bigcup_{i=1}^n B_\varepsilon(x_i)$.',
        r'For any $\varepsilon$, finitely many $\varepsilon$-balls suffice to cover $A$.',
        r'An infinite set with the discrete metric is bounded but not totally bounded.',
        'thm:gen-heine-borel, prop:totally-bounded-Cauchy'),
    'prop:totally-bounded-implies-bounded': (
        r'State: totally bounded implies bounded.',
        r'If $A\subseteq X$ is totally bounded then $A$ is bounded.',
        r'The finite cover by $\varepsilon$-balls has bounded union.', '', ''),
    'prop:bounded-not-totally-bounded': (
        r'State: bounded does not imply totally bounded.',
        r'The unit ball in an infinite-dimensional space (or any infinite discrete metric space) is bounded but not totally bounded.',
        '', '', ''),
    'prop:totally-bounded-Rn': (
        r'State: in $\mathbb{R}^n$, bounded iff totally bounded.',
        r'$A\subseteq\mathbb{R}^n$ is totally bounded iff it is bounded.',
        r'Bounded subsets of $\mathbb{R}^n$ can always be covered by finitely many $\varepsilon$-boxes.', '', ''),
    'prop:totally-bounded-Cauchy': (
        r'State: totally bounded sets have Cauchy subsequences.',
        r'If $A\subseteq X$ is totally bounded then every sequence in $A$ has a Cauchy subsequence.',
        r'Diagonal argument: at each stage pass to a subsequence in a ball of radius $1/k$.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-distance-between-sets.tex', {
    'def:dist-point-set': (
        r'Define the distance from a point to a set.',
        r'$d(x,A):=\inf\{d(x,a):a\in A\}$ for $x\in X$, $A\neq\varnothing$.',
        r'$d(x,A)=0\iff x\in\overline{A}$.', '', 'prop:dist-point-set-closure'),
    'def:dist-sets': (
        r'Define the distance between two sets.',
        r'$d(A,B):=\inf\{d(a,b):a\in A,b\in B\}$ for nonempty $A,B\subseteq X$.',
        r'$d(A,B)=0$ does NOT imply $A\cap B\neq\varnothing$.', '', ''),
    'prop:dist-point-set-closure': (
        r'State: $d(x,A)=0$ iff $x\in\overline{A}$.',
        r'$d(x,A)=0\iff x\in\overline{A}$.',
        r'$d(x,A)=0$ means every ball around $x$ meets $A$, which is exactly the definition of adherence.', '', ''),
    'prop:dist-sets-not-metric': (
        r'State: $d(A,B)=0$ does not imply $A\cap B\neq\varnothing$.',
        r'In general $d(A,B)=0$ does not imply $A\cap B\neq\varnothing$.',
        r'Example: $A=(0,1)$, $B=\{0\}$ in $\mathbb{R}$: $d(A,B)=0$ but $A\cap B=\varnothing$.', '', ''),
    'prop:dist-sets-symmetric': (
        r'State: set distance is symmetric.',
        r'$d(A,B)=d(B,A)$ for nonempty $A,B\subseteq X$.', '', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-multiple-metrics.tex', {
    'def:equivalent-metrics': (
        r'Define equivalent metrics.',
        r'Metrics $d_1,d_2$ on $X$ are equivalent if they induce the same topology: every $d_1$-open set is $d_2$-open and vice versa.',
        r'Equivalent metrics have the same convergent sequences and the same open sets.', '', ''),
    'def:ultrametric': (
        r'Define an ultrametric.',
        r'$d$ is an ultrametric if $d(x,z)\le\max\{d(x,y),d(y,z)\}$ for all $x,y,z\in X$ (strong triangle inequality).',
        r'In an ultrametric space, every triangle is isosceles with the two equal sides being the longest.', '', ''),
    'def:antimetric': (
        r'Define an antimetric.',
        r'$d:X\times X\to\mathbb{R}$ is an antimetric if (A1) $d\ge 0$; (A2) $d(x,y)=0\iff x=y$; (A3) symmetry; and the reversed triangle inequality $d(x,z)\ge d(x,y)+d(y,z)$.',
        r'Antimetrics are degenerate: any antimetric must be identically zero.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/example-spaces/notes-example-spaces.tex', {
    'def:lp-rn': (
        r'Define the $\ell^p$ metric on $\mathbb{R}^n$.',
        r'For $1\le p<\infty$: $d_p(x,y)=(\sum_{i=1}^n|x_i-y_i|^p)^{1/p}$. For $p=\infty$: $d_\infty(x,y)=\max_i|x_i-y_i|$.',
        r'$p=1$: taxicab. $p=2$: Euclidean. $p=\infty$: sup-norm.', '', ''),
    'def:ell2': (
        r'Define the $\ell^2$ sequence space.',
        r'$\ell^2:=\{(x_k)_{k\ge 1}:x_k\in\mathbb{R},\,\sum_{k=1}^\infty x_k^2<\infty\}$ with $d_2(x,y)=(\sum|x_k-y_k|^2)^{1/2}$.',
        r'$\ell^2$ is the infinite-dimensional analogue of Euclidean space; it is complete.', '', ''),
    'def:ellinf': (
        r'Define the $\ell^\infty$ sequence space.',
        r'$\ell^\infty:=\{(x_k)_{k\ge 1}:\sup_k|x_k|<\infty\}$ with $d_\infty(x,y)=\sup_k|x_k-y_k|$.',
        r'$\ell^\infty$ is bounded sequences with the sup metric; it is complete.', '', ''),
    'def:ell1': (
        r'Define the $\ell^1$ sequence space.',
        r'$\ell^1:=\{(x_k)_{k\ge 1}:\sum_{k=1}^\infty|x_k|<\infty\}$ with $d_1(x,y)=\sum_k|x_k-y_k|$.',
        r'$\ell^1$ is absolutely summable sequences; it is complete and $\ell^1\subsetneq\ell^2\subsetneq\ell^\infty$.', '', ''),
    'def:cont-uniform': (
        r'Define $C([a,b])$ with the uniform metric.',
        r'$C([a,b])$ with $d_\infty(f,g)=\sup_{x\in[a,b]}|f(x)-g(x)|$ — continuous functions with the uniform (sup-norm) metric.',
        r'Convergence in $d_\infty$ is uniform convergence. This space is complete.', '', ''),
    'def:cont-l1': (
        r'Define $C([a,b])$ with the $L^1$ metric.',
        r'$C([a,b])$ with $d_1(f,g)=\int_a^b|f(x)-g(x)|\,dx$ — continuous functions with the $L^1$ metric.',
        r'$L^1$-convergence does not imply pointwise convergence; this space is NOT complete.', '', ''),
    'def:poly-space': (
        r'Define the polynomial metric space $\mathcal{P}([a,b])$.',
        r'$\mathcal{P}([a,b])$ with $d_\infty$ — polynomials on $[a,b]$ with the uniform metric.',
        r'Dense in $C([a,b])$ by Weierstrass; not complete since limits of polynomials need not be polynomials.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-product-metric.tex', {
    'def:product-metric': (
        r'Define the product metric space.',
        r'For metric spaces $(X,d_X)$ and $(Y,d_Y)$, define $d$ on $X\times Y$ by any of $d_1$, $d_2$, or $d_\infty$ applied to coordinate distances; all three are equivalent.',
        r'Convergence in product = coordinatewise convergence.', '', 'prop:product-convergence'),
    'def:product-coord-metrics': (
        r'Define coordinate metric spaces in a product.',
        r'The $X$-coordinate of $(x,y)\in X\times Y$ is $x$; the $Y$-coordinate is $y$. Coordinate projections $\pi_X,\pi_Y$ are continuous.',
        '', '', ''),
    'prop:product-convergence': (
        r'State convergence in the product metric.',
        r'$(x_n,y_n)\to(x,y)$ in $X\times Y$ iff $x_n\to x$ in $X$ and $y_n\to y$ in $Y$.',
        r'Since product metrics are equivalent and each is controlled by coordinate distances.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-dense-separable.tex', {
    'def:nowhere-dense': (
        r'Define a nowhere-dense set.',
        r'$E$ is nowhere-dense if $\operatorname{int}(\overline{E})=\varnothing$.',
        '', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-properties-metric-space.tex', {
    'def:diameter': (
        r'Define the diameter of a set.',
        r'$\operatorname{diam}(A):=\sup\{d(x,y):x,y\in A\}$. $\operatorname{diam}(\varnothing)=0$.',
        r'Diameter measures the greatest spread in a set.', '', ''),
    'def:bounded-set': (
        r'Define a bounded set in a metric space.',
        r'$A\subseteq X$ is bounded if $\operatorname{diam}(A)<\infty$, i.e.\ $\exists M: d(x,y)\le M$ for all $x,y\in A$.',
        r'Equivalently: $A$ is contained in some open ball.', '', ''),
    'def:isolated-point-ms': (
        r'Define an isolated point in a metric space.',
        r'$x\in X$ is isolated if $\{x\}$ is open in $X$, i.e.\ $\exists r>0: B_r(x)=\{x\}$.',
        '', '', ''),
    'def:discrete-metric-space': (
        r'Define a discrete metric space.',
        r'$(X,d)$ is discrete if every subset is open, equivalently every point is isolated.',
        r'The discrete metric $d(x,y)=\mathbf{1}_{x\neq y}$ makes any set discrete.', '', ''),
    'thm:monotonicity-of-diameter': (
        r'State Monotonicity of Diameter.',
        r'If $A\subseteq B\subseteq X$ then $\operatorname{diam}(A)\le\operatorname{diam}(B)$.',
        r'$\operatorname{diam}(A)=\sup_{A\times A}d$; restricting to a subset can only decrease or preserve the sup.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/completeness/notes-co-cauchy.tex', {
    'def:co-cauchy': (
        r'Define co-Cauchy (equivalent) sequences.',
        r'Cauchy sequences $(p_n)$ and $(q_n)$ in $(X,d)$ are co-Cauchy if $d(p_n,q_n)\to 0$.',
        r'Co-Cauchy = the two sequences converge to the same point in the completion.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-unit-ball-characterisations.tex', {
    'def:ball-d1': (
        r'Describe the unit ball of $d_1$ on $\mathbb{R}^2$.',
        r'$B_1^{d_1}(0)=\{(x_1,x_2):|x_1|+|x_2|<1\}$ — a diamond (rotated square).',
        '', '', ''),
    'def:ball-d2': (
        r'Describe the unit ball of $d_2$ on $\mathbb{R}^2$.',
        r'$B_1^{d_2}(0)=\{(x_1,x_2):x_1^2+x_2^2<1\}$ — an open disk.',
        '', '', ''),
    'def:ball-dinf': (
        r'Describe the unit ball of $d_\infty$ on $\mathbb{R}^2$.',
        r'$B_1^{d_\infty}(0)=\{(x_1,x_2):|x_1|<1\text{ and }|x_2|<1\}$ — an open square.',
        '', '', ''),
    'def:ball-std-std': (
        r'Describe the unit ball under the standard$\times$standard product metric on $\mathbb{R}^2$.',
        r'Using $d_\mathrm{sum}=|x_1-y_1|+|x_2-y_2|$: the unit ball is the same diamond as $d_1$.',
        '', '', ''),
    'def:ball-std-disc': (
        r'Describe the unit ball under the standard$\times$discrete product metric on $\mathbb{R}^2$.',
        r'Using $d=|x_1-x_2|+\mathbf{1}_{y_1\neq y_2}$: the unit ball centred at $(0,0)$ is $\{(x,0):|x|<1\}$ — a horizontal open interval.',
        '', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-axiom-verification.tex', {
    'def:r2-three-metrics': (
        r'State the three standard metrics on $\mathbb{R}^2$.',
        r'$d_1(p,q)=|x_1-y_1|+|x_2-y_2|$ (taxicab); $d_2(p,q)=\sqrt{(x_1-y_1)^2+(x_2-y_2)^2}$ (Euclidean); $d_\infty(p,q)=\max(|x_1-y_1|,|x_2-y_2|)$ (sup).',
        r'Unit balls: diamond / disk / square.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-product-metric.tex', {
    'def:product-metrics': (
        r'State the three equivalent product metrics.',
        r'$d_2((x_1,y_1),(x_2,y_2))=\sqrt{d_X^2+d_Y^2}$; $d_1=d_X+d_Y$; $d_\infty=\max(d_X,d_Y)$; all are equivalent.',
        '', '', 'prop:product-metrics-equiv'),
    'prop:product-metrics-equiv': (
        r'State the equivalence of product metrics.',
        r'$d_\infty\le d_2\le d_1\le 2d_\infty$ on $X\times Y$; in particular $d_1,d_2,d_\infty$ are equivalent.',
        '', '', ''),
    'prop:Rn-as-product': (
        r'State: $\mathbb{R}^n$ is an iterated product metric space.',
        r'$(\mathbb{R}^n,d_2)$ is the $n$-fold product of $(\mathbb{R},|\cdot|)$ with itself.',
        '', '', ''),
})

# ── REAL ANALYSIS ─────────────────────────────────────────────────────────────

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/01-inequalities-bounding.tex', {
    'prop:order-arithmetic': (
        r'State the order arithmetic properties of $\mathbb{R}$.',
        r'(i) $a\le b,c\le d\Rightarrow a+c\le b+d$; (ii) $a\le b,c>0\Rightarrow ac\le bc$; (iii) $a\le b,c<0\Rightarrow ac\ge bc$.',
        r'Multiplication by a negative flips the inequality.', '', ''),
    'thm:triangle-inequality': (
        r'State the triangle inequality for $\mathbb{R}$.',
        r'$|a+b|\le|a|+|b|$ for all $a,b\in\mathbb{R}$.',
        r'Also: $|a-b|\le|a|+|b|$ and $||a|-|b||\le|a-b|$ (reverse triangle inequality).', '', ''),
    'prop:convergent-bounded': (
        r'State: convergent sequences are bounded.',
        r'If $(b_n)\to b$ then $\exists M>0$ with $|b_n|\le M$ for all $n$.',
        r'Take $\varepsilon=1$: beyond $N$ all terms lie in $(b-1,b+1)$; finitely many terms before $N$ give a finite bound.',
        '', ''),
    'prop:eps-char-sup': (
        r'State the $\varepsilon$-characterisation of $\sup$.',
        r'If $s=\sup S$ then $\forall\varepsilon>0\,\exists x\in S: x>s-\varepsilon$.',
        r'The sup can always be approximated from below by elements of $S$.', '', ''),
    'prop:monotone-approx-bounds': (
        r'State Monotone Approximation of Bounds.',
        r'For nonempty bounded $S\subseteq\mathbb{R}$: there exist monotone sequences $(x_n),(y_n)\subseteq S$ with $x_n\to\sup S$ and $y_n\to\inf S$.',
        r'Repeatedly apply the $\varepsilon$-characterisation of sup with $\varepsilon=1/n$.', '', ''),
    'prop:io-ev-dichotomy': (
        r'State the infinitely-often/eventually dichotomy.',
        r'For any predicate $P$ on $\mathbb{N}$: either $P$ holds infinitely often or $\neg P$ holds eventually (not both, not neither).',
        r'$\neg P$ eventually $\Leftrightarrow$ $P$ holds only finitely often $\Leftrightarrow$ not infinitely often.', '', ''),
})

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/02-completeness-construction.tex', {
    'def:peak': (
        r'Define a peak term of a sequence.',
        r'$a_m$ is a peak term of $(a_n)$ if $a_m\ge a_n$ for all $n\ge m$.',
        r'A peak is a term that is at least as large as everything that follows it.',
        '', ''),
    'thm:monotone-subsequence': (
        r'State the Monotone Subsequence Theorem.',
        r'Every sequence in $\mathbb{R}$ has a monotone subsequence.',
        r'Cases: infinitely many peak terms give a decreasing subsequence; finitely many give an increasing one.',
        '', ''),
    'prop:bw-bisection': (
        r'State Bolzano--Weierstrass via bisection.',
        r'Every bounded sequence in $\mathbb{R}$ has a convergent subsequence.',
        r'Bisect the bounding interval; infinitely many terms in one half; repeat diagonally.',
        '', ''),
    'prop:eps-char-sup': (
        r'State the $\varepsilon$-characterisation of $\sup$ (proof-techniques version).',
        r'If $s=\sup S$ then $\forall\varepsilon>0\,\exists x\in S: x>s-\varepsilon$.',
        '', '', ''),
    'prop:monotone-approx-bounds': (
        r'State Monotone Approximation of Bounds (proof-techniques version).',
        r'For nonempty bounded $S$: there exist monotone sequences in $S$ converging to $\sup S$ and $\inf S$.',
        '', '', ''),
})

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/03-predicate-walking.tex', {
    'def:inf-often-eventually': (
        r'Define "infinitely often" and "eventually".',
        r'"$P(n)$ holds infinitely often" means $\{n:P(n)\}$ is infinite. "$P(n)$ holds eventually" means $\exists N\,\forall n\ge N: P(n)$.',
        '', '', ''),
    'prop:io-ev-dichotomy': (
        r'State the IO/EV dichotomy.',
        r'For any predicate $P$: exactly one of ($P$ infinitely often) or ($\neg P$ eventually) holds.',
        '', '', ''),
})

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/04-bisection-nested-intervals.tex', {
    'def:bisection-sequence': (
        r'Define a bisection sequence.',
        r'A bisection sequence is a sequence of nested closed intervals $[a_n,b_n]$ where $[a_{n+1},b_{n+1}]$ is one of the two halves of $[a_n,b_n]$, so $b_n-a_n=(b_1-a_1)/2^{n-1}$.',
        r'Lengths $\to 0$, so the intersection is a single point by the Nested Interval Property.', '', ''),
    'prop:ivt-bisection': (
        r'State the Intermediate Value Theorem.',
        r'If $f:[a,b]\to\mathbb{R}$ is continuous and $f(a)<L<f(b)$ (or $f(a)>L>f(b)$), then $\exists c\in(a,b): f(c)=L$.',
        r'Bisection: maintain $f(a_n)<L<f(b_n)$; the intersection point $c$ satisfies $f(c)=L$ by continuity.', '', ''),
})

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/05-residue-partition.tex', {
    'thm:k-periodicity': (
        r'State the $k$-periodicity / finite partition convergence principle.',
        r'If $\mathbb{N}=E_1\cup\cdots\cup E_k$ (finite partition, each $E_i$ infinite) and each subsequence $(a_n)_{n\in E_i}\to L$, then $(a_n)\to L$.',
        r'For any $\varepsilon>0$, each sub-sequence eventually satisfies $|a_n-L|<\varepsilon$; take the max of the finitely many thresholds.',
        '', ''),
    'prop:residue-divergence': (
        r'State the Residue Divergence Criterion.',
        r'If residue subsequences $(a_{kn+r})\to L$ and $(a_{kn+s})\to M$ with $L\neq M$, then $(a_n)$ diverges.',
        '', '', ''),
    'thm:inductive-selection': (
        r'State the Inductive Selection Principle.',
        r'From any sequence with infinitely many terms in a set $A$, one can inductively extract a subsequence entirely in $A$.',
        r'At each step, use infinitely-many-terms-in-$A$ to choose the next index.', '', ''),
    'prop:r-uncountable': (
        r'State: $\mathbb{R}$ is uncountable.',
        r'$\mathbb{R}$ is uncountable (proved by Cantor\'s diagonal argument or nested intervals).',
        r'Any proposed enumeration $(r_1,r_2,\ldots)$ can be contradicted by a nested-interval construction.', '', ''),
    'thm:alternating-series-test': (
        r'State the Alternating Series Test.',
        r'If $(b_n)$ is decreasing with $b_n\to 0$ then $\sum_{n=1}^\infty(-1)^{n+1}b_n$ converges.',
        r'Partial sums oscillate and squeeze: even and odd partial sums are monotone and converge to the same limit.', '', ''),
    'def:LUB': (
        r'Define the Least Upper Bound (LUB) property.',
        r'$\mathbb{R}$ has the LUB property: every nonempty subset bounded above has a least upper bound in $\mathbb{R}$.',
        r'This is the completeness axiom for $\mathbb{R}$; $\mathbb{Q}$ does not have it.', '', 'thm:LUB-iff-MCT'),
    'thm:LUB-iff-MCT': (
        r'State the equivalence LUB $\Leftrightarrow$ MCT.',
        r'TFAE: (i) $\mathbb{R}$ has the LUB property; (ii) every bounded monotone sequence converges.',
        r'(i)$\Rightarrow$(ii): sup of values is the limit. (ii)$\Rightarrow$(i): approximate the sup by a monotone sequence.', '', ''),
    'thm:nested-interval-property': (
        r'State the Nested Interval Property (real-analysis version).',
        r'Nested closed intervals have nonempty intersection.',
        '', '', ''),
})

# ── SEQUENCES ─────────────────────────────────────────────────────────────────

inject('volume-iii/analysis/sequences/notes/sequences/notes-sequences.tex', {
    'thm:seq-determined-by-values': (
        r'State: a sequence is determined by its values.',
        r'If $x_n=y_n$ for all $n\in\mathbb{N}$ then $x=y$ as sequences (functions $\mathbb{N}\to X$).',
        r'Sequences are functions; function equality is pointwise.', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-convergence.tex', {
    'thm:uniqueness-limits': (
        r'State Uniqueness of Limits.',
        r'A sequence can converge to at most one limit: if $x_n\to L$ and $x_n\to M$ then $L=M$.',
        r'$0\le|L-M|\le|L-x_n|+|x_n-M|\to 0$; squeeze $|L-M|=0$.',
        '', ''),
    'thm:conv-seq-bounded': (
        r'State: convergent sequences are bounded.',
        r'If $(x_n)$ converges then $(x_n)$ is bounded.',
        r'Take $\varepsilon=1$; terms beyond $N$ lie within 1 of the limit; finitely many terms before $N$ are bounded.',
        '', ''),
    'prop:bounded-not-convergent': (
        r'State: bounded does not imply convergent.',
        r'$((-1)^n)$ is bounded but divergent — it oscillates.', '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-k-tails.tex', {
    'thm:conv-tail-property': (
        r'State: convergence is a tail property.',
        r'$(x_n)\to L$ iff every $k$-tail $(x_{n+k})_{n\ge 1}\to L$.',
        r'Adding or removing finitely many terms does not affect convergence or its limit.', '', ''),
    'thm:finite-modification': (
        r'State the Finite Modification Theorem.',
        r'If $(x_n)$ and $(y_n)$ differ in only finitely many terms: either both converge to the same limit or neither converges.',
        '', '', 'cor:tails-converge'),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-algebra-of-sequences.tex', {
    'thm:algebra-of-limits': (
        r'State the Algebra of Limits.',
        r'If $x_n\to x$ and $y_n\to y$: $x_n+y_n\to x+y$; $x_n-y_n\to x-y$; $cx_n\to cx$; $x_ny_n\to xy$; if $y\neq 0$ then $x_n/y_n\to x/y$.',
        r'Each rule follows from the $\varepsilon$-$N$ definition; products use the rectangle decomposition.',
        '', ''),
    'thm:order-limit-theorem': (
        r'State the Order Limit Theorem.',
        r'If $x_n\le y_n$ for all $n$ and $x_n\to x$, $y_n\to y$, then $x\le y$.',
        r'If $x>y$, take $\varepsilon=(x-y)/2$; eventually $x_n>y_n$, contradiction.',
        r'Strict inequality $x_n<y_n$ does NOT force $x<y$: $(0)\to 0$ and $(1/n)\to 0$.',
        ''),
    'thm:squeeze-theorem': (
        r'State the Squeeze Theorem.',
        r'If $x_n\le y_n\le z_n$ for all $n$ and $x_n\to L$, $z_n\to L$, then $y_n\to L$.',
        r'For $\varepsilon>0$: eventually $L-\varepsilon<x_n\le y_n\le z_n<L+\varepsilon$.',
        '', ''),
    'cor:limit-respects-upper-bounds': (
        r'State: limits respect upper bounds.',
        r'If $x_n\le b$ for all $n$ and $x_n\to x$, then $x\le b$.',
        r'Apply OLT with $y_n=b$.', '', ''),
    'cor:conv-times-bounded': (
        r'State: convergent times bounded is bounded.',
        r'If $(x_n)\to x$ and $(y_n)$ is bounded then $(x_ny_n)$ is bounded.',
        r'$(x_n)$ is bounded (convergent $\Rightarrow$ bounded); product of bounded sequences is bounded.', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-monotone-sequences.tex', {
    'thm:mct': (
        r'State the Monotone Convergence Theorem (MCT).',
        r'Every bounded monotone sequence converges: an increasing bounded sequence converges to $\sup\{x_n\}$; decreasing bounded below converges to $\inf\{x_n\}$.',
        r'Let $L=\sup\{x_n\}$; given $\varepsilon$, the $\varepsilon$-characterisation gives $N$ with $x_N>L-\varepsilon$; monotonicity propagates.',
        r'Fails without boundedness: $(n)$ is monotone and diverges.',
        'thm:bolzano-weierstrass'),
    'thm:MCT': (
        r'State the Monotone Convergence Theorem.',
        r'Every bounded monotone sequence of real numbers converges.',
        '', '', 'thm:LUB-iff-MCT'),
    'thm:LUB-iff-MCT': (
        r'State the equivalence LUB $\Leftrightarrow$ MCT.',
        r'TFAE: (i) LUB property; (ii) every bounded monotone sequence converges.',
        '', '', ''),
    'def:LUB': (
        r'Define the Least Upper Bound property.',
        r'$\mathbb{R}$ has the LUB property: every nonempty subset bounded above has a supremum in $\mathbb{R}$.',
        '', '', ''),
    'prop:monotone-approx-bounds': (
        r'State Monotone Approximation of Bounds.',
        r'For nonempty bounded $S$: there exist monotone $(x_n),(y_n)\subseteq S$ with $x_n\to\sup S$, $y_n\to\inf S$.',
        '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-subsequences.tex', {
    'prop:conv-inherited': (
        r'State: convergence is inherited by subsequences.',
        r'If $(a_n)\to L$ then every subsequence $(a_{n_k})\to L$.',
        r'For $\varepsilon>0$, pick $N$ for the original sequence; since $n_k\ge k$, $k\ge N$ implies $n_k\ge N$.',
        '', ''),
    'prop:boundedness-inherited': (
        r'State: boundedness is inherited by subsequences.',
        r'If $(a_n)$ is bounded then every subsequence $(a_{n_k})$ is bounded by the same bound.',
        '', '', ''),
    'prop:monotonicity-inherited': (
        r'State: monotonicity is inherited by subsequences.',
        r'If $(a_n)$ is monotone increasing (resp.\ decreasing) then so is every subsequence.',
        r'$n_j<n_k$ implies $a_{n_j}\le a_{n_k}$ by monotonicity of the original sequence.', '', ''),
    'thm:subseq-inherit-limits': (
        r'State: subsequences inherit limits.',
        r'If $a_n\to L$ then $a_{n_k}\to L$ for every subsequence $(a_{n_k})$.',
        '', '', ''),
    'thm:bw': (
        r'State Bolzano--Weierstrass.',
        r'Every bounded sequence in $\mathbb{R}$ has a convergent subsequence.',
        r'Extract a monotone subsequence (Monotone Subsequence Theorem); bounded monotone sequences converge (MCT).',
        '', 'cor:subseq-limits-exist, cor:seq-compact-closed-intervals'),
    'thm:monotone-subseq-theorem': (
        r'State the Monotone Subsequence Theorem.',
        r'Every sequence in $\mathbb{R}$ has a monotone subsequence.',
        r'Peak argument: if infinitely many peak terms, they form a decreasing subsequence; if finitely many, extract an increasing one from beyond the last peak.',
        '', 'thm:bw'),
    'thm:dense-subsequence-criterion': (
        r'State the Dense Subsequence Criterion.',
        r'If every neighbourhood of $L$ contains infinitely many terms of $(a_n)$, then $(a_n)$ has a subsequence converging to $L$.',
        r'From $B_{1/k}(L)$ containing infinitely many terms, pick $n_k>n_{k-1}$ with $|a_{n_k}-L|<1/k$.', '', ''),
    'cor:subseq-limits-exist': (
        r'State: every bounded sequence has a subsequential limit.',
        r'If $(a_n)$ is bounded then $\mathcal{L}(a_n)\neq\varnothing$.',
        r'By BW, extract a convergent subsequence; its limit is a subsequential limit.', '', ''),
    'cor:seq-compact-closed-intervals': (
        r'State sequential compactness of closed intervals.',
        r'Every sequence in $[a,b]$ has a convergent subsequence whose limit lies in $[a,b]$.',
        r'BW gives a convergent subsequence; the limit is in $[a,b]$ since $[a,b]$ is closed.', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-subsequence-toolkit.tex', {
    'thm:finite-partition-convergence': (
        r'State the Finite Partition Convergence Principle.',
        r'If $\mathbb{N}=E_1\cup\cdots\cup E_k$ (finite partition, each $E_i$ infinite) and $(a_n)_{n\in E_i}\to L$ for each $i$, then $a_n\to L$.',
        r'Take the maximum of the finitely many thresholds $N_i$.', '', ''),
    'cor:residue-class-convergence': (
        r'State Residue-Class Convergence.',
        r'If $(a_{kn+r})\to L$ for each $r\in\{0,\ldots,k-1\}$, then $(a_n)\to L$.',
        r'The residue classes form a finite partition of $\mathbb{N}$; apply Finite Partition Convergence.', '', ''),
    'cor:even-odd-convergence': (
        r'State Even-Odd Convergence.',
        r'If $(a_{2n})\to L$ and $(a_{2n+1})\to L$ then $(a_n)\to L$.',
        r'Special case of Residue-Class Convergence with $k=2$.', '', ''),
    'thm:conv-even-odd': (
        r'State Convergence via Even and Odd Subsequences.',
        r'$(a_n)\to L$ iff $(a_{2n})\to L$ and $(a_{2n+1})\to L$.',
        '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/tail-properties/notes-eventually.tex', {
    'thm:finite-modification-tail': (
        r'State: finite modification does not affect convergence.',
        r'If $(x_n)$ and $(y_n)$ differ in finitely many terms they converge to the same limit or both diverge.',
        '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/tail-properties/notes-subsequence-extraction.tex', {
    'thm:index-growth': (
        r'State the Index Growth lemma.',
        r'If $(n_k)$ is strictly increasing in $\mathbb{N}$ then $n_k\ge k$ for all $k$.',
        r'Induction: $n_1\ge 1$; if $n_k\ge k$ then $n_{k+1}\ge n_k+1\ge k+1$.', '', ''),
})

inject('volume-iii/analysis/sequences/notes/bounds/notes-completeness.tex', {
    'thm:cauchy-criterion': (
        r'State the Cauchy Criterion.',
        r'In $\mathbb{R}$: $(a_n)$ converges iff $(a_n)$ is Cauchy.',
        r'Convergent $\Rightarrow$ Cauchy always. Cauchy $\Rightarrow$ bounded $\Rightarrow$ BW gives convergent subsequence $\Rightarrow$ Cauchy with convergent subsequence converges.',
        '', ''),
    'thm:convergent-implies-cauchy': (
        r'State: convergent implies Cauchy.',
        r'If $(a_n)$ converges in $\mathbb{R}$ then $(a_n)$ is Cauchy.',
        '', '', ''),
    'thm:cauchy-bounded': (
        r'State: Cauchy sequences are bounded.',
        r'Every Cauchy sequence in $\mathbb{R}$ is bounded.',
        '', '', ''),
    'thm:bolzano-weierstrass': (
        r'State Bolzano--Weierstrass.',
        r'Every bounded sequence in $\mathbb{R}$ has a convergent subsequence.',
        '', '', ''),
    'cor:closed-interval-sequential-compact': (
        r'State: closed intervals are sequentially compact.',
        r'Every sequence in $[a,b]$ has a subsequence converging to a point in $[a,b]$.',
        '', '', ''),
    'thm:cauchy-subseq-limit': (
        r'State: Cauchy with convergent subsequence converges.',
        r'If $(a_n)$ is Cauchy and $(a_{n_k})\to L$ then $a_n\to L$.',
        '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-cauchy.tex', {
    'def:cauchy-sequence': (
        r'Define a Cauchy sequence.',
        r'$(a_n)$ is Cauchy if $\forall\varepsilon>0\,\exists N\,\forall m,n\ge N:|a_m-a_n|<\varepsilon$.',
        r'Terms eventually become arbitrarily close to each other.',
        r'In $\mathbb{Q}$: decimal approximations of $\sqrt{2}$ are Cauchy but have no limit in $\mathbb{Q}$.',
        'thm:convergent-implies-cauchy'),
    'thm:algebra-cauchy': (
        r'State the Algebra of Cauchy Sequences.',
        r'If $(a_n),(b_n)$ are Cauchy and $c\in\mathbb{R}$: $(a_n+b_n)$, $(a_n-b_n)$, $(ca_n)$, $(a_nb_n)$ are all Cauchy.',
        '', '', ''),
    'thm:linear-comb-cauchy': (
        r'State: linear combinations of Cauchy sequences are Cauchy.',
        r'If $(a_n),(b_n)$ are Cauchy and $\alpha,\beta\in\mathbb{R}$ then $(\alpha a_n+\beta b_n)$ is Cauchy.',
        '', '', ''),
    'thm:cauchy-divergence': (
        r'State the Cauchy Divergence Criterion.',
        r'$(a_n)$ diverges if $\exists\varepsilon_0>0$ such that $\forall N\,\exists m,n\ge N:|a_m-a_n|\ge\varepsilon_0$.',
        r'Negation of the Cauchy condition; useful to prove divergence.', '', ''),
    'def:rapidly-cauchy': (
        r'Define a rapidly Cauchy sequence.',
        r'$(f_k)$ is rapidly Cauchy if $\sum_{k=1}^\infty\|f_{k+1}-f_k\|<\infty$ (the series of successive differences converges).',
        '', '', 'thm:rapidly-cauchy'),
    'cor:convergent-series-cauchy-partial-sums': (
        r'State: convergent series have Cauchy partial sums.',
        r'If $\sum a_n$ converges and $s_n=\sum_{k=1}^n a_k$, then $(s_n)$ is Cauchy.',
        '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/bounds/notes-bounds-extremal-values.tex', {
    'prop:monotone-approx-bounds': (
        r'State Monotone Approximation of Bounds (sequences version).',
        r'For nonempty bounded $S\subseteq\mathbb{R}$: there exist monotone sequences $(x_n),(y_n)\subseteq S$ with $x_n\to\sup S$ and $y_n\to\inf S$.',
        '', '', ''),
    'def:LUB': (
        r'Define the LUB property (sequences version).',
        r'$\mathbb{R}$ has the LUB property: every nonempty bounded-above subset has a supremum in $\mathbb{R}$.',
        '', '', ''),
    'thm:LUB-iff-MCT': (
        r'State LUB $\Leftrightarrow$ MCT.',
        r'TFAE: (i) LUB property; (ii) bounded monotone sequences converge.',
        '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/applications/notes-theorems1.tex', {
    'thm:conv-even-odd': (
        r'State convergence via even/odd.',
        r'$(a_n)\to L$ iff $(a_{2n})\to L$ and $(a_{2n+1})\to L$.',
        '', '', ''),
    'cor:residue-class-convergence': (
        r'State Residue-Class Convergence.',
        r'If $(a_{kn+r})\to L$ for all $r\in\{0,\ldots,k-1\}$ then $(a_n)\to L$.', '', '', ''),
    'cor:even-odd-convergence': (
        r'State Even-Odd Convergence.',
        r'If $(a_{2n})\to L$ and $(a_{2n+1})\to L$ then $(a_n)\to L$.', '', '', ''),
    'thm:finite-partition-convergence': (
        r'State Finite Partition Convergence.',
        r'Finite partition into infinite sets, each converging to $L$, implies $(a_n)\to L$.', '', '', ''),
})

print('\nAll done.')

# ── SECOND PASS: corrected file paths ────────────────────────────────────────

inject('volume-iii/analysis/metric-spaces/notes/metrics/notes-properties-of-metrics.tex', {
    'def:metric': (
        r'State the four axioms defining a metric.',
        r'(M1) $d(x,y)\ge 0$; (M2) $d(x,y)=0\iff x=y$; (M3) $d(x,y)=d(y,x)$; (M4) $d(x,z)\le d(x,y)+d(y,z)$.',
        r'Non-negativity, identity of indiscernibles, symmetry, triangle inequality.',
        r'$d(x,y)=|x-y|^2$ fails M4: $(0-2)^2=4 > (0-1)^2+(1-2)^2=2$.', ''),
    'def:equivalent-metrics': (
        r'Define equivalent metrics.',
        r'$d_1\sim d_2$ if they induce the same topology: every $d_1$-open set is $d_2$-open and vice versa.',
        r'Equivalent iff they have the same convergent sequences.', '', ''),
    'def:ultrametric': (
        r'Define an ultrametric.',
        r'$d$ is an ultrametric if $d(x,z)\le\max\{d(x,y),d(y,z)\}$ (strong triangle inequality).',
        r'In ultrametric spaces every triangle is isosceles with the two equal sides longest.', '', ''),
    'def:antimetric': (
        r'Define an antimetric.',
        r'Like a metric but with reversed triangle inequality $d(x,z)\ge d(x,y)+d(y,z)$. Any such function is identically zero.',
        '', '', ''),
    'prop:distance-to-itself': (
        r'State: $d(x,x)=0$.',
        r'$d(x,x)=0$ for all $x$ (from M2: $d(x,x)=0\iff x=x$).',
        '', '', ''),
    'prop:positivity': (
        r'State: $x\neq y\Rightarrow d(x,y)>0$.',
        r'If $x\neq y$ then $d(x,y)>0$ (by M1 and M2).',
        '', '', ''),
    'prop:generalised-triangle': (
        r'State the Generalised Triangle Inequality.',
        r'$d(x_0,x_n)\le\sum_{k=0}^{n-1}d(x_k,x_{k+1})$ for any finite chain in $X$.',
        r'Induction on $n$ using M4.', '', ''),
    'prop:continuity-of-distance': (
        r'State continuity of the distance function.',
        r'For fixed $z$, $f(x):=d(x,z)$ is 1-Lipschitz: $|d(x,z)-d(y,z)|\le d(x,y)$.',
        '', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/balls/notes-working-with-balls.tex', {
    'prop:triangle-three-forms': (
        r'State the three forms of the triangle inequality.',
        r'Each side $\le$ sum of the other two: $d(x,z)\le d(x,y)+d(y,z)$; $d(x,y)\le d(x,z)+d(z,y)$; $d(y,z)\le d(y,x)+d(x,z)$.',
        r'All three follow by relabelling M4.', '', ''),
    'prop:reverse-triangle': (
        r'State the Reverse Triangle Inequality.',
        r'$|d(x,z)-d(y,z)|\le d(x,y)$.',
        r'Apply M4 twice and take absolute value.', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/convergence/notes-analysis-spine.tex', {
    'def:closed-sequential': (
        r'State the sequential definition of closed.',
        r'$F$ is closed if every sequence in $F$ converging in $X$ converges to a point of $F$.',
        '', '', 'thm:closed-equiv'),
    'def:open-sequential': (
        r'State the sequential definition of open.',
        r'$U$ is open if every sequence converging to a point of $U$ is eventually in $U$.',
        '', '', ''),
    'thm:closed-equiv': (
        r'State the equivalent characterisations of closed sets.',
        r'TFAE: (i) $X\setminus F$ is open; (ii) $F\supseteq F\'$; (iii) $F$ is sequentially closed.',
        '', '', ''),
    'prop:sequential-duality': (
        r'State sequential duality of open and closed.',
        r'$F=X\setminus U$ is closed iff sequences in $F$ converge in $F$; equivalently $U$ captures limits of sequences converging into it.',
        '', '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/limit-points/notes-limit-points.tex', {
    'def:derived-set': (
        r"Define the derived set $E'$.",
        r"$E':=\{x\in X:x\text{ is a limit point of }E\}$.",
        r"$E'$ is always closed; $E$ closed iff $E'\subseteq E$; $\overline{E}=E\cup E'$.",
        '', 'thm:derived-closed, thm:closed-derived, thm:closure-formula'),
    'def:perfect-set': (
        r'Define a perfect set.',
        r"$E$ is perfect if $E$ is closed and $E=E'$.",
        r'Perfect sets are uncountable if nonempty (e.g.\ the Cantor set).', '', ''),
    'thm:derived-closed': (
        r"State: $E'$ is closed.",
        r"For any $E\subseteq X$, the derived set $E'$ is a closed set.",
        r"Take a limit point of $E'$; diagonal sequence argument shows it is in $E'$.", '', ''),
    'thm:closed-derived': (
        r"State: $E$ is closed iff $E'\subseteq E$.",
        r"$E$ closed $\iff E'\subseteq E$.",
        r"Closed $\Rightarrow$ sequentially closed $\Rightarrow$ limit points in $E$.", '', ''),
    'thm:closure-formula': (
        r"State the Closure Formula.",
        r"$\overline{E}=E\cup E'$.",
        r"Adherent points are either in $E$ or are limits of sequences in $E$ (i.e.\ limit points).", '', ''),
})

inject('volume-iii/analysis/metric-spaces/notes/metric-spaces/notes-multiple-metrics.tex', {
    'def:r2-three-metrics': (
        r'State the three standard metrics on $\mathbb{R}^2$.',
        r'$d_1(p,q)=|x_1-y_1|+|x_2-y_2|$; $d_2(p,q)=\sqrt{(x_1-y_1)^2+(x_2-y_2)^2}$; $d_\infty(p,q)=\max(|x_1-y_1|,|x_2-y_2|)$.',
        r'Unit balls: diamond / disk / square.', '', ''),
    'def:product-metric': (
        r'Define the product metric space.',
        r'$(X\times Y,d)$ with $d$ being any of $d_1=d_X+d_Y$, $d_2=\sqrt{d_X^2+d_Y^2}$, $d_\infty=\max(d_X,d_Y)$; all equivalent.',
        '', '', ''),
    'def:product-coord-metrics': (
        r'Define coordinate projections in a product metric space.',
        r'The coordinate projections $\pi_X:X\times Y\to X$ and $\pi_Y:X\times Y\to Y$ are continuous; $(x_n,y_n)\to(x,y)$ iff $x_n\to x$ and $y_n\to y$.',
        '', '', ''),
})

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/03-predicate-walking.tex', {
    'def:peak': (
        r'Define a peak term.',
        r'$a_m$ is a peak term of $(a_n)$ if $a_m\ge a_n$ for all $n\ge m$.',
        r'A peak is a term that dominates everything following it.', '', ''),
    'thm:monotone-subsequence': (
        r'State the Monotone Subsequence Theorem.',
        r'Every sequence in $\mathbb{R}$ has a monotone subsequence.',
        r'If infinitely many peaks: they form a decreasing subsequence. If finitely many: beyond the last peak, each term is exceeded by a later term, giving an increasing subsequence.',
        '', ''),
    'prop:bw-bisection': (
        r'State Bolzano--Weierstrass.',
        r'Every bounded sequence in $\mathbb{R}$ has a convergent subsequence.',
        r'Extract a monotone subsequence; bounded monotone sequences converge by MCT.', '', ''),
    'def:inf-often-eventually': (
        r'Define "infinitely often" and "eventually".',
        r'"$P(n)$ holds infinitely often": $\{n:P(n)\}$ is infinite. "$P(n)$ holds eventually": $\exists N\,\forall n\ge N:P(n)$.',
        '', '', ''),
})

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/04-bisection-nested-intervals.tex', {
    'def:bisection-sequence': (
        r'Define a bisection sequence.',
        r'Nested closed intervals $[a_n,b_n]$ where each is a half of the previous; $b_n-a_n=(b_1-a_1)/2^{n-1}\to 0$.',
        r'By NIP the intersection is a single point.', '', ''),
    'prop:r-uncountable': (
        r'State: $\mathbb{R}$ is uncountable.',
        r'$\mathbb{R}$ is uncountable: any proposed enumeration can be contradicted by a nested-interval construction.',
        '', '', ''),
})

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/05-residue-partition.tex', {
    'thm:k-periodicity': (
        r'State the $k$-Periodicity / Finite Partition Convergence Principle.',
        r'Finite partition $\mathbb{N}=E_1\cup\cdots\cup E_k$ (each $E_i$ infinite), each $(a_n)_{n\in E_i}\to L$ $\Rightarrow$ $(a_n)\to L$.',
        r'Take max of the $k$ thresholds.', '', ''),
    'thm:inductive-selection': (
        r'State the Inductive Selection Principle.',
        r'From any sequence with infinitely many terms in $A$, we can inductively extract a subsequence entirely in $A$.',
        '', '', ''),
    'thm:alternating-series-test': (
        r'State the Alternating Series Test.',
        r'If $(b_n)$ is decreasing with $b_n\to 0$, then $\sum(-1)^{n+1}b_n$ converges.',
        r'Even and odd partial sums are monotone and converge to the same limit by squeeze.', '', ''),
    'def:LUB': (
        r'Define the LUB property.',
        r'$\mathbb{R}$ has the LUB property: every nonempty subset bounded above has a supremum in $\mathbb{R}$.',
        '', '', ''),
    'thm:LUB-iff-MCT': (
        r'State LUB $\Leftrightarrow$ MCT.',
        r'TFAE: (i) LUB property; (ii) every bounded monotone sequence converges.',
        '', '', ''),
    'thm:nested-interval-property': (
        r'State the Nested Interval Property.',
        r'Nested nonempty closed intervals have nonempty intersection.',
        '', '', ''),
})

inject('volume-iii/analysis/real-analysis/notes/proof-techniques/02-completeness-construction.tex', {
    'thm:inductive-selection': (
        r'State the Inductive Selection Principle.',
        r'From a sequence with infinitely many terms in $A$, one can extract a subsequence entirely in $A$.',
        '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-algebra-of-sequences.tex', {
    'thm:uniqueness-limits': (
        r'State Uniqueness of Limits.',
        r'If $x_n\to L$ and $x_n\to M$ then $L=M$.',
        r'$|L-M|\le|L-x_n|+|x_n-M|\to 0$.', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-subsequence-toolkit.tex', {
    'prop:conv-inherited': (
        r'State: convergence is inherited by subsequences.',
        r'If $(a_n)\to L$ then every subsequence $(a_{n_k})\to L$.',
        r'For $\varepsilon>0$ pick $N$; since $n_k\ge k$, $k\ge N$ forces $n_k\ge N$.', '', ''),
    'prop:boundedness-inherited': (
        r'State: boundedness is inherited.',
        r'If $(a_n)$ is bounded then every subsequence is bounded by the same bound.', '', '', ''),
    'prop:monotonicity-inherited': (
        r'State: monotonicity is inherited.',
        r'If $(a_n)$ is monotone then every subsequence is monotone in the same direction.', '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-subsequences.tex', {
    'thm:index-growth': (
        r'State the Index Growth lemma.',
        r'If $(n_k)$ is strictly increasing in $\mathbb{N}$ then $n_k\ge k$ for all $k$.',
        r'Induction: $n_1\ge 1$; $n_{k+1}\ge n_k+1\ge k+1$.', '', ''),
    'thm:conv-even-odd': (
        r'State convergence via even and odd subsequences.',
        r'$(a_n)\to L$ iff $(a_{2n})\to L$ and $(a_{2n+1})\to L$.',
        '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/monotone-approximation.tex', {
    'thm:MCT': (
        r'State the Monotone Convergence Theorem.',
        r'Every bounded monotone sequence converges.',
        r'Increasing bounded: converges to $\sup\{x_n\}$. Decreasing bounded below: converges to $\inf\{x_n\}$.', '', 'thm:LUB-iff-MCT'),
    'def:LUB': (
        r'Define the LUB property.',
        r'$\mathbb{R}$ has the LUB property: every nonempty bounded-above set has a supremum.', '', '', 'thm:LUB-iff-MCT'),
    'thm:LUB-iff-MCT': (
        r'State LUB $\Leftrightarrow$ MCT.',
        r'TFAE: (i) LUB property; (ii) bounded monotone sequences converge.', '', '', ''),
    'prop:monotone-approx-bounds': (
        r'State Monotone Approximation of Bounds.',
        r'For nonempty bounded $S$: there exist monotone sequences $(x_n),(y_n)\subseteq S$ with $x_n\to\sup S$, $y_n\to\inf S$.', '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-cauchy.tex', {
    'def:cauchy-sequence': (
        r'Define a Cauchy sequence.',
        r'$(a_n)$ is Cauchy if $\forall\varepsilon>0\,\exists N\,\forall m,n\ge N:|a_m-a_n|<\varepsilon$.',
        r'Terms become arbitrarily close; no limit needed in the definition.',
        r'$\mathbb{Q}$ is not complete: $(1,1.4,1.41,\ldots)$ is Cauchy but $\to\sqrt{2}\notin\mathbb{Q}$.', ''),
    'def:rapidly-cauchy': (
        r'Define a rapidly Cauchy sequence.',
        r'$(f_k)$ is rapidly Cauchy if $\sum_{k=1}^\infty\|f_{k+1}-f_k\|<\infty$.',
        '', '', 'thm:rapidly-cauchy'),
    'thm:cauchy-criterion': (
        r'State the Cauchy Criterion.',
        r'In $\mathbb{R}$: $(a_n)$ converges $\iff$ $(a_n)$ is Cauchy.',
        r'Convergent $\Rightarrow$ Cauchy always. Cauchy $\Rightarrow$ bounded $\Rightarrow$ BW $\Rightarrow$ convergent subsequence $\Rightarrow$ Cauchy with convergent subsequence converges.',
        '', ''),
    'thm:bolzano-weierstrass': (
        r'State Bolzano--Weierstrass.',
        r'Every bounded sequence in $\mathbb{R}$ has a convergent subsequence.', '', '', ''),
    'thm:convergent-implies-cauchy': (
        r'State: convergent implies Cauchy.',
        r'If $(a_n)$ converges then $(a_n)$ is Cauchy.', '', '', ''),
    'thm:cauchy-bounded': (
        r'State: Cauchy sequences are bounded.',
        r'Every Cauchy sequence is bounded.', '', '', ''),
    'thm:cauchy-subseq-limit': (
        r'State: Cauchy with convergent subsequence converges.',
        r'If $(a_n)$ Cauchy and $(a_{n_k})\to L$ then $a_n\to L$.', '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/bounds/notes-completeness.tex', {
    'thm:cauchy-criterion': (
        r'State the Cauchy Criterion.',
        r'In $\mathbb{R}$: $(a_n)$ converges iff Cauchy.', '', '', ''),
    'thm:bolzano-weierstrass': (
        r'State Bolzano--Weierstrass.',
        r'Every bounded sequence has a convergent subsequence.', '', '', ''),
    'cor:closed-interval-sequential-compact': (
        r'State sequential compactness of closed intervals.',
        r'Every sequence in $[a,b]$ has a convergent subsequence with limit in $[a,b]$.', '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/bounds/notes-bounds-extremal-values.tex', {
    'prop:monotone-approx-bounds': (
        r'State Monotone Approximation of Bounds.',
        r'Nonempty bounded $S$: there exist monotone sequences in $S$ converging to $\sup S$ and $\inf S$.', '', '', ''),
    'def:LUB': (
        r'Define the LUB property.',
        r'$\mathbb{R}$ has the LUB property: every nonempty bounded-above subset has a supremum.', '', '', ''),
    'thm:LUB-iff-MCT': (
        r'State LUB $\Leftrightarrow$ MCT.',
        r'TFAE: (i) LUB property; (ii) bounded monotone sequences converge.', '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/notes-convergence.tex', {
    'thm:uniqueness-limits': (
        r'State Uniqueness of Limits.',
        r'If $x_n\to L$ and $x_n\to M$ then $L=M$.',
        r'$|L-M|\le|L-x_n|+|x_n-M|\to 0$; squeeze gives $L=M$.', '', ''),
})

inject('volume-iii/analysis/sequences/notes/applications/notes-theorems1.tex', {
    'thm:conv-even-odd': (
        r'State convergence via even/odd subsequences.',
        r'$(a_n)\to L$ iff $(a_{2n})\to L$ and $(a_{2n+1})\to L$.', '', '', ''),
    'cor:residue-class-convergence': (
        r'State Residue-Class Convergence.',
        r'If $(a_{kn+r})\to L$ for all $r\in\{0,\ldots,k-1\}$ then $(a_n)\to L$.', '', '', ''),
    'cor:even-odd-convergence': (
        r'State Even-Odd Convergence.',
        r'$(a_{2n})\to L$ and $(a_{2n+1})\to L$ $\Rightarrow$ $(a_n)\to L$.', '', '', ''),
    'thm:finite-partition-convergence': (
        r'State Finite Partition Convergence.',
        r'Finite partition into infinite sets all converging to $L$ $\Rightarrow$ $(a_n)\to L$.', '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/tail-properties/notes-eventually.tex', {
    'thm:finite-modification-tail': (
        r'State: finite modification does not affect convergence.',
        r'If $(x_n)$ and $(y_n)$ differ in finitely many terms, they converge to the same limit or both diverge.', '', '', ''),
})

inject('volume-iii/analysis/sequences/notes/sequences/tail-properties/notes-subsequence-extraction.tex', {
    'thm:index-growth': (
        r'State the Index Growth Lemma.',
        r'If $(n_k)$ is strictly increasing in $\mathbb{N}$ then $n_k\ge k$.', '', '', ''),
})

print('\nSecond pass done.')
