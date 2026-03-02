# RA-to-do-with-dependencies.md

Each theorem lists its real-analysis dependencies explicitly.
You may not attempt a theorem unless all dependencies are proven cleanly in your own hand.

---

## Level 0 — Order Foundations (No Sequence Dependencies)

### Upper Bound is Unique
Dependencies: Definition of Upper Bound

### Lower Bound is Unique
Dependencies: Definition of Lower Bound

### Maximum Implies Supremum
Dependencies: Definition of Maximum, Definition of Supremum

### Minimum Implies Infimum
Dependencies: Definition of Minimum, Definition of Infimum

### Supremum is Unique
Dependencies: Definition of Supremum

### Infimum is Unique
Dependencies: Definition of Infimum

### Every Nonempty Finite Set Has a Maximum
Dependencies: Order Axioms

### Archimedean Property of ℝ
Dependencies: Order Axioms, Field Properties

### Density of ℚ in ℝ
Dependencies: Archimedean Property

### Density of Irrationals in ℝ
Dependencies: Density of ℚ in ℝ

---

## Level 1 — Completeness Machinery

### Characterization of Supremum (ε-Formulation)
Dependencies: Definition of Supremum

### Monotone Increasing and Bounded ⇒ Convergent
Dependencies: LUB Property, Supremum Characterization

### Monotone Decreasing and Bounded ⇒ Convergent
Dependencies: GLB Property

### Nested Interval Theorem
Dependencies: LUB Property

---

## Level 2 — Basic Sequence Theory

### Convergent Sequence Has Unique Limit
Dependencies: Definition of Limit

### Convergent Sequence is Bounded
Dependencies: Definition of Limit

### Limit Laws (Sum, Product, Quotient)
Dependencies: Definition of Limit, Triangle Inequality

### Order Limit Theorem
Dependencies: Limit Definition, Order Properties

### Squeeze Theorem
Dependencies: Order Limit Theorem

---

## Level 3 — Subsequences

### Subsequence of Convergent Sequence Converges to Same Limit
Dependencies: Definition of Subsequence, Limit Definition

### Monotone Subsequence Theorem
Dependencies: Order Properties

### Bolzano–Weierstrass Theorem
Dependencies: Monotone Subsequence Theorem, Monotone Convergence Theorem

---

## Level 4 — Cauchy Theory

### Convergent ⇒ Cauchy
Dependencies: Definition of Limit, Definition of Cauchy

### Cauchy ⇒ Bounded
Dependencies: Definition of Cauchy

### Cauchy + Subsequence Converges ⇒ Whole Sequence Converges
Dependencies: Cauchy ⇒ Bounded, Subsequence Limit Theorem

### Cauchy ⇒ Convergent (Completeness of ℝ)
Dependencies: Bolzano–Weierstrass, Previous Cauchy Results

---

## Level 5 — Completeness Equivalences

### LUB ⇔ Monotone Convergence Theorem
Dependencies: LUB Property, Monotone Convergence

### LUB ⇔ Nested Interval Property
Dependencies: Nested Interval Theorem

### LUB ⇔ Cauchy Completeness
Dependencies: Cauchy ⇒ Convergent

---

## Level 6 — Compactness

### Sequential Compactness of Closed Bounded Intervals
Dependencies: Bolzano–Weierstrass

### Heine–Borel (Sequential Version in ℝ)
Dependencies: Sequential Compactness
