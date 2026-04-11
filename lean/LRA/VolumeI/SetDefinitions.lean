namespace LRA.VolumeI

-- A Set is defined as a predicate on a Type alpha
def Set (α : Type) := α → Prop

-- Element-of relation: x ∈ S
def mem (x : α) (s : Set α) : Prop := s x

-- Intersection (A ∩ B): The set of x such that x is in A AND x is in B
def inter (s1 s2 : Set α) : Set α :=
  fun x => mem x s1 ∧ mem x s2

-- Union (A ∪ B): The set of x such that x is in A OR x is in B
def union (s1 s2 : Set α) : Set α :=
  fun x => mem x s1 ∨ mem x s2

-- Extensionality: Two sets are equal if they have the same members.
-- This is the only "Axiom" we need for these proofs.
axiom set_ext {α : Type} (A B : Set α) : (∀ x, mem x A ↔ mem x B) → A = B

end LRA.VolumeI
