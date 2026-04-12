# Adding a Math Component to NDDE Engine

**Audience:** You — a solo learner who already understands the engine
architecture and wants to add a new renderable mathematical object.

---

## 0. The Coordinate Subsystem

Before adding anything, understand `app/Viewport.hpp`. It is the **single
source of truth** for all coordinate transforms. No coordinate math should
live anywhere else.

```
app/Viewport
  pan_x, pan_y     — world-space view centre
  zoom             — scale: visible_half_height = base_extent / zoom
  fb_w, fb_h       — GPU framebuffer pixels    (for MVP aspect ratio)
  dp_w, dp_h       — ImGui logical pixels      (for mouse input)

  left/right/top/bottom()   — world bounds of the visible area
  ortho_mvp()               — orthographic MVP matrix (uses fb aspect)
  pixel_to_world(lx, ly)    — logical pixels → world space (uses dp size)
  world_to_pixel(wx, wy)    — world space → logical pixels
  zoom_toward(lx, ly, t)    — zoom toward a logical-pixel point
  pan_by_pixels(dx, dy)     — pan by a logical-pixel delta
```

**Two pixel spaces that must never be confused:**

| Space | Source | Used for |
|---|---|---|
| Framebuffer pixels | `EngineAPI::viewport_size()` → `fb_w/h` | MVP aspect ratio, Vulkan viewport |
| Logical pixels | `ImGui::GetIO().DisplaySize` → `dp_w/h` | MousePos, all input events |

On a 1:1 display these are equal. On HiDPI / Windows scaled displays they
differ by the DPI scale factor. Confusing them causes the cursor/ball offset
bug. All input goes through `pixel_to_world`, which uses logical pixels.

`Scene::sync_viewport()` is called first every frame and populates both
spaces. Your new component never needs to touch pixel coordinates.

---

## 1. Mental Model

```
Your IConic subclass          BufferManager              GPU
─────────────────────         ─────────────────          ──────────────
evaluate(t) -> Vec3           acquire(n)                 vkCmdDraw
tessellate(span, n, color) -> writes Vertex[] ──────────> renders pixels
```

The math layer writes world-space vertices. The Viewport's `ortho_mvp()`
converts them to clip space. The GPU renders them. No other coordinate
math is needed.

---

## 2. The Five-Step Checklist

| Step | File(s) | What you do |
|------|---------|-------------|
| 1 | `src/math/YourComponent.hpp` | Define the class |
| 2 | `src/math/YourComponent.cpp` | Implement `evaluate`, `derivative`, `tessellate` |
| 3 | `src/CMakeLists.txt` | Add `.cpp` to `ndde_math` |
| 4 | `src/app/Scene.hpp` | Add field and helpers |
| 5 | `src/app/Scene.cpp` | Wire geometry submission and ImGui panel |

Never touch: `engine/`, `platform/`, `renderer/`, `memory/`, `app/Viewport.hpp`.

---

## 3. Types Reference

```cpp
struct Vertex { Vec3 pos; Vec4 color; };

enum class Topology {
    LineList,       // vertex pairs — axes, grids, interval lines
    LineStrip,      // contiguous chain — curves, epsilon ball
    TriangleList,   // triangles — surfaces
};

enum class DrawMode {
    VertexColor,    // use Vertex::color
    UniformColor,   // use the Vec4 passed to submit()
};
```

In Scene, the full submission pattern is:
```cpp
const u32   n     = my_curve->vertex_count(tessellation);
auto        slice = m_api.acquire(n);
my_curve->tessellate({ slice.vertices(), n }, tessellation, color);
m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor,
             color, m_vp.ortho_mvp());
//                         ^^^^^^^^^^^
//           Always pass m_vp.ortho_mvp() — never build your own MVP
```

---

## 4. Worked Example A — 2D Line

### math/Line2D.hpp
```cpp
#pragma once
#include "math/Conics.hpp"
namespace ndde::math {
class Line2D final : public IConic {
public:
    explicit Line2D(float x0=-1.f, float y0=0.f,
                    float dx=1.f,  float dy=0.f,
                    float tmin=-1.f, float tmax=1.f);
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

### math/Line2D.cpp
```cpp
#include "math/Line2D.hpp"
#include <stdexcept>
namespace ndde::math {
Line2D::Line2D(float x0, float y0, float dx, float dy, float tmin, float tmax)
    : m_x0(x0), m_y0(y0), m_dx(dx), m_dy(dy), m_tmin(tmin), m_tmax(tmax) {
    if (tmin >= tmax) throw std::invalid_argument("[Line2D] t_min must be < t_max");
}
Vec3  Line2D::evaluate(float t) const { return {m_x0+m_dx*t, m_y0+m_dy*t, 0.f}; }
Vec3  Line2D::derivative(float)         const { return {m_dx, m_dy, 0.f}; }
Vec3  Line2D::second_derivative(float)  const { return {0.f, 0.f, 0.f}; }
Vec3  Line2D::third_derivative(float)   const { return {0.f, 0.f, 0.f}; }
float Line2D::curvature(float)          const { return 0.f; }
} // namespace ndde::math
```

### src/CMakeLists.txt
```cmake
add_library(ndde_math STATIC
    math/Conics.cpp
    math/Axes.cpp
    math/Line2D.cpp    # add this
)
```

### Scene wiring
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
    const u32  n   = m_line->vertex_count(2);
    const Vec4 col = { 1.f, 0.5f, 0.f, 1.f };
    auto slice = m_api.acquire(n);
    m_line->tessellate({ slice.vertices(), n }, 2, col);
    m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor,
                 col, m_vp.ortho_mvp());
}
```

---

## 5. Worked Example B — 3D Plane

A surface, not a curve. Standalone class, no `IConic`. Generates `TriangleList`.

### math/Plane3D.hpp
```cpp
#pragma once
#include "math/Types.hpp"
#include <span>
namespace ndde::math {
class Plane3D {
public:
    explicit Plane3D(Vec3 normal={0,1,0}, float extent=1.f, u32 divisions=10);
    [[nodiscard]] u32  vertex_count() const noexcept; // 6 * divisions^2
    void tessellate(std::span<Vertex> out, Vec4 color={0.3f,0.3f,0.8f,0.4f}) const;
    [[nodiscard]] Vec3 evaluate(float u, float v) const noexcept;
private:
    Vec3 m_normal, m_e1, m_e2;
    float m_extent;
    u32   m_divisions;
    void build_basis(Vec3 raw);
};
} // namespace ndde::math
```

Submit with `Topology::TriangleList` and `m_vp.ortho_mvp()` in 2D or a
perspective MVP (computed manually from `m_vp` bounds) in 3D.

---

## 6. Topology Reference

| Topology | GPU reads vertices as | Use for |
|---|---|---|
| `LineList` | pairs `(v0,v1), (v2,v3), ...` | Axes, grids, interval lines |
| `LineStrip` | chain `v0→v1→v2→...` | Curves, epsilon ball |
| `TriangleList` | triples `(v0,v1,v2), ...` | Surfaces |

---

## 7. Coordinate Rules (summary)

1. Write vertices in **world space**. The MVP handles the rest.
2. Pass `m_vp.ortho_mvp()` to every `m_api.submit()` call.
3. Never call `pixel_to_world` or `ortho_mvp` outside of `Scene` and `Viewport`.
4. If you need the visible world bounds (to extend a line to the screen edge),
   use `m_vp.left()`, `m_vp.right()`, `m_vp.top()`, `m_vp.bottom()`.
5. If you need to place something at the cursor, use `m_hover.world_x/y`
   (already computed by `Scene::update_hover()` via `m_vp.pixel_to_world`).

---

## 8. Files Modified Per Component

```
New curve (IConic):
  src/math/MyCurve.hpp/.cpp        <- new
  src/CMakeLists.txt               <- add to ndde_math
  src/app/Scene.hpp/.cpp           <- field, wiring, panel
  tests/test_my_curve.cpp          <- no Vulkan needed

New surface (standalone):
  src/math/MySurface.hpp/.cpp      <- new
  src/CMakeLists.txt               <- add to ndde_math
  src/app/Scene.hpp/.cpp           <- field, wiring, panel
```

Never touch: `engine/`, `platform/`, `renderer/`, `memory/`, `app/Viewport.hpp`.
