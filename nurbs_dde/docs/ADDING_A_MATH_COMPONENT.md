# Adding a Math Component to NDDE Engine

**Audience:** You — a solo learner who understands the engine architecture and
wants to add a new renderable mathematical object.

**Last updated:** 2026-04-13

---

## 0. The Coordinate Subsystem

Before adding anything, understand `app/Viewport.hpp`. It is the **single
source of truth** for all coordinate transforms.

```
Viewport members:
  pan_x, pan_y       — world-space view centre
  zoom               — visible half-height = base_extent / zoom
  yaw, pitch         — 3D orbit angles (radians)
  fb_w, fb_h         — GPU framebuffer pixels   (for MVP aspect ratio)
  dp_w, dp_h         — ImGui logical pixels     (for mouse input)

Viewport methods:
  left/right/top/bottom()    — 2D world bounds of visible area
  ortho_mvp()                — 2D orthographic MVP
  perspective_mvp()          — 3D orbit camera MVP
  pixel_to_world(lx, ly)     — logical pixels → world space
  world_to_pixel(wx, wy)     — world space → logical pixels
  zoom_toward(lx, ly, t)     — zoom toward a logical-pixel point
  pan_by_pixels(dx, dy)      — pan by a logical-pixel delta
  orbit(dx, dy)              — 3D yaw/pitch update
  mouse_valid(lx, ly)        — rejects ImGui sentinel (-FLT_MAX)
```

**Two pixel spaces that must never be confused:**

| Space | Source | Used for |
|---|---|---|
| Framebuffer pixels | `EngineAPI::viewport_size()` → `fb_w/h` | MVP aspect ratio, Vulkan viewport |
| Logical pixels | `ImGui::GetIO().DisplaySize` → `dp_w/h` | MousePos, all input events |

On a 1:1 display these are equal. On HiDPI / scaled displays they differ.
Confusing them causes cursor/ball offset bugs. All input goes through
`pixel_to_world`, which uses logical pixels.

`Scene::sync_viewport()` is called first every frame and populates both spaces.
Your component never needs to touch pixel coordinates.

---

## 1. Mental Model

```
Your IConic subclass          BufferManager               GPU
─────────────────────         ─────────────────           ───────────────
evaluate(t) -> Vec3    ──▶    acquire(n)           ──▶    vkCmdDraw
tessellate(span, n, col)      writes Vertex[]              renders pixels
```

You write world-space vertices. The Viewport's MVP converts them to clip
space. The GPU renders them. No other coordinate math is needed in your class.

---

## 2. The IConic Interface

All curves inherit from `math::IConic`. The interface includes the full
**Frenet–Serret** differential geometry frame:

```cpp
class IConic {
    // Required:
    virtual Vec3  evaluate(float t)          = 0;  // position p(t)
    virtual float t_min()                    = 0;
    virtual float t_max()                    = 0;

    // Override for accuracy; defaults use central-difference finite differences:
    virtual Vec3  derivative(float t)        const; // p'(t)
    virtual Vec3  second_derivative(float t) const; // p''(t)
    virtual Vec3  third_derivative(float t)  const; // p'''(t)
    virtual float curvature(float t)         const; // κ = |p'×p''| / |p'|³
    virtual float torsion(float t)           const; // τ = (p'×p'')·p''' / |p'×p''|²

    // Non-virtual; computed from the above:
    Vec3 unit_tangent(float t);   // T = p' / |p'|
    Vec3 unit_normal(float t);    // N = (p'' - (p''·T)T) / |...|
    Vec3 unit_binormal(float t);  // B = T × N
};
```

**Always override** `curvature()` and `torsion()` with analytic formulas when
they have closed form — the finite-difference defaults accumulate error at
curve endpoints and near inflection points.

---

## 3. The Five-Step Checklist

| Step | File(s) | What you do |
|------|---------|-------------|
| 1 | `src/math/YourCurve.hpp` | Declare the class |
| 2 | `src/math/Conics.cpp` (or a new `.cpp`) | Implement all virtual methods |
| 3 | `src/CMakeLists.txt` | Add `.cpp` to `ndde_math` (if new file) |
| 4 | `src/app/Scene.hpp` | Add `ConicEntry` type id, params, snap cache |
| 5 | `src/app/Scene.cpp` | Wire `rebuild()`, geometry submission, panel sliders |

Never touch: `engine/`, `platform/`, `renderer/`, `memory/`, `app/Viewport.hpp`.

---

## 4. Types Reference

```cpp
struct Vertex { Vec3 pos; Vec4 color; };

enum class Topology {
    LineList,      // vertex pairs — axes, grids, interval lines, Frenet arrows
    LineStrip,     // contiguous chain — curves, epsilon ball, osculating circle
    TriangleList,  // triangles — surfaces (future)
};

enum class DrawMode {
    VertexColor,   // use Vertex::color per vertex
    UniformColor,  // use the Vec4 passed to submit()
};
```

### Submission pattern

```cpp
// 2D curve:
const u32 n     = my_curve->vertex_count(tessellation);
auto      slice = m_api.acquire(n);
my_curve->tessellate({ slice.vertices(), n }, tessellation, color);
m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor,
             color, m_vp.ortho_mvp());

// 3D curve:
m_api.submit(slice, Topology::LineStrip, DrawMode::VertexColor,
             color, m_vp.perspective_mvp());
```

Always pass the MVP from `m_vp`. Never build your own.

---

## 5. Worked Example A — 2D Parabola (reference)

The `Parabola` class in `math/Conics.hpp` is the canonical example:
```
p(t) = (t,  a*t² + b*t + c,  0)    t ∈ [t_min, t_max]
```
- Snap cache: `vector<pair<float,float>>` stored left→right in x
- Auto-extended each frame to cover the visible viewport x-range
- κ is analytic: `|2a| / (1 + (2at+b)²)^(3/2)`

---

## 6. Worked Example B — 3D Helix (reference)

The `Helix` class is the canonical 3D example:
```
p(t) = (r·cos t,  r·sin t,  b·t)    where b = pitch / 2π
```

Key implementation decisions:
- **Snap cache is 3D:** `vector<float>` with x,y,z interleaved — `snap_cache3d`
- **Snap search is screen-projected:** uses `perspective_mvp()` to project each
  cache point and measures screen-pixel distance (not world distance)
- **All Frenet quantities are analytic and constant:**
  ```
  κ = r / (r² + b²)     — curvature, constant for all t
  τ = b / (r² + b²)     — torsion, constant for all t
  ```
  This demonstrates the Locus Theorem directly in the Analysis Readout panel.
- **Helix is disabled by default** — only visible when 3D mode is active.

To add a new 3D curve modelled on the Helix:
1. Implement `evaluate()` returning a non-zero z component
2. Add a `snap_cache3d` path in `ConicEntry::rebuild()` (type id ≥ 2)
3. Add a search loop in `update_hover_3d()` projecting via `perspective_mvp()`
4. Override `curvature()` and `torsion()` analytically if possible

---

## 7. Worked Example C — 2D Line (minimal new class)

```cpp
// math/Line2D.hpp
class Line2D final : public IConic {
public:
    explicit Line2D(float x0=-1.f, float y0=0.f,
                    float dx=1.f,  float dy=0.f,
                    float tmin=-1.f, float tmax=1.f);
    [[nodiscard]] Vec3  evaluate(float t)         const override;
    [[nodiscard]] Vec3  derivative(float t)        const override;
    [[nodiscard]] Vec3  second_derivative(float t) const override;
    [[nodiscard]] Vec3  third_derivative(float t)  const override;
    [[nodiscard]] float curvature(float)            const override { return 0.f; }
    [[nodiscard]] float torsion(float)              const override { return 0.f; }
    [[nodiscard]] float t_min() const override { return m_tmin; }
    [[nodiscard]] float t_max() const override { return m_tmax; }
private:
    float m_x0, m_y0, m_dx, m_dy, m_tmin, m_tmax;
};

// Implementation — derivative is constant:
Vec3 Line2D::evaluate(float t)        const { return {m_x0+m_dx*t, m_y0+m_dy*t, 0.f}; }
Vec3 Line2D::derivative(float)         const { return {m_dx, m_dy, 0.f}; }
Vec3 Line2D::second_derivative(float)  const { return {0.f, 0.f, 0.f}; }
Vec3 Line2D::third_derivative(float)   const { return {0.f, 0.f, 0.f}; }
```

A line has κ = 0 and τ = 0. The osculating circle is suppressed for κ < 1e-8.
The Frenet N and B vectors are degenerate for a straight line — `unit_normal()`
returns an arbitrary perpendicular. This is correct behaviour.

---

## 8. Topology Reference

| Topology | GPU reads vertices as | Use for |
|---|---|---|
| `LineList` | pairs `(v0,v1), (v2,v3), ...` | Axes, grids, Frenet arrows, interval lines |
| `LineStrip` | chain `v0→v1→v2→...` | Curves, epsilon ball/sphere, osculating circle |
| `TriangleList` | triples `(v0,v1,v2), ...` | Surfaces |

The two-branch hyperbola uses `LineStrip` with a NaN sentinel vertex between
the branches (`Vec3{NaN, NaN, NaN}`, alpha=0). The fragment shader discards
degenerate triangles, cleanly breaking the strip.

---

## 9. HoverResult Integration

If you want your curve to participate in the interactive snap and Frenet frame
display, `update_hover()` (or `update_hover_3d()`) will automatically call
`unit_tangent()`, `unit_normal()`, `unit_binormal()`, `curvature()`, and
`torsion()` via the `IConic` interface — no extra wiring needed. The results
appear in `HoverResult` and flow automatically to:

- Frenet arrow display (`submit_frenet_frame()`)
- Osculating circle (`submit_osc_circle()`)
- Analysis Readout panel (κ, τ, speed, R, T, N, B)

---

## 10. Coordinate Rules (summary)

1. Write vertices in **world space**. The MVP handles the rest.
2. Pass `m_vp.ortho_mvp()` for 2D, `m_vp.perspective_mvp()` for 3D.
3. Never call `pixel_to_world` or any MVP computation outside `Scene` and `Viewport`.
4. For world bounds (extend a line to screen edge), use `m_vp.left/right/top/bottom()`.
5. For cursor position, use `m_hover.world_x/y/z` — already computed by `update_hover()`.
6. For 3D snap, add your points to `snap_cache3d` (x,y,z interleaved, float).
7. Always override `curvature()` and `torsion()` analytically if a closed form exists.

---

## 11. Files Modified Per Component

```
New 2D curve (IConic):
  src/math/Conics.hpp/.cpp     ← add class declaration + implementation
  src/app/Scene.hpp/.cpp       ← ConicEntry type id, snap_cache, panel sliders
  src/CMakeLists.txt           ← only if adding a new .cpp file

New 3D curve (IConic):
  same as above, plus:
  snap_cache3d (float, x/y/z interleaved) in ConicEntry::rebuild()
  projection search in Scene::update_hover_3d()

New surface (standalone, not IConic):
  src/math/MySurface.hpp/.cpp  ← new class
  src/CMakeLists.txt           ← add to ndde_math
  src/app/Scene.hpp/.cpp       ← field, wiring, Topology::TriangleList submission
```

**Never touch:** `engine/`, `platform/`, `renderer/`, `memory/`, `app/Viewport.hpp`.
