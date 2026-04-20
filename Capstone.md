# Capstone: Reading a Real Mathematics Paper

## The Target Paper

**Primary announcement (6 pages):**

> M. Arnaudon, K. A. Coulibaly, A. Thalmaier.
> *Brownian motion with respect to a metric depending on time;
> definition, existence and applications to Ricci flow.*
> C. R. Acad. Sci. Paris, Ser. I, **346** (13–14), pp. 773–778, 2008.
> DOI: 10.1016/j.crma.2008.05.004

**Full technical paper (22 pages, recommended reading target):**

> K. A. Coulibaly-Pasquier.
> *Brownian motion with respect to time-changing Riemannian metrics,
> applications to Ricci flow.*
> Ann. Inst. Henri Poincaré Probab. Stat. **47** (2), pp. 515–538, 2011.
> arXiv: **0901.1999**
> https://arxiv.org/abs/0901.1999

**Why two papers.** The Comptes Rendus paper is a 6-page announcement:
results are stated, proofs are compressed or omitted. The Coulibaly-Pasquier
paper is the full technical development of the same ideas with complete proofs.
The capstone goal is to read both. The announcement is the target to parse
first; the full paper is the target to understand completely.

---

## What the Paper Does — One Paragraph

Given a compact smooth manifold $M$ equipped with a smoothly
time-varying family of Riemannian metrics $g(t)$, the paper constructs
a stochastic process $X_t$ on $M$ — called $g(t)$-Brownian motion —
whose infinitesimal generator at each time $t$ is the Laplace-Beltrami
operator $\Delta_{g(t)}$ of the current metric. The construction works
by lifting to the orthonormal frame bundle $O_{g(t)}(M)$ and running a
horizontal diffusion there, then projecting back to $M$. The paper then
derives: a martingale representation formula for a nonlinear PDE on $M$;
a Bismut-type formula giving a probabilistic representation of the
gradient of the heat semigroup; and a characterization of the Ricci
flow in terms of damped parallel transport along $g(t)$-Brownian paths.

---

## Mathematical Objects Named in the Paper

Every object below must be formally defined in your notes before the
paper becomes fully legible. The pointer column records the canonical
location once the definition exists. Objects are listed in the order
a reader encounters them.

---

### Layer 1 — Smooth manifolds and Riemannian geometry

| Object | Informal meaning | Notes pointer |
|--------|-----------------|---------------|
| Smooth manifold $M$ | A topological space that locally looks like $\mathbb{R}^n$, with smooth transition maps between charts | `volume-v / diff-geo / def:smooth-manifold` |
| Compact manifold | A manifold with no boundary and finite extent; every open cover has a finite subcover | `volume-v / topology / def:compact` |
| $n$-dimensional manifold | Dimension equals the dimension of the local chart space $\mathbb{R}^n$ | `volume-v / diff-geo / def:dimension-manifold` |
| Riemannian metric $g$ | A smoothly varying inner product $g_p : T_p M \times T_p M \to \mathbb{R}$ on each tangent space | `volume-v / diff-geo / def:riemannian-metric` |
| Riemannian manifold $(M, g)$ | A smooth manifold equipped with a Riemannian metric | `volume-v / diff-geo / def:riemannian-manifold` |
| Family of metrics $g(t)$ | A one-parameter family $t \mapsto g(t)$ of Riemannian metrics, smooth in $t$ | `volume-v / diff-geo / def:time-dependent-metric` |
| Tangent space $T_p M$ | The vector space of all tangent vectors to $M$ at the point $p$ | `volume-v / diff-geo / def:tangent-space` |
| Tangent bundle $TM$ | The union $\bigsqcup_{p \in M} T_p M$ with its natural smooth structure | `volume-v / diff-geo / def:tangent-bundle` |
| Levi-Civita connection $\nabla^{g(t)}$ | The unique torsion-free metric connection associated to $g(t)$ | `volume-v / diff-geo / def:levi-civita-connection` |
| Laplace-Beltrami operator $\Delta_{g(t)}$ | The canonical second-order differential operator on $(M, g(t))$; generalizes $\Delta = \sum \partial^2/\partial x_i^2$ | `volume-v / diff-geo / def:laplace-beltrami` |
| Geodesic | A curve $\gamma$ on $M$ satisfying $\nabla^g_{\dot\gamma} \dot\gamma = 0$; locally length-minimizing | `volume-v / diff-geo / def:geodesic` |
| Geodesically complete manifold | Every geodesic extends to all of $\mathbb{R}$; equivalent to the Hopf-Rinow theorem conditions | `volume-v / diff-geo / thm:hopf-rinow` |
| Ricci curvature $\mathrm{Ric}(g)$ | A symmetric $(0,2)$-tensor obtained by tracing the Riemann curvature tensor | `volume-v / diff-geo / def:ricci-curvature` |
| Ricci flow | The PDE $\partial_t g(t) = -2\,\mathrm{Ric}(g(t))$ on the space of metrics | `volume-v / diff-geo / def:ricci-flow` |
| Mean curvature flow | Evolution of a hypersurface by its mean curvature vector | `volume-v / diff-geo / def:mean-curvature-flow` |
| Parallel transport | The operation of moving a tangent vector along a curve while keeping it parallel with respect to $\nabla^g$ | `volume-v / diff-geo / def:parallel-transport` |
| Damped parallel transport | A modification of parallel transport incorporating a curvature-dependent damping term; reduces to ordinary parallel transport when curvature vanishes | `volume-v / diff-geo / def:damped-parallel-transport` |

---

### Layer 2 — Frame bundles and connections

| Object | Informal meaning | Notes pointer |
|--------|-----------------|---------------|
| Frame bundle $F(M)$ | The principal $GL(n)$-bundle whose fiber over $p \in M$ is the set of all ordered bases of $T_p M$ | `volume-v / diff-geo / def:frame-bundle` |
| Orthonormal frame bundle $O_{g(t)}(M)$ | The sub-bundle of $F(M)$ whose fiber over $p$ is the set of $g(t)$-orthonormal bases of $T_p M$; varies with $t$ | `volume-v / diff-geo / def:orthonormal-frame-bundle` |
| Principal bundle | A fiber bundle with a free transitive structure group action on the fibers | `volume-v / diff-geo / def:principal-bundle` |
| Connection on a principal bundle | A decomposition of the tangent bundle of $F(M)$ into horizontal and vertical subspaces | `volume-v / diff-geo / def:principal-connection` |
| Horizontal vector field $H_i(t, u)$ | The horizontal lift of the $i$-th standard basis vector $e_i$ to the frame bundle, with respect to $\nabla^{g(t)}$ | `volume-v / diff-geo / def:horizontal-lift` |
| Projection $\pi : F(M) \to M$ | The bundle projection sending a frame $u$ at $p$ to the base point $p$ | `volume-v / diff-geo / def:bundle-projection` |
| $g(t)$-horizontal vector field | The horizontal vector field on $F(M)$ built from the $g(t)$-Levi-Civita connection | `volume-v / diff-geo / def:g-horizontal-field` |

---

### Layer 3 — Stochastic analysis and SDEs

| Object | Informal meaning | Notes pointer |
|--------|-----------------|---------------|
| Probability space $(\Omega, \mathcal{F}, \mathbb{P})$ | The triple of sample space, sigma-algebra of events, and probability measure | `volume-v / measure-theory / def:probability-space` |
| Brownian motion in $\mathbb{R}^n$ | A continuous stochastic process with independent Gaussian increments and covariance $t \cdot I$ | `volume-v / stochastic / def:brownian-motion-rn` |
| Stochastic differential equation (SDE) | An equation of the form $dX_t = b(X_t)\,dt + \sigma(X_t)\,dB_t$ | `volume-v / stochastic / def:sde` |
| Itô SDE vs Stratonovich SDE | Two conventions for stochastic integration; Stratonovich obeys the ordinary chain rule and is natural on manifolds | `volume-v / stochastic / def:stratonovich-integral` |
| Stratonovich SDE on $F(M)$ | The SDE $dU_t = \sum_i H_i(t, U_t) \circ dB^i_t$ on the frame bundle; the $\circ$ denotes Stratonovich integration | `volume-v / stochastic / def:stratonovich-sde-frame-bundle` |
| $g(t)$-Brownian motion $X_t$ | The projection $X_t = \pi(U_t)$ of the frame bundle process to $M$; the main object constructed | `volume-v / stochastic / def:g-t-brownian-motion` |
| Generator of a diffusion | The second-order differential operator $L$ such that $f(X_t) - \int_0^t Lf(X_s)\,ds$ is a martingale | `volume-v / stochastic / def:generator` |
| Martingale | A stochastic process $M_t$ with $\mathbb{E}[M_t \mid \mathcal{F}_s] = M_s$ for $s \leq t$ | `volume-v / stochastic / def:martingale` |
| Filtration $\mathcal{F}_t$ | An increasing family of sigma-algebras encoding information up to time $t$ | `volume-v / measure-theory / def:filtration` |
| Semimartingale | A stochastic process decomposable as a local martingale plus a finite-variation process; the natural class for stochastic integration | `volume-v / stochastic / def:semimartingale` |
| Itô formula | The stochastic chain rule: $df(X_t) = \nabla f \cdot dX_t + \frac{1}{2} \Delta f\,dt$ for Brownian motion | `volume-v / stochastic / thm:ito-formula` |
| Itô formula on a manifold | The version of Itô's formula for a diffusion on a Riemannian manifold, involving the Levi-Civita connection | `volume-v / stochastic / thm:ito-formula-manifold` |
| Heat semigroup $P_t$ | The operator $P_t f(x) = \mathbb{E}^x[f(X_t)]$ giving the expected value of $f$ along Brownian paths started at $x$ | `volume-v / stochastic / def:heat-semigroup` |
| Heat equation on $(M, g(t))$ | The PDE $(\partial_t - \Delta_{g(t)}) u = 0$; governs the evolution of the probability density of $X_t$ | `volume-v / pde / def:heat-equation-manifold` |

---

### Layer 4 — Advanced objects (graduate level)

| Object | Informal meaning | Notes pointer |
|--------|-----------------|---------------|
| Martingale representation theorem | Every $L^2$ martingale can be written as a stochastic integral with respect to Brownian motion | `volume-v / stochastic / thm:martingale-representation` |
| Bismut formula | A probabilistic formula for $\nabla_x P_t f(x)$: the gradient of the heat semigroup expressed as an expectation involving Brownian paths | `volume-v / stochastic / thm:bismut-formula` |
| Bismut-type formula for geometric quantities | A generalization of the Bismut formula to quantities evolving under a geometric flow | `volume-v / stochastic / thm:bismut-geometric-flow` |
| Dohrn-Guerra parallel transport | An alternative stochastic parallel transport arising in Nelson's stochastic mechanics; the paper shows it coincides with damped parallel transport under Ricci flow | `volume-v / stochastic / def:dohrn-guerra-transport` |
| Gradient estimate for heat equation | A bound of the form $\|\nabla P_t f\| \leq C(t) \cdot P_t(\|f\|)$ controlling spatial variation of the heat semigroup | `volume-v / stochastic / thm:gradient-estimate-heat` |
| Nonlinear PDE over $M$ | In this context: a PDE of the form $\partial_t u = \frac{1}{2}\Delta_{g(t)} u + F(u, \nabla u)$ for some nonlinearity $F$ | `volume-v / pde / def:nonlinear-pde-manifold` |
| Wasserstein distance | A metric on probability measures on $M$ based on optimal transport; appears in contraction estimates for the heat flow | `volume-v / measure-theory / def:wasserstein-distance` |

---

## Theorems and Results in the Paper

Each result below needs a corresponding proof file in your notes
before you can claim to have verified the paper's claims independently.

| Result | Statement (informal) | Notes pointer |
|--------|---------------------|---------------|
| Existence of $g(t)$-Brownian motion | The Stratonovich SDE on $F(M)$ has a unique strong solution $U_t$ with $U_t \in O_{g(t)}(M)$ for all $t$; the projection $X_t = \pi U_t$ is the $g(t)$-Brownian motion | `proofs / stochastic / prf:g-t-bm-existence` |
| Generator identification | The generator of $X_t$ at time $t$ is $\frac{1}{2}\Delta_{g(t)}$ | `proofs / stochastic / prf:g-t-bm-generator` |
| Martingale representation | For solutions $u$ of the heat equation $(\partial_t - \frac{1}{2}\Delta_{g(t)})u = 0$, the process $u(t, X_t)$ is a martingale | `proofs / stochastic / prf:heat-martingale` |
| Bismut-type gradient formula | $\nabla_x u(0, x) = -\frac{1}{t} \mathbb{E}^x\bigl[u(t, X_t) \int_0^t \langle \mathbb{T}_{0,s}, dB_s \rangle \bigr]$ where $\mathbb{T}_{0,s}$ is the damped parallel transport | `proofs / stochastic / prf:bismut-formula-g-t` |
| Gradient control for heat equation | Under forward Ricci flow, $\|\nabla P_t f\|_{g(0)} \leq P_t(\|\nabla f\|_{g(t)})$ | `proofs / stochastic / prf:gradient-control-ricci` |
| Ricci flow characterization | The forward Ricci flow is the unique geometric flow for which the damped parallel transport is an isometry | `proofs / stochastic / prf:ricci-flow-characterization` |

---

## Key References Cited by the Paper

These are the works the paper builds on directly. Each is a future
reading target once the prerequisites are in place.

| Reference | What it contributes |
|-----------|-------------------|
| Hsu, E. P. *Stochastic Analysis on Manifolds* (AMS, 2002) | The standard reference for SDEs on manifolds, Itô formula on manifolds, and the frame bundle construction for fixed-metric Brownian motion. Direct prerequisite. |
| Elworthy, K. D. *Stochastic Differential Equations on Manifolds* (Cambridge, 1982) | Earlier foundational treatment of diffusions on manifolds and the Bismut formula. |
| Hackenbroch, W. and Thalmaier, A. *Stochastische Analysis* (Teubner, 1994) | One of the authors' own reference for stochastic analysis foundations. In German. |
| Coulibaly-Pasquier, K. A. arXiv:0901.1999 (2009/2011) | The full technical paper expanding the Comptes Rendus announcement. The primary reading target. |
| Kuwada, K. and Philipowski, R. arXiv:0910.1730 (2010) | Proves non-explosion of the $g(t)$-Brownian motion; derives Itô formula for geodesic distance under evolving metric. |
| Arnaudon, Coulibaly, Thalmaier. arXiv:0904.2762 (2011) | Horizontal diffusion in $C^1$ path space; proves contraction in Wasserstein distance under Ricci flow. |
| Chavel, I. *Riemannian Geometry: A Modern Introduction* (Cambridge, 1993) | Standard reference for Riemannian geometry prerequisites. |

---

## Reading Log

Each entry records the date, the current curriculum stage, what was
understood, and what remained opaque. The goal is to accumulate
entries over years until every line is clear.

---

### Entry 0 — Project start (add date when written)

**Curriculum stage:** Real analysis — sequences chapter. Linear algebra
just beginning.

**What is clear:**
- The paper exists and concerns Brownian motion on a manifold with
  a time-varying metric.
- The motivating application is Ricci flow and mean curvature flow.
- The construction uses the frame bundle and a Stratonovich SDE.
- The main results include a Bismut formula and a Ricci flow
  characterization.

**What is opaque (everything below this line):**
- What a smooth manifold is, formally.
- What a Riemannian metric is.
- What the frame bundle is or why it is needed.
- What a Stratonovich SDE is or how it differs from Itô.
- What the Laplace-Beltrami operator is.
- What a martingale is.
- What the Bismut formula says or why it is useful.
- What Ricci flow is.
- Why the damped parallel transport characterizes the Ricci flow.

**Honest self-assessment:** The abstract is partially readable.
The body of the paper is not yet legible. This is the correct
starting position.

**Next milestone to unlock:** Completing the sequences and metric
spaces chapters will make the topology prerequisites visible. Opening
Shifrin will make the Riemannian geometry prerequisites visible.

---

### Entry 1 — (add date)

**Curriculum stage:** *(fill in)*

**What is now clear that was opaque before:**

*(fill in)*

**What remains opaque:**

*(fill in)*

**Specific lines in the paper now parseable:**

*(fill in — quote the line, explain why it is now readable)*

---

### Entry 2 — (add date)

*(continue the pattern)*

---

## Capstone Completion Criteria

The capstone is complete when the following are all true:

1. Every object in the Mathematical Objects table has a corresponding
   formal definition in the notes with a valid pointer.
2. Every theorem in the Theorems table has a corresponding proof file
   with a valid pointer.
3. The reading log has an entry that can parse every line of the
   Comptes Rendus paper without gaps.
4. The full Coulibaly-Pasquier paper (arXiv:0901.1999) can be read
   with all major arguments followed, all lemmas verified, and all
   notation translated into house notation.
5. A final reading log entry states, for each result in the paper,
   why it is true — not just what it says.

---

## Connection to the Simulation Goal

The $g(t)$-Brownian motion constructed in this paper is the stochastic
process that lives on the undulating torus $(\mathbb{T}^2, g(t))$ in
the stretch goal simulation. Specifically:

- The time-varying metric $g(t)$ encodes the deformation of the torus.
- The process $X_t$ is the Brownian particle being chased.
- The heat equation on $(M, g(t))$ governs the probability density
  of $X_t$ — this is the Fokker-Planck equation for the simulation.
- The Bismut formula gives a probabilistic way to compute spatial
  gradients of the heat semigroup — relevant to the pursuer's
  gradient-following strategy.
- The gradient control formula under Ricci flow has an analogue
  for general deformations, which controls how quickly particle
  densities spread on the undulating surface — this feeds into
  Lipschitz stability bounds for the GPU integrator.

The DDE component — the pursuer reading the evader's position at
time $t - \tau$ on the earlier geometry $g_{t-\tau}$ — is not covered
by this paper. That is the genuinely novel part of the stretch goal,
sitting beyond the current literature.

---

*This file is a living document. Update the reading log entries and
note pointers as the curriculum progresses. The day every pointer
resolves to a completed definition or proof, and the reading log
entry for the final pass contains no remaining opaque lines,
is the day the capstone is complete.*