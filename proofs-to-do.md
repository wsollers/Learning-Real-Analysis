# Proofs To Do

Results are ordered from foundations upward — each entry depends only on results above it.

## Session Strategy

- **First pass per chapter:** complete all Easy proofs in that chapter before moving on.
- **After that:** one proof at a time, in dependency order.
- **Commands:**
  - `add standard proof for <theorem-name>` — normal mode proof, fully hooked up with nav links.
  - `add justified proof for <theorem-name>` — thorough mode (3-column tag/step/justification) with full commentary.
  - `convert proof standard <theorem-name>` — replace a justified proof file with its normal mode equivalent (use after the proof is learned).

## Status Key

| Symbol | Meaning |
|--------|---------|
| ✓ | Done — proof file exists, nav links in place |
| → | In progress |
| · | To do |

## Proof List

| # | Volume | Proof Name | File | Difficulty | Status |
|---|--------|-----------|------|------------|--------|
| 1 | I | Tao 2.1.4 | `axiom-systems/notes/peano/notes-peano-numerals` | Easy | ✓ |
| 2 | I | Tao 2.1.6 — 4 ≠ 0 | `axiom-systems/notes/peano/notes-peano-numerals` | Easy | ✓ |
| 3 | I | Tao 2.1.8 — 6 ≠ 2 | `axiom-systems/notes/peano/notes-peano-numerals` | Easy | ✓ |
| 4 | I | Uniqueness of the empty set | `axiom-systems/notes/zermelo/zermelo-numerals` | Easy | ✓ |
| 5 | I | Singleton is well-defined | `axiom-systems/notes/zermelo/zermelo-numerals` | Easy | ✓ |
| 6 | I | Basic properties of union | `axiom-systems/notes/von-neuman/von_neuman-numerals` | Easy | ✓ |
| 7 | I | First few Zermelo numerals | `axiom-systems/notes/zermelo/zermelo-numerals` | Easy | ✓ |
| 8 | I | First few von Neumann numerals | `axiom-systems/notes/von-neuman/von_neuman-numerals` | Easy | ✓ |
| 9 | I | Injectivity of Zermelo successor | `axiom-systems/notes/zermelo/zermelo-numerals` | Easy | ✓ |
| 10 | I | Distinctness of Zermelo numerals | `axiom-systems/notes/zermelo/zermelo-numerals` | Easy | ✓ |
| 11 | I | Membership pattern | `axiom-systems/notes/zermelo/zermelo-numerals` | Medium | · |
| 12 | I | Each von Neumann numeral is transitive | `axiom-systems/notes/von-neuman/von_neuman-numerals` | Easy | ✓ |
| 13 | I | Successor is injective | `axiom-systems/notes/von-neuman/von_neuman-numerals` | Easy | ✓ |
| 14 | I | No numeral contains itself | `axiom-systems/notes/zermelo/zermelo-numerals` | Easy | ✓ |
| 15 | I | Order by membership | `axiom-systems/notes/von-neuman/von_neuman-numerals` | Medium | · |
| 16 | I | Strict chain under membership | `axiom-systems/notes/zermelo/zermelo-numerals` | Easy | ✓ |
| 17 | I | Well-ordering by \in | `axiom-systems/notes/von-neuman/von_neuman-numerals` | Medium | · |
| 18 | I | Each numeral is the set of its predecessors | `axiom-systems/notes/von-neuman/von_neuman-numerals` | Easy | ✓ |
| 19 | I | Monotonicity | `axiom-systems/notes/von-neuman/von_neuman-numerals` | Easy | ✓ |
| 20 | I | Unique Readability | `propositional-logic/notes/notes-syntax` | Hard | · |
| 21 | I | Non-Associativity of NAND | `propositional-logic/notes/syntax/notes-connectives` | Easy | ✓ |
| 22 | I | Non-Associativity of NOR | `propositional-logic/notes/syntax/notes-connectives` | Easy | ✓ |
| 23 | I | XOR Equivalences | `propositional-logic/notes/notes-connectives` | Medium | · |
| 24 | I | Properties of XOR | `propositional-logic/notes/notes-connectives` | Medium | · |
| 25 | I | NAND Definition | `propositional-logic/notes/notes-connectives` | Medium | · |
| 26 | I | Expressing Connectives with NAND | `propositional-logic/notes/notes-connectives` | Medium | · |
| 27 | I | NOR Definition | `propositional-logic/notes/notes-connectives` | Medium | · |
| 28 | I | Expressing Connectives with NOR | `propositional-logic/notes/notes-connectives` | Medium | · |
| 29 | I | Double Negation | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 30 | I | De Morgan's Laws | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 31 | I | Commutativity | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 32 | I | Associativity | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 33 | I | Distributivity | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 34 | I | Idempotence | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 35 | I | Absorption | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 36 | I | Identity Laws | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 37 | I | Every Formula Has an NNF | `propositional-logic/notes/semantics/notes-normal-forms` | Easy | ✓ |
| 38 | I | Domination Laws | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 39 | I | Negation Laws | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 40 | I | Material Implication | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 41 | I | Contraposition | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 42 | I | Exportation / Importation | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 43 | I | Every Formula Has a CNF | `propositional-logic/notes/semantics/notes-normal-forms` | Easy | ✓ |
| 44 | I | Negation of Conditional | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 45 | I | Biconditional Expansion | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 46 | I | Biconditional as Disjunction | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 47 | I | Negation of Biconditional | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 48 | I | Every Formula Has a DNF | `propositional-logic/notes/semantics/notes-normal-forms` | Easy | ✓ |
| 49 | I | Duality Principle | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 50 | I | Semantic Duality | `propositional-logic/notes/notes-equivalences` | Medium | · |
| 51 | I | NNF Conversion | `propositional-logic/notes/notes-normal-forms` | Medium | · |
| 52 | I | CNF Existence | `propositional-logic/notes/notes-normal-forms` | Medium | · |
| 53 | I | DNF Existence | `propositional-logic/notes/notes-normal-forms` | Medium | · |
| 54 | I | Soundness and Completeness of Propositional Logic | `propositional-logic/notes/notes-proof-systems` | Hard | · |
| 55 | I | Soundness of Resolution | `propositional-logic/notes/notes-resolution` | Medium | · |
| 56 | I | Completeness of Resolution | `propositional-logic/notes/notes-resolution` | Hard | · |
| 57 | I | Standard Functionally Complete Sets | `propositional-logic/notes/notes-functional-completeness` | Medium | · |
| 58 | I | NAND is Functionally Complete | `propositional-logic/notes/notes-functional-completeness` | Medium | · |
| 59 | I | NOR is Functionally Complete | `propositional-logic/notes/notes-functional-completeness` | Medium | · |
| 60 | I | Minimal Adequate Sets | `propositional-logic/notes/notes-functional-completeness` | Medium | · |
| 61 | I | Non-Adequate Sets | `propositional-logic/notes/notes-functional-completeness` | Medium | · |
| 62 | I | Post's Theorem | `propositional-logic/notes/notes-functional-completeness` | Hard | · |
| 63 | I | Compactness Theorem for Propositional Logic | `propositional-logic/notes/notes-compactness` | Hard | · |
| 64 | I | Compactness for Logical Consequence | `propositional-logic/notes/notes-compactness` | Easy | ✓ |
| 65 | I | Compactness of Logical Consequence | `propositional-logic/notes/metatheory/notes-compactness` | Easy | ✓ |
| 66 | I | Craig's Interpolation Theorem | `propositional-logic/notes/notes-interpolation` | Hard | · |
| 67 | I | Substitution Lemma for Terms | `predicate-logic/notes/semantics/notes-predicates-lemmas` | Medium | · |
| 68 | I | Substitution Lemma for Formulas | `predicate-logic/notes/semantics/notes-predicates-lemmas` | Medium | · |
| 69 | I | Quantifier Negation Laws | `predicate-logic/notes/quantifiers/notes-quantifier-laws` | Easy | ✓ |
| 70 | I | Quantifier Commutation | `predicate-logic/notes/quantifiers/notes-quantifier-laws` | Medium | · |
| 71 | I | Quantifier Distribution | `predicate-logic/notes/quantifiers/notes-quantifier-laws` | Medium | · |
| 72 | I | Vacuous Quantification | `predicate-logic/notes/quantifiers/notes-quantifier-laws` | Easy | ✓ |
| 73 | I | Negation of Bounded Quantifiers | `predicate-logic/notes/quantifiers/notes-basic-quantifiers` | Medium | · |
| 74 | I | Renaming Bound Variables | `predicate-logic/notes/quantifiers/notes-quantifier-laws` | Easy | ✓ |
| 75 | I | Soundness of First-Order Logic | `predicate-logic/notes/proof-theory/notes-soundness-completeness` | Medium | · |
| 76 | I | Symmetry of Equality | `predicate-logic/notes/proof-theory/notes-equality` | Medium | · |
| 77 | I | Transitivity of Equality | `predicate-logic/notes/proof-theory/notes-equality` | Medium | · |
| 78 | I | Term Substitution under Equality | `predicate-logic/notes/proof-theory/notes-equality` | Medium | · |
| 79 | I | Predicate Substitution under Equality | `predicate-logic/notes/proof-theory/notes-equality` | Medium | · |
| 80 | I | Commutativity of Union and Intersection | `sets-relations-functions/notes/sets/notes-set-algebra` | Medium | · |
| 81 | I | Associativity of Union and Intersection | `sets-relations-functions/notes/sets/notes-set-algebra` | Medium | · |
| 82 | I | Distributive Laws | `sets-relations-functions/notes/sets/notes-set-algebra` | Medium | · |
| 83 | I | Identity and Absorption Laws | `sets-relations-functions/notes/sets/notes-set-algebra` | Medium | · |
| 84 | I | Involution of Complement | `sets-relations-functions/notes/sets/notes-set-algebra` | Medium | · |
| 85 | I | Principle of Set Duality | `sets-relations-functions/notes/sets/notes-set-operations` | Easy | ✓ |
| 86 | I | Uniqueness of Ordered Pairs | `sets-relations-functions/notes/relations/notes-relations` | Medium | · |
| 87 | I | Associativity of Composition | `sets-relations-functions/notes/functions/notes-composition` | Medium | · |
| 88 | I | Identity and Composition | `sets-relations-functions/notes/functions/notes-composition` | Medium | · |
| 89 | I | Injectivity and Surjectivity Under Composition | `sets-relations-functions/notes/functions/notes-composition` | Medium | · |
| 90 | I | Characterization of Inverse Functions | `sets-relations-functions/notes/functions/notes-composition` | Medium | · |
| 91 | I | Inverse of a Composition | `sets-relations-functions/notes/functions/notes-composition` | Medium | · |
| 92 | I | One-Sided Inverses and Function Properties | `sets-relations-functions/notes/functions/notes-composition` | Medium | · |
| 93 | I | Preimages Preserve Set Operations | `sets-relations-functions/notes/functions/notes-composition` | Medium | · |
| 94 | I | Images and Set Operations | `sets-relations-functions/notes/functions/notes-composition` | Medium | · |
| 95 | I | Representative Independence Lemma | `sets-relations-functions/notes/equivalence/notes-equivalence` | Medium | · |
| 96 | I | Equivalence Relations and Partitions | `sets-relations-functions/notes/equivalence/notes-equivalence` | Medium | · |
| 97 | I | ≤q_f is always a preorder | `sets-relations-functions/notes/order/notes-order-induced` | Easy | ✓ |
| 98 | I | ≤q_f is a partial order iff f is injective | `sets-relations-functions/notes/order/notes-order-induced` | Easy | ✓ |
| 99 | I | Complete preorder extension | `sets-relations-functions/notes/order/notes-order-extensions` | Easy | ✓ |
| 100 | I | Order embeddings are injective | `sets-relations-functions/notes/order/notes-order-induced` | Medium | · |
| 101 | I | Order embedding is isomorphism onto image | `sets-relations-functions/notes/order/notes-order-induced` | Medium | · |
| 102 | I | Equivalence of induction and well-ordering | `proof-techniques/notes/induction/notes-ind-wellordering` | Medium | · |
| 103 | II | Tao 2.2.2 | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Medium | · |
| 104 | II | Tao 2.2.3 | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Medium | · |
| 105 | II | Tao 2.2.12 — Basic properties of order | `natural-numbers/notes/arithmetic/addition/notes-add-order` | Medium | · |
| 106 | II | Successor as Addition of One | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Easy | · |
| 107 | II | Tao 2.2.4 — Commutativity | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Medium | · |
| 108 | II | Tao 2.2.5 — Associativity | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Medium | · |
| 109 | II | Tao 2.2.6 — Cancellation | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Medium | · |
| 110 | II | Tao 2.2.13 — Trichotomy | `natural-numbers/notes/arithmetic/addition/notes-add-order` | Medium | · |
| 111 | II | Tao 2.2.8 | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Medium | · |
| 112 | II | Tao 2.2.9 | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Easy | · |
| 113 | II | Tao 2.2.10 — Predecessor | `natural-numbers/notes/arithmetic/addition/notes-add-toolkit` | Medium | · |
| 114 | II | Tao 2.3.2 — Commutativity | `natural-numbers/notes/arithmetic/multiplication/notes-mult-toolkit` | Medium | · |
| 115 | II | Tao 2.3.3 — No zero divisors | `natural-numbers/notes/arithmetic/multiplication/notes-mult-toolkit` | Medium | · |
| 116 | II | Tao 2.3.4 — Distributive law | `natural-numbers/notes/arithmetic/multiplication/notes-mult-toolkit` | Medium | · |
| 117 | II | Tao 2.3.5 — Associativity | `natural-numbers/notes/arithmetic/multiplication/notes-mult-toolkit` | Medium | · |
| 118 | II | Tao 2.3.6 — Multiplication preserves order | `natural-numbers/notes/arithmetic/multiplication/notes-mult-toolkit` | Medium | · |
| 119 | II | Tao 2.3.7 — Cancellation for multiplication | `natural-numbers/notes/arithmetic/multiplication/notes-mult-toolkit` | Easy | · |
| 120 | II | Tao 2.3.9 — Euclidean algorithm | `natural-numbers/notes/arithmetic/multiplication/notes-mult-toolkit` | Medium | · |
| 121 | II | Tao 4.1.3 — Addition and multiplication are well-defined | `integers/notes/notes-int-tao-construction` | Medium | · |
| 122 | II | Tao 4.1.5 — Trichotomy of integers | `integers/notes/notes-int-tao-construction` | Medium | · |
| 123 | II | Tao 4.1.8 — No zero divisors | `integers/notes/notes-int-tao-construction` | Medium | · |
| 124 | II | Tao 4.1.9 — Cancellation law for ℤ | `integers/notes/notes-int-tao-construction` | Medium | · |
| 125 | II | Tao 4.1.11 — Properties of order on ℤ | `integers/notes/notes-int-tao-construction` | Medium | · |
| 126 | II | Mendelson 1.1 — \sim is an equivalence relation | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 127 | II | Mendelson 2.1 — Addition well-defined | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 128 | II | Mendelson 2.2 — Properties of addition | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 129 | II | Mendelson 2.3 — Multiplication well-defined | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 130 | II | Mendelson 2.4 — Properties of multiplication | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 131 | II | Mendelson 3.3 — Cancellation \Leftrightarrow no zero divisors | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 132 | II | Mendelson 3.4 — Trivial ring | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 133 | II | Mendelson 4.1 — Consequences of order axioms | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 134 | II | Mendelson 4.2 — Order and positivity | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 135 | II | Mendelson 4.3 — Positivity set construction | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 136 | II | Mendelson 4.4 — Positivity is class-invariant | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 137 | II | Mendelson 4.5 | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 138 | II | Mendelson 4.6 | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 139 | II | Mendelson 4.7 — Recovery of Peano system | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 140 | II | Mendelson 4.8 — Properties of absolute value | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 141 | II | Mendelson 5.1 — Euclidean division | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 142 | II | Mendelson 5.2--5.9 | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 143 | II | Mendelson 9.8 — Uniqueness of ℤ | `integers/notes/notes-int-mendelson-construction` | Medium | · |
| 144 | II | Uniqueness of Supremum and Infimum | `reals/notes/foundations/notes-bounds` | Medium | · |
| 145 | II | ε-Characterization of Supremum and Infimum | `reals/notes/foundations/notes-bounds` | Medium | · |
| 146 | II | ε-Approximation | `reals/notes/foundations/notes-bounds` | Easy | · |
| 147 | II | Characterization of Intervals | `reals/notes/foundations/notes-intervals` | Medium | · |
| 148 | II | Nested Interval Property | `reals/notes/foundations/notes-completeness` | Hard | · |
| 149 | II | Archimedean Property | `reals/notes/foundations/notes-completeness` | Hard | · |
| 150 | II | Archimedean Property — Corollary | `reals/notes/foundations/notes-completeness` | Hard | · |
| 151 | II | Floor lemma | `reals/notes/foundations/notes-completeness` | Medium | · |
| 152 | II | Density of ℚ | `reals/notes/foundations/notes-completeness` | Hard | · |
| 153 | II | Irrationals Are Dense | `reals/notes/foundations/notes-completeness` | Easy | · |
| 154 | II | Density of Irrationals | `reals/notes/foundations/notes-completeness` | Easy | · |
| 155 | II | Existence of Square Roots | `reals/notes/foundations/notes-completeness` | Hard | · |
| 156 | III | Divisibility of Linear Combinations | `algebra/abstract-algebra/notes/notes-integers` | Easy | · |
| 157 | III | Division Algorithm | `algebra/abstract-algebra/notes/notes-integers` | Medium | · |
| 158 | III | Bézout's Identity | `algebra/abstract-algebra/notes/notes-integers` | Medium | · |
| 159 | III | Bezout's Identity — Coprime Case | `algebra/abstract-algebra/notes/notes-integers` | Medium | · |
| 160 | III | Equivalent Characterizations of Relatively Prime Integers | `algebra/abstract-algebra/notes/notes-integers` | Medium | · |
| 161 | III | Euclid's Lemma | `algebra/abstract-algebra/notes/notes-integers` | Medium | · |
| 162 | III | Fundamental Theorem of Arithmetic | `algebra/abstract-algebra/notes/notes-integers` | Hard | · |
| 163 | III | Structural Relation Between gcd and lcm | `algebra/abstract-algebra/notes/notes-integers` | Medium | · |
| 164 | III | Second Principle of Mathematical Induction (Strong Induction) | `algebra/abstract-algebra/notes/notes-induction` | Medium | · |
| 165 | III | Idempotence of mod | `algebra/abstract-algebra/notes/notes-modular-arithmetic` | Easy | · |
| 166 | III | Congruence is an Equivalence Relation | `algebra/abstract-algebra/notes/notes-modular-arithmetic` | Medium | · |
| 167 | III | Compatibility with Arithmetic | `algebra/abstract-algebra/notes/notes-modular-arithmetic` | Medium | · |
| 168 | III | Equivalence Relations and Partitions | `algebra/abstract-algebra/notes/notes-relations-and-functions` | Medium | · |
| 169 | III | Basic Properties of Function Composition | `algebra/abstract-algebra/notes/notes-relations-and-functions` | Medium | · |
| 170 | III | GCD as Smallest Positive Linear Combination | `algebra/abstract-algebra/notes/proof-gcd-linear-combination` | Medium | · |
| 171 | III | Uniqueness of the Identity | `algebra/algebraic-structures/notes/groups/notes-groups-theorems` | Easy | · |
| 172 | III | Uniqueness of Inverses | `algebra/algebraic-structures/notes/groups/notes-groups-theorems` | Easy | · |
| 173 | III | Cancellation Laws | `algebra/algebraic-structures/notes/groups/notes-groups-theorems` | Easy | · |
| 174 | III | Socks-Shoes Property | `algebra/algebraic-structures/notes/groups/notes-groups-theorems` | Easy | · |
| 175 | III | Multiplication by Zero | `algebra/algebraic-structures/notes/rings/notes-rings-theorems` | Easy | · |
| 176 | III | Multiplication by Additive Inverse | `algebra/algebraic-structures/notes/rings/notes-rings-theorems` | Medium | · |
| 177 | III | Cancellation in Integral Domains | `algebra/algebraic-structures/notes/rings/notes-rings-integral-domains` | Medium | · |
| 178 | III | Every Field is an Integral Domain | `algebra/algebraic-structures/notes/fields/notes-fields-theorems` | Easy | · |
| 179 | III | Zero Product Property in a Field | `algebra/algebraic-structures/notes/fields/notes-fields-theorems` | Easy | · |
| 180 | III | Nonzero Scalars Have Inverses | `algebra/algebraic-structures/notes/fields/notes-fields-theorems` | Easy | · |
| 181 | III | Characteristic of a Field | `algebra/algebraic-structures/notes/fields/notes-fields-theorems` | Easy | · |
| 182 | III | Identification of subsets with functions | `algebra/set-algebras/notes/notes-power-set-and-characteristic-functions` | Easy | · |
| 183 | III | Boolean ring structure on 2^X | `algebra/set-algebras/notes/notes-power-set-and-characteristic-functions` | Easy | · |
| 184 | III | F_2-vector space structure on 2^X | `algebra/set-algebras/notes/notes-power-set-and-characteristic-functions` | Easy | · |
| 185 | III | Algebra = Ring + X | `algebra/set-algebras/notes/notes-set-algebras` | Easy | · |
| 186 | III | Closure consequences of a \sigma-algebra | `algebra/set-algebras/notes/notes-set-algebras` | Easy | · |
| 187 | III | Uniqueness of the Additive Identity | `algebra/linear-algebra/notes/vector-spaces/06-basic-propositions` | Easy | · |
| 188 | III | Uniqueness of the Additive Inverse | `algebra/linear-algebra/notes/vector-spaces/06-basic-propositions` | Easy | · |
| 189 | III | Scalar Multiplication by Zero and Negation | `algebra/linear-algebra/notes/vector-spaces/06-basic-propositions` | Easy | · |
| 190 | III | Hilbert Basis Theorem | `algebra/algebraic-geometry/notes/notes-polynomial-rings` | Hard | · |
| 191 | III | Properties of the modulus | `analysis/metric-spaces/notes/notes-metric-space` | Medium | · |
| 192 | III | Neighborhood = ball in ℝ | `analysis/point-set-topology/notes/notes-topology` | Medium | · |
| 193 | III | Ball characterization of closure | `analysis/point-set-topology/notes/notes-topology` | Medium | · |
| 194 | III | Ball characterization of interior | `analysis/point-set-topology/notes/notes-topology` | Medium | · |
| 195 | III | Heine--Borel Theorem on ℝ^n | `analysis/point-set-topology/notes/notes-topology` | Hard | · |
| 196 | III | A Sequence Is Determined by Its Values | `analysis/real-analysis/notes/sequences/notes-sequences` | Hard | · |
| 197 | III | Every Subsequence Is a Sequence | `analysis/real-analysis/notes/sequences/notes-sequences` | Hard | · |
| 198 | III | Every convergent sequence is bounded | `analysis/real-analysis/notes/sequences/notes-convergence` | Hard | · |
| 199 | III | Equivalence of convergence definitions in ℝ | `analysis/real-analysis/notes/sequences/notes-convergence` | Hard | · |
| 200 | III | Bounded Does Not Imply Convergent | `analysis/real-analysis/notes/sequences/notes-convergence` | Medium | · |
| 201 | III | Monotone Approximation of Bounds | `analysis/real-analysis/notes/sequences/monotone-approximation` | Medium | · |
| 202 | III | Monotone Convergence Theorem (MCT) | `analysis/real-analysis/notes/sequences/monotone-approximation` | Hard | · |
| 203 | III | Equivalence: LUB \Longleftrightarrow MCT | `analysis/real-analysis/notes/sequences/monotone-approximation` | Hard | · |
| 204 | III | Monotone Convergence Theorem | `analysis/real-analysis/notes/sequences/notes-monotone-sequences` | Hard | · |
| 205 | III | Uniqueness of Limits | `analysis/real-analysis/notes/sequences/notes-algebra-of-sequences` | Hard | · |
| 206 | III | Order Limit Theorem | `analysis/real-analysis/notes/sequences/notes-algebra-of-sequences` | Medium | · |
| 207 | III | Squeeze Theorem | `analysis/real-analysis/notes/sequences/notes-algebra-of-sequences` | Medium | · |
| 208 | III | Algebra of limits | `analysis/real-analysis/notes/sequences/notes-algebra-of-sequences` | Medium | · |
| 209 | III | Limit Respects Upper Bounds | `analysis/real-analysis/notes/sequences/notes-algebra-of-sequences` | Medium | · |
| 210 | III | Convergent Times Bounded Is Bounded | `analysis/real-analysis/notes/sequences/notes-algebra-of-sequences` | Medium | · |
| 211 | III | Convergence of Absolute Values | `analysis/real-analysis/notes/sequences/notes-algebra-of-sequences` | Medium | · |
| 212 | III | Convergence is a Tail Property | `analysis/real-analysis/notes/sequences/notes-k-tails` | Medium | · |
| 213 | III | Finite Modification Theorem | `analysis/real-analysis/notes/sequences/notes-k-tails` | Medium | · |
| 214 | III | Cauchy is a Tail Property | `analysis/real-analysis/notes/sequences/notes-k-tails` | Medium | · |
| 215 | III | Tail Suprema are Monotone | `analysis/real-analysis/notes/sequences/notes-k-tails` | Medium | · |
| 216 | III | Limit Superior via Tails | `analysis/real-analysis/notes/sequences/notes-k-tails` | Medium | · |
| 217 | III | Subsequences are Iterated Tails | `analysis/real-analysis/notes/sequences/notes-k-tails` | Medium | · |
| 218 | III | Finite Alteration Does Not Affect Limit | `analysis/real-analysis/notes/sequences/notes-k-tails` | Medium | · |
| 219 | III | Tails of a Convergent Sequence Converge | `analysis/real-analysis/notes/sequences/notes-k-tails` | Medium | · |
| 220 | III | Index Growth | `analysis/real-analysis/notes/sequences/notes-subsequences` | Medium | · |
| 221 | III | Subsequences Inherit Limits | `analysis/real-analysis/notes/sequences/notes-subsequences` | Medium | · |
| 222 | III | Convergence via Even and Odd Subsequences | `analysis/real-analysis/notes/sequences/notes-subsequences` | Medium | · |
| 223 | III | Bolzano--Weierstrass | `analysis/real-analysis/notes/sequences/notes-subsequences` | Hard | · |
| 224 | III | Existence of Subsequential Limits | `analysis/real-analysis/notes/sequences/notes-subsequences` | Medium | · |
| 225 | III | Sequential Compactness of Closed Intervals | `analysis/real-analysis/notes/sequences/notes-subsequences` | Medium | · |
| 226 | III | Monotone Subsequence Theorem | `analysis/real-analysis/notes/sequences/notes-subsequences` | Hard | · |
| 227 | III | Characterization of Divergence | `analysis/real-analysis/notes/sequences/notes-subsequences` | Hard | · |
| 228 | III | Finite Partition Convergence Principle | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 229 | III | Residue-Class Convergence | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 230 | III | Even-Odd Convergence | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 231 | III | Convergence is Inherited | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 232 | III | Boundedness is Inherited | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 233 | III | Monotonicity is Inherited | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 234 | III | Cauchy Property is Inherited | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 235 | III | Subsequence of a Subsequence | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 236 | III | Dense Subsequence Criterion | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Hard | · |
| 237 | III | Diagonal Subsequence Lemma | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Hard | · |
| 238 | III | Reflection via Universal Properties | `analysis/real-analysis/notes/sequences/notes-subsequence-toolkit` | Medium | · |
| 239 | III | Every convergent sequence is Cauchy | `analysis/real-analysis/notes/sequences/notes-cauchy` | Hard | · |
| 240 | III | Every convergent series has Cauchy partial sums | `analysis/real-analysis/notes/sequences/notes-cauchy` | Medium | · |
| 241 | III | Cauchy sequences are bounded | `analysis/real-analysis/notes/sequences/notes-cauchy` | Medium | · |
| 242 | III | Bolzano--Weierstrass Theorem | `analysis/real-analysis/notes/sequences/notes-cauchy` | Hard | · |
| 243 | III | Cauchy Criterion in ℝ | `analysis/real-analysis/notes/sequences/notes-cauchy` | Hard | · |
| 244 | III | Monotonicity of tail suprema and infima | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 245 | III | Equivalent formulation as inf-sup | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Medium | · |
| 246 | III | Equivalent characterizations | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Medium | · |
| 247 | III | Basic inequalities | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 248 | III | Sign symmetry | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 249 | III | Order preservation | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 250 | III | Subadditivity of \limsup | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Medium | · |
| 251 | III | Superadditivity of \liminf | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Medium | · |
| 252 | III | Equality when one sequence converges | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 253 | III | Addition law for convergent sequences | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 254 | III | Scalar multiplication | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 255 | III | Product inequality for nonnegative sequences | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 256 | III | Characterization via subsequences | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 257 | III | Extremal subsequences | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 258 | III | Convergence criterion | `analysis/real-analysis/notes/asymptotics/notes-limsup-liminf` | Hard | · |
| 259 | III | Iterative Propagation | `analysis/real-analysis/notes/asymptotics/notes-recurrence-relations` | Medium | · |
| 260 | III | Multiplicative Recurrence Bound | `analysis/real-analysis/notes/asymptotics/notes-recurrence-relations` | Medium | · |
| 261 | III | Affine Recurrence Bound | `analysis/real-analysis/notes/asymptotics/notes-recurrence-relations` | Medium | · |
| 262 | III | Discrete Grönwall Inequality | `analysis/real-analysis/notes/asymptotics/notes-recurrence-relations` | Hard | · |
| 263 | III | Fekete's Lemma for Subadditive Sequences | `analysis/real-analysis/notes/asymptotics/notes-recurrence-relations` | Hard | · |
| 264 | III | Fekete's Lemma for Superadditive Sequences | `analysis/real-analysis/notes/asymptotics/notes-recurrence-relations` | Hard | · |
| 265 | III | Contraction Sequence Is Cauchy | `analysis/real-analysis/notes/asymptotics/notes-recurrence-relations` | Hard | · |
| 266 | III | Contraction Subsequence Is Cauchy | `analysis/real-analysis/notes/asymptotics/notes-recurrence-relations` | Hard | · |
| 267 | III | Ratio theorem for sequences | `analysis/real-analysis/notes/asymptotics/notes-growth-and-asymptotics` | Medium | · |
| 268 | III | Root theorem for sequences | `analysis/real-analysis/notes/asymptotics/notes-growth-and-asymptotics` | Medium | · |
| 269 | III | Properties of asymptotic equivalence | `analysis/real-analysis/notes/asymptotics/notes-growth-and-asymptotics` | Medium | · |
| 270 | III | Stolz--Cesàro | `analysis/real-analysis/notes/asymptotics/notes-growth-and-asymptotics` | Hard | · |
| 271 | III | Fundamental asymptotic limits | `analysis/real-analysis/notes/asymptotics/notes-growth-and-asymptotics` | Hard | · |
| 272 | III | Direct Comparison Test | `analysis/real-analysis/notes/series/notes-series-tests` | Medium | · |
| 273 | III | Absolute convergence is stable under rearrangement | `analysis/real-analysis/notes/series/notes-series-rearrangements` | Hard | · |
| 274 | III | Limit Comparison Test | `analysis/real-analysis/notes/series/notes-series-tests` | Medium | · |
| 275 | III | Cauchy Condensation Test | `analysis/real-analysis/notes/series/notes-series` | Medium | · |
| 276 | III | Riemann Rearrangement Theorem | `analysis/real-analysis/notes/series/notes-series-rearrangements` | Hard | · |
| 277 | III | Ratio Test | `analysis/real-analysis/notes/series/notes-series-tests` | Medium | · |
| 278 | III | Regrouping and convergence | `analysis/real-analysis/notes/series/notes-series-rearrangements` | Hard | · |
| 279 | III | Root Test | `analysis/real-analysis/notes/series/notes-series-tests` | Medium | · |
| 280 | III | Integral Test | `analysis/real-analysis/notes/series/notes-series-tests` | Hard | · |
| 281 | III | Cauchy Product Theorem | `analysis/real-analysis/notes/series/notes-series-rearrangements` | Hard | · |
| 282 | III | p-Series | `analysis/real-analysis/notes/series/notes-series-tests` | Medium | · |
| 283 | III | Alternating Series Test | `analysis/real-analysis/notes/series/notes-series-tests` | Medium | · |
| 284 | III | Dirichlet Test | `analysis/real-analysis/notes/series/notes-series-tests` | Hard | · |
| 285 | III | Abel Test | `analysis/real-analysis/notes/series/notes-series-tests` | Medium | · |
| 286 | III | Absolute convergence implies convergence | `analysis/real-analysis/notes/series/notes-absolute-convergence` | Hard | · |
| 287 | III | Comparison via absolute values | `analysis/real-analysis/notes/series/notes-absolute-convergence` | Hard | · |
| 288 | III | Absolute convergence is rearrangement invariant | `analysis/real-analysis/notes/series/notes-absolute-convergence` | Hard | · |
| 289 | III | Radius of Convergence Theorem | `analysis/real-analysis/notes/series/notes-power-series` | Hard | · |
| 290 | III | Cauchy--Hadamard Formula | `analysis/real-analysis/notes/series/notes-power-series` | Hard | · |
| 291 | III | Term-by-term differentiation | `analysis/real-analysis/notes/series/notes-power-series` | Hard | · |