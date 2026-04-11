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
theorem dist_inter_union (A B C : Set α) :
  inter A (union B C) = union (inter A B) (inter A C) := by
  -- Use extensionality to turn a set equality into a logical equivalence
  apply set_ext
  intro x
  -- Goal: x ∈ A ∩ (B ∪ C) ↔ x ∈ (A ∩ B) ∪ (A ∩ C)
  sorry

end LRA.VolumeI.Sets
