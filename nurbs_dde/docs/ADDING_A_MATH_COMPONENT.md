# Adding a Math Component to NDDE Engine

**Audience:** You -- a solo learner who already understands the engine
architecture and wants to add a new renderable mathematical object.

**Scope:** Everything from the pure math definition through the GPU draw
call, with a worked example for each of the two fundamental cases:
a **2D Line** and a **3D Plane**.

---

## 0. Mental Model First

Before touching a file, understand what happens to your math each frame.

```
Your IConic subclass          BufferManager              GPU
─────────────────────         ─────────────────          ──────────────
evaluate(t) -> Vec3           acquire(n)                 vkCmdDraw
tessellate(span, n, color) -> writes Vertex[] ──────────> renders pixels
```

The invariant that makes this work:

> **The math layer never allocates GPU memory. It only writes into
> memory that the engine has already allocated for it.**

`Scene::on_frame()` calls `m_api.acquire(n)` to get a slice of the
per-frame vertex arena, then passes `slice.vertices()` -- a raw
`Vertex*` into GPU-visible memory -- to your tessellate function.
Your function writes. The engine submits. No copies, no staging buffers.

---

## 1. The Five-Step Checklist

For every new math component you add, you touch exactly these things:

| Step | File(s) | What you do |
|------|---------|-------------|
| 1 | `src/math/YourComponent.hpp` | Define the class, inheriting `IConic` (curves) or standalone (surfaces) |
| 2 | `src/math/YourComponent.cpp` | Implement `evaluate`, `derivative`, `tessellate` |
| 3 | `src/CMakeLists.txt` | Add `.cpp` to `ndde_math` sources |
| 4 | `src/app/Scene.hpp` | Add field and helpers |
| 5 | `src/app/Scene.cpp` | Add submission logic and ImGui panel |

You do **not** touch: any renderer file, any platform file, `Engine.hpp/.cpp`,
`EngineAPI.hpp`, or any shader.

---

## 2. The Types You Work With

Everything you need is in `math/Types.hpp`:

```cpp
// A single vertex -- what you write into the GPU buffer
struct Vertex {
    Vec3 pos;    // position in world space (x, y, z)
    Vec4 color;  // RGBA, each channel [0, 1]
};

// How you tell the renderer which GPU primitive to use
enum class Topology {
    LineList,       // pairs:  v0-v1, v2-v3, ...   (axes, grids)
    LineStrip,      // chain:  v0->v1->v2->...      (curves)
    TriangleList,   // triples: v0-v1-v2, ...       (surfaces)
};
```

The `EngineAPI` in `Scene` gives you:

```cpp
// 1. Get GPU-visible memory for n vertices
auto slice = m_api.acquire(n);

// 2. Write your vertices into slice.vertices()
my_conic.tessellate({ slice.vertices(), n }, n, color);

// 3. Submit
m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor,
             color, mvp_matrix);
```

---

## 3. Worked Example A -- 2D Line

### The Mathematics

A line in R^2 through point (x0, y0) with direction (dx, dy):

    p(t) = (x0 + dx*t,  y0 + dy*t,  0)    t in [t_min, t_max]

Curvature is identically zero. Derivative is constant: p'(t) = (dx, dy, 0).

### Step 1 -- `src/math/Line2D.hpp`

```cpp
#pragma once
#include "math/Conics.hpp"

namespace ndde::math {

class Line2D final : public IConic {
public:
    explicit Line2D(float x0 = -1.f, float y0 = 0.f,
                    float dx =  1.f, float dy = 0.f,
                    float tmin = -1.f, float tmax = 1.f);

    [[nodiscard]] Vec3  evaluate(float t)         const override;
    [[nodiscard]] Vec3  derivative(float t)        const override;
    [[nodiscard]] Vec3  second_derivative(float t) const override;
    [[nodiscard]] Vec3  third_derivative(float t)  const override;
    [[nodiscard]] float curvature(float t)          const override;
    [[nodiscard]] float t_min() const override { return m_tmin; }
    [[nodiscard]] float t_max() const override { return m_tmax; }

private:
    float m_x0, m_y0, m_dx, m_dy, m_tmin, m_tmax;
};

} // namespace ndde::math
```

### Step 2 -- `src/math/Line2D.cpp`

```cpp
#include "math/Line2D.hpp"
#include <stdexcept>
#include <cmath>

namespace ndde::math {

Line2D::Line2D(float x0, float y0, float dx, float dy, float tmin, float tmax)
    : m_x0(x0), m_y0(y0), m_dx(dx), m_dy(dy), m_tmin(tmin), m_tmax(tmax)
{
    if (tmin >= tmax) throw std::invalid_argument("[Line2D] t_min must be < t_max");
}

Vec3 Line2D::evaluate(float t) const { return Vec3{ m_x0 + m_dx*t, m_y0 + m_dy*t, 0.f }; }
Vec3 Line2D::derivative(float)         const { return Vec3{ m_dx, m_dy, 0.f }; }
Vec3 Line2D::second_derivative(float)  const { return Vec3{ 0.f, 0.f, 0.f }; }
Vec3 Line2D::third_derivative(float)   const { return Vec3{ 0.f, 0.f, 0.f }; }
float Line2D::curvature(float)         const { return 0.f; }

} // namespace ndde::math
```

### Step 3 -- `src/CMakeLists.txt`

Add to `add_library(ndde_math STATIC ...)`:
```cmake
    math/Line2D.cpp    # add this
```

### Steps 4 & 5 -- `Scene.hpp` and `Scene.cpp`

In `Scene.hpp` (private section):
```cpp
std::unique_ptr<math::Line2D> m_line;
void add_line();
void submit_line();
```

In `Scene.cpp`:
```cpp
void Scene::add_line() {
    m_line = std::make_unique<math::Line2D>(-1.5f, 0.f, 1.f, 0.f, -1.5f, 1.5f);
}

void Scene::submit_line() {
    if (!m_line) return;
    const u32 n     = 1u;
    const u32 count = m_line->vertex_count(n);
    const Vec4 color = { 1.f, 0.5f, 0.f, 1.f };
    auto slice = m_api.acquire(count);
    m_line->tessellate({ slice.vertices(), count }, n, color);
    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor, color, ortho_mvp());
}
```

Call `add_line()` from `Scene::Scene()` and `submit_line()` from `on_frame()`.

---

## 4. Worked Example B -- 3D Plane

A plane is not a curve. It does not inherit `IConic`. It generates
`TriangleList` geometry directly.

### The Mathematics

Plane through origin with normal n_hat. Parameterised patch:

    p(u, v) = u*e1 + v*e2      (u,v) in [-extent, extent]^2

where e1, e2 are a Gram-Schmidt orthonormal basis perpendicular to n_hat.

### Step 1 -- `src/math/Plane3D.hpp`

```cpp
#pragma once
#include "math/Types.hpp"
#include <span>

namespace ndde::math {

class Plane3D {
public:
    explicit Plane3D(Vec3 normal = {0.f,1.f,0.f}, float extent = 1.f, u32 divisions = 10);

    [[nodiscard]] u32  vertex_count() const noexcept; // 6 * divisions^2
    void tessellate(std::span<Vertex> out, Vec4 color = {0.3f,0.3f,0.8f,0.4f}) const;

    [[nodiscard]] Vec3 normal()    const noexcept { return m_normal; }
    [[nodiscard]] Vec3 evaluate(float u, float v) const noexcept;

private:
    Vec3  m_normal, m_e1, m_e2;
    float m_extent;
    u32   m_divisions;
    void build_basis(Vec3 raw_normal);
};

} // namespace ndde::math
```

### Step 2 -- `src/math/Plane3D.cpp`

```cpp
#include "math/Plane3D.hpp"
#include <stdexcept>
#include <glm/glm.hpp>

namespace ndde::math {

Plane3D::Plane3D(Vec3 normal, float extent, u32 divisions)
    : m_extent(extent), m_divisions(divisions)
{
    if (extent <= 0.f) throw std::invalid_argument("[Plane3D] extent must be positive");
    if (divisions == 0) throw std::invalid_argument("[Plane3D] divisions must be > 0");
    build_basis(normal);
}

void Plane3D::build_basis(Vec3 raw) {
    const float len = glm::length(raw);
    if (len < 1e-8f) throw std::invalid_argument("[Plane3D] normal must be nonzero");
    m_normal = raw / len;
    Vec3 up = (std::abs(m_normal.y) < 0.99f) ? Vec3{0,1,0} : Vec3{1,0,0};
    m_e1 = glm::normalize(up - glm::dot(up, m_normal) * m_normal);
    m_e2 = glm::cross(m_normal, m_e1);
}

Vec3  Plane3D::evaluate(float u, float v) const noexcept { return u*m_e1 + v*m_e2; }
u32   Plane3D::vertex_count() const noexcept { return 6u * m_divisions * m_divisions; }

void Plane3D::tessellate(std::span<Vertex> out, Vec4 color) const {
    if (out.size() < vertex_count()) throw std::invalid_argument("[Plane3D] span too small");
    const float step  = (2.f * m_extent) / static_cast<float>(m_divisions);
    const float start = -m_extent;
    u32 idx = 0;
    for (u32 j = 0; j < m_divisions; ++j) {
        for (u32 i = 0; i < m_divisions; ++i) {
            float u0=start+i*step, u1=u0+step, v0=start+j*step, v1=v0+step;
            Vec3 p00=evaluate(u0,v0), p10=evaluate(u1,v0);
            Vec3 p01=evaluate(u0,v1), p11=evaluate(u1,v1);
            out[idx++]={p00,color}; out[idx++]={p10,color}; out[idx++]={p11,color};
            out[idx++]={p00,color}; out[idx++]={p11,color}; out[idx++]={p01,color};
        }
    }
}

} // namespace ndde::math
```

### Steps 3-5

Same pattern as Example A. Add to CMakeLists, add `unique_ptr<math::Plane3D>`
to Scene, implement `add_plane()` / `submit_plane()`, wire into `on_frame()`.
Use `Topology::TriangleList` when calling `m_api.submit()`.

---

## 5. The General Pattern

```
1. Class definition
   For curves:   inherit IConic, implement evaluate/derivative/tessellate
   For surfaces: standalone, implement vertex_count/tessellate

2. Tessellate contract
   Accept std::span<Vertex> out -- write here, never allocate
   Check out.size() >= vertex_count, throw invalid_argument if not
   Fill out[0..n-1] with { Vec3 pos, Vec4 color }

3. Scene wiring
   Scene owns the object (unique_ptr)
   Scene: acquire -> tessellate -> submit
   Scene adds ImGui panel, marks dirty on parameter change
```

---

## 6. Topology Reference

| Topology | GPU reads vertices as | Use for |
|---|---|---|
| `LineList` | pairs (v0,v1), (v2,v3), ... | Axes, grids |
| `LineStrip` | chain v0->v1->v2->... | Curves, trajectories |
| `TriangleList` | triples (v0,v1,v2), ... | Surfaces |

---

## 7. Thread Safety for Future DDE Components

When you add DDE trajectory integrators, the source of vertex data changes
but the contract does not:

```
Simulation thread                   Render thread
──────────────────                  ─────────────
DDE integrator -> ring buffer  -->  Scene reads -> acquire -> tessellate -> submit
```

The ring buffer is a lock-free SPSC queue of Vec3 with a std::atomic<u32>
write cursor. `tessellate()` still writes into a caller-supplied span --
the span abstraction is intentionally neutral about where source data comes from.

---

## 8. Files Modified Per Component

```
New curve (IConic subclass):
  src/math/MyCurve.hpp/.cpp        <- new
  src/CMakeLists.txt               <- add to ndde_math
  src/app/Scene.hpp/.cpp           <- add field, wiring, panel
  tests/test_my_curve.cpp          <- new (no Vulkan, just GLM + GTest)
  tests/CMakeLists.txt             <- add to ndde_tests

New surface (standalone):
  src/math/MySurface.hpp/.cpp      <- new
  src/CMakeLists.txt               <- add to ndde_math
  src/app/Scene.hpp/.cpp           <- add field, wiring, panel
```

Files you **never** touch when adding math:
`engine/`, `platform/`, `renderer/`, `memory/`.

---

*Last updated: refactor v0.2.0*
