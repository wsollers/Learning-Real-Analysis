-- LRA/VolumeI/Sets.lean
import LRA.VolumeI.Sets.SetDefinitions

-- This is the crucial line: it brings Set, inter, and union into scope
open LRA.VolumeI.Sets

namespace LRA.VolumeI.Sets

variable {α : Type}

/--
Theorem: Intersection distributes over Union.
A ∩ (B ∪ C) = (A ∩ B) ∪ (A ∩ C)
-/
theorem Intersection_Distributes_Over_Union (A B C : Set α) :
  intersection A (union B C) = union (intersection A B) (intersection A C) := by
  -- Use extensionality to turn a set equality into a logical equivalence
  -- Goal: x ∈ A ∩ (B ∪ C) ↔ x ∈ (A ∩ B) ∪ (A ∩ C)
  apply axiom_set_extension
  intro x
  constructor
  -- ══════════════════════════════════════════════
  -- (→) x ∈ A ∩ (B ∪ C)  →  x ∈ (A ∩ B) ∪ (A ∩ C)
  -- ══════════════════════════════════════════════
  · intro hypothesis -- Let x be an arbitrary element of α := A ∩ (B ∪ C)
  obtain ⟨x_in_A, x_in_union⟩ := hypothesis
  cases x_in_union with
  | inl x_in_B =>
    -- Case 1: x ∈ B
    -- Goal: x ∈ (A ∩ B) ∪ (A ∩ C)
    apply Or.inl
    exact ⟨x_in_A, x_in_B⟩
  | inr x_in_C =>
    -- Case 2: x ∈ C
    exact Or.inr ⟨x_in_A, x_in_C⟩
  -- Now we need to prove the reverse direction: x ∈ A ∩ (B ∪ C)
  · intro hypothesis -- Let x be an arbitrary element of α := (A ∩ B) ∪ (A ∩ C)
  cases hypothesis with
  | inl x_in_A_and_B =>
    -- Case 1: x ∈ A ∩ B
    -- Goal: x ∈ A ∩ (B ∪ C)
    obtain ⟨x_in_A, x_in_B⟩ := x_in_A_and_B
    apply And.intro
    exact x_in_A
    apply Or.inl
    exact x_in_B
  | inr x_in_A_and_C =>
    -- Case 2: x ∈ A ∩ C
    obtain ⟨x_in_A, x_in_C⟩ := x_in_A_and_C
    apply And.intro
    exact x_in_A
    apply Or.inr
    exact x_in_C


end LRA.VolumeI.Sets
