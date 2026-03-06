# Metric Spaces — Proofs To Do

Sorted by dependency level. Complete all Easy items before Medium; all Medium before Hard.

---

## Level 0 — Pure Metric (no topology, no sequences)

---

- [ ] **T15.6 — Properties of the Modulus** | Easy
  - Def 15.1 Modulus / absolute value

---

- [ ] **P15.32 — Distance to Itself** `d(x,x) = 0` | Easy
  - Def 15.31 Metric (M1 non-negativity, M2 identity)

---

- [ ] **P15.33 — Positivity** `x ≠ y ⇒ d(x,y) > 0` | Easy
  - Def 15.31 Metric (M1, M2)

---

- [✅] **T15.18 — Monotonicity of Diameter** `A ⊆ B ⇒ diam(A) ≤ diam(B)` | Easy
  - Def 15.15 Diameter
  - Def 15.31 Metric
  - Supremum is the least upper bound (from real number foundations)
  - Def of subset

---

- [ ] **P15.21 — Discrete Metric** | Easy
  - Def 15.31 Metric (verify all four axioms for d_disc)
  - Def 15.55 Open Ball (to show B_{1/2}(x) = {x})

---

- [ ] **P15.34 — Reverse Triangle Inequality** `|d(x,z) − d(y,z)| ≤ d(x,y)` | Easy
  - Def 15.31 Metric (M3 symmetry, M4 triangle inequality)
  - P15.58 Three forms of triangle inequality (or derive inline)

---

- [ ] **P15.35 — Generalised Triangle Inequality** | Easy
  - Def 15.31 Metric (M4)
  - Induction on n (proof technique)

---

- [ ] **P15.36 — Continuity of Distance** `x ↦ d(x,z)` is continuous | Easy
  - P15.34 Reverse triangle inequality
  - Def of convergence in a metric space (Def 15.81)

---

- [ ] **P15.25 — Convergence in the Product Metric** | Easy
  - Def 15.23 Product metric
  - Def 15.81 Convergence in a metric space

---

- [ ] **P15.43 — Injectivity of Isometries** | Easy
  - Def 15.41 Isometry
  - Def 15.31 Metric (M2 identity)

---

- [ ] **P15.44 — Inverse of a Bijective Isometry** | Easy
  - Def 15.41 Isometry (bijective case)

---

- [✅] **P15.46 — Preservation of Diameter** | Easy
  - Def 15.41 Isometry
  - Def 15.15 Diameter

---

- [ ] **T15.40 — Degeneracy of the Antimetric** | Easy
  - Def 15.39 Antimetric (reversed triangle inequality + non-negativity + identity)

---

## Level 1 — Open and Closed Balls

---

- [ ] **P15.58 — Three Forms of the Triangle Inequality** | Easy
  - Def 15.31 Metric (M3 symmetry, M4 triangle inequality)

---

- [ ] **P15.60 — Open Ball as a Union of Closed Balls** `Bᵣ(x) = ⋃_{s<r} B̄ₛ(x)` | Easy
  - Def 15.55 Open Ball
  - Def 15.56 Closed Ball
  - Def 15.31 Metric

---

- [ ] **P15.61 — Closed Ball as an Intersection of Open Balls** `B̄ᵣ(x) = ⋂_{ε>0} B_{r+ε}(x)` | Easy
  - Def 15.55 Open Ball
  - Def 15.56 Closed Ball
  - Def 15.31 Metric

---

- [ ] **P15.45 — Preservation of Open Balls** by isometries | Easy
  - Def 15.41 Isometry
  - Def 15.55 Open Ball

---

## Level 2 — Open Sets and Closed Sets

---

- [ ] **P15.72 — Trivial Open and Closed Sets** (∅ and X) | Easy
  - Def 15.71 Open Set / Closed Set
  - Vacuous truth (logic)

---

- [ ] **P15.73 — Balls are Open and Closed** | Easy
  - Def 15.55 Open Ball, Def 15.56 Closed Ball
  - Def 15.71 Open Set / Closed Set
  - Def 15.31 Metric (M4 triangle inequality)
  - Interior ball identity / radial gap (from notes-balls)

---

- [ ] **T15.87 — Arbitrary Unions of Open Sets are Open** | Easy
  - Def 15.71 Open Set
  - Def 15.55 Open Ball

---

- [ ] **T15.88 — Finite Intersections of Open Sets are Open** | Easy
  - Def 15.71 Open Set
  - Def 15.55 Open Ball
  - Finiteness of minimum of finitely many positive radii

---

- [ ] **T15.89 — Arbitrary Intersections of Closed Sets are Closed** | Easy
  - Def 15.71 Closed Set
  - T15.87 Arbitrary unions of open sets are open
  - De Morgan's law

---

- [ ] **T15.90 — Finite Union of Closed Sets is Closed** | Easy
  - Def 15.71 Closed Set
  - T15.88 Finite intersections of open sets are open
  - De Morgan's law

---

- [ ] **P15.47 — Preservation of Open and Closed Sets** by bijective isometries | Easy
  - Def 15.41 Isometry (bijective)
  - P15.45 Preservation of open balls
  - Def 15.71 Open Set / Closed Set

---

- [ ] **T15.77 — Interior–Boundary–Exterior Decomposition** | Easy
  - Def 15.74 Interior
  - Def 15.75 Exterior
  - Def 15.76 Boundary
  - Set theory: mutual exclusivity and exhaustiveness

---

- [ ] **T15.78 — Boundary via Closure** `∂A = Ā ∩ (X\A)‾` | Medium
  - Def 15.76 Boundary
  - Def of closure (as set of all adherent points)
  - T15.77 Decomposition theorem

---

- [ ] **P15.79 — Closed Sets Contain Their Boundary** `F closed ⟺ ∂F ⊆ F` | Medium
  - Def 15.71 Closed Set
  - Def 15.76 Boundary
  - T15.77 Decomposition theorem

---

## Level 3 — Limit Points and Sequences

---

- [ ] **L15.82 — Convergence is Preserved by Isometries** | Easy
  - Def 15.81 Convergence in a metric space
  - Def 15.41 Isometry

---

- [ ] **T15.86 — Closed Sets and Open Complements** `F closed ⟺ X\F open` | Easy
  - Def 15.71 Closed Set (this is essentially definitional — may be trivial)
  - Def 15.71 Open Set

---

- [ ] **T15.65 — Sequential Characterization of Limit Points** | Medium
  - Def 15.62 Limit point
  - Def 15.81 Convergence in a metric space
  - Def 15.55 Open Ball
  - Construction of a sequence using shrinking radii 1/n

---

- [ ] **C15.66 — Infinite Neighbourhood Characterization** | Medium
  - T15.65 Sequential characterization of limit points
  - Def 15.62 Limit point
  - Finite sets have no limit points (consequence)

---

- [ ] **T15.67 — Derived Set is Closed** | Medium
  - Def 15.63 Derived set E′
  - T15.65 Sequential characterization of limit points
  - Def 15.81 Convergence in a metric space

---

- [ ] **T15.68 — Closed Sets and Derived Sets** `E closed ⟺ E′ ⊆ E` | Medium
  - Def 15.71 Closed Set
  - Def 15.63 Derived set E′
  - T15.65 Sequential characterization of limit points

---

- [ ] **T15.69 — Closure Formula** `Ē = E ∪ E′` | Medium
  - Def of closure Ē
  - Def 15.63 Derived set E′
  - T15.67 Derived set is closed
  - T15.68 Closed sets and derived sets

---

- [ ] **T15.85 — Sequential Characterization of Closure Points** | Medium
  - Def of closure Ē (adherent points)
  - Def 15.81 Convergence in a metric space
  - Def 15.55 Open Ball
  - Construction of a sequence using shrinking radii 1/n

---

- [ ] **T15.91 — Equivalent Characterizations of Closure** | Medium
  - Def of closure Ē
  - T15.85 Sequential characterization of closure points
  - Def 15.62 Limit point

---

- [ ] **T15.94 / T15.95 — Continuity of the Metric** `d(xₙ,yₙ) → d(x,y)` | Easy
  - Def 15.81 Convergence in a metric space
  - P15.34 Reverse triangle inequality
  - Convergence of real sequences

---

## Level 4 — Sequential Characterizations of Open and Closed Sets

---

- [ ] **T15.86 — Closed Sets are Sequentially Closed** (= T15.102 (iii)) | Medium
  - Def 15.71 Closed Set
  - Def 15.81 Convergence in a metric space
  - T15.88 Finite intersections of open sets are open (used in the contrapositive direction)

---

- [ ] **T15.102 — Equivalent Characterizations of Closed Sets** | Medium
  - Def 15.71 Closed Set (topological)
  - Def 15.62 Limit point (limit-point condition)
  - T15.65 Sequential characterization of limit points
  - T15.68 Closed sets and derived sets
  - Def 15.81 Convergence (sequential condition)

---

- [ ] **P15.105 — Sequential Duality of Open and Closed Sets** | Medium
  - Def 15.71 Open Set / Closed Set
  - Def 15.81 Convergence in a metric space
  - T15.102 Equivalent characterizations of closed sets
  - De Morgan's laws (complement duality)

---

## Level 5 — Isometry Structure Theorems

---

- [ ] **P15.48 — Preservation of Convergence and Cauchy Sequences** | Easy
  - Def 15.41 Isometry (bijective)
  - Def 15.81 Convergence in a metric space
  - Def of Cauchy sequence
  - L15.82 Convergence is preserved by isometries

---

- [ ] **P15.49 — Isometry Group** | Medium
  - Def 15.41 Isometry (bijective)
  - Group axioms (closure, associativity, identity, inverses)
  - P15.44 Inverse of a bijective isometry

---

- [ ] **P15.51 — Rotations are Isometries** | Easy
  - Def 15.41 Isometry
  - Def of orthogonal matrix (R⊤R = I)
  - Euclidean norm properties

---

- [ ] **P15.52 — Translations are Isometries** | Easy
  - Def 15.41 Isometry
  - Def of translation Tₐ(x) = x + a

---

- [ ] **P15.53 — Reflections are Isometries** | Easy
  - Def 15.41 Isometry
  - P15.51 Rotations are isometries (reflection matrix is orthogonal)

---

- [ ] **T15.54 — Classification of Euclidean Rigid Motions** | Hard
  - Def 15.50 Rigid motion
  - P15.51 Rotations are isometries
  - P15.52 Translations are isometries
  - Linear algebra: orthogonal matrices, O(n)
  - Key step: g = f ∘ T⁻¹_{f(0)} fixes the origin and is linear

---