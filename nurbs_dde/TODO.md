# Adding New Conics to nurbs_dde

This document explains the full set of changes required to add a new conic
section — ellipse, hyperbola, circle, or general conic — to the renderer.
A worked example for a hyperbola is given at the end.

---

## Architecture overview

The renderer has a clean separation of concerns that makes adding curves
straightforward:

```
geometry/
    Curve.hpp/.cpp          ← abstract base: evaluate(t), t_min(), t_max()
    Parabola.hpp/.cpp       ← concrete: y = at² + bt + c

app/
    Application.hpp/.cpp    ← owns the active curve, ParabolaParams, build_ui()

renderer/
    Renderer.hpp/.cpp       ← upload_curve(), draw_frame()
```

The renderer never knows what curve it is drawing — it just consumes a
`std::span<const math::Vec4>` vertex buffer.  Adding a new curve means:

1. Write a new `Curve` subclass in `geometry/`
2. Add a params struct and imgui panel in `Application`
3. Hook the active curve selection into `init_curve()` and `rebuild_curve()`
4. Register the new source file in `src/CMakeLists.txt`

---

## Step 1 — Write the geometry class

Create `src/geometry/Hyperbola.hpp`:

```cpp
#pragma once
// geometry/Hyperbola.hpp
// p(t) = (a·sec(t),  b·tan(t), 0)  — standard hyperbola x²/a² - y²/b² = 1
// Two branches: right branch t ∈ (-π/2+ε, π/2-ε),
//               left  branch t ∈ ( π/2+ε, 3π/2-ε)

#include "Curve.hpp"

namespace ndde::geometry {

enum class HyperbolaBranch { Right, Left, Both };

class Hyperbola final : public Curve {
public:
    explicit Hyperbola(
        float a      = 1.f,
        float b      = 1.f,
        float t_half = 1.3f,          // parameter half-range (< π/2 ≈ 1.5708)
        HyperbolaBranch branch = HyperbolaBranch::Both
    );

    [[nodiscard]] Vec3  evaluate(float t) const override;
    [[nodiscard]] float t_min()           const override { return m_tmin; }
    [[nodiscard]] float t_max()           const override { return m_tmax; }

    float a()      const { return m_a; }
    float b()      const { return m_b; }

private:
    float           m_a, m_b;
    float           m_tmin, m_tmax;
    HyperbolaBranch m_branch;
};

} // namespace ndde::geometry
```

Create `src/geometry/Hyperbola.cpp`:

```cpp
#include "Hyperbola.hpp"
#include <cmath>
#include <stdexcept>

namespace ndde::geometry {

Hyperbola::Hyperbola(float a, float b, float t_half, HyperbolaBranch branch)
    : m_a(a), m_b(b), m_branch(branch)
{
    // sec(t) = 1/cos(t) blows up at t = ±π/2.
    // Clamp the half-range so we never get there.
    constexpr float pi_half = 1.5707963f;
    if (t_half >= pi_half)
        throw std::invalid_argument("Hyperbola: t_half must be < π/2");

    m_tmin = -t_half;
    m_tmax =  t_half;
}

Vec3 Hyperbola::evaluate(float t) const {
    // Right branch: x = a·sec(t),  y = b·tan(t)
    // Left  branch: x = -a·sec(t), y = b·tan(t)
    float sec_t = 1.f / std::cos(t);
    float tan_t = std::tan(t);

    float x = m_a * sec_t;
    if (m_branch == HyperbolaBranch::Left) x = -x;

    return Vec3{ x, m_b * tan_t, 0.f };
}

} // namespace ndde::geometry
```

**Note on "Both" branches:** The `Both` case requires two separate draw calls
or a buffer with a primitive restart index.  The simplest approach is to
tessellate right and left into separate buffers and call `upload_curve` twice
with two different `GpuBuffer` slots — or just expose a toggle in the UI.
For a first implementation, start with a single branch.

---

## Step 2 — Register in CMakeLists

In `src/CMakeLists.txt`, add the new source to `APP_SOURCES`:

```cmake
set(APP_SOURCES
    ...
    geometry/Hyperbola.cpp      # ← add this
    ...
)
```

Also add it to `nurbs_dde_tests` in `tests/CMakeLists.txt` if you write tests.

---

## Step 3 — Add a params struct in Application.hpp

Each conic gets its own params struct, mirroring `ParabolaParams`:

```cpp
// In Application.hpp

struct HyperbolaParams {
    float a      = 1.f;
    float b      = 1.f;
    float t_half = 1.2f;   // < π/2
    int   steps  = 512;
    int   branch = 0;      // 0 = right, 1 = left

    bool operator==(const HyperbolaParams&) const = default;
};
```

Add a curve-type selector enum:

```cpp
enum class CurveType { Parabola, Ellipse, Hyperbola, Circle };
```

And add the new state to the `Application` class body:

```cpp
// In Application (private section)
CurveType        m_curve_type    = CurveType::Parabola;
CurveType        m_last_type     = CurveType::Parabola;

ParabolaParams   m_parabola;
HyperbolaParams  m_hyperbola;
// add EllipseParams m_ellipse; etc. as you implement them
```

Remove the old `m_params` / `m_last_params` members — they become
type-specific fields.

---

## Step 4 — Add the imgui panel

In `Application.cpp`, `build_ui()` becomes a tabbed or combo-driven panel.
The cleanest approach is a combo at the top that switches which sub-panel
is visible:

```cpp
void Application::build_ui() {
    ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(480.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Curve editor");

    // ── Curve selector ────────────────────────────────────────────────────────
    const char* curve_names[] = { "Parabola", "Ellipse", "Hyperbola", "Circle" };
    int current = static_cast<int>(m_curve_type);
    if (ImGui::Combo("Curve type", &current, curve_names, 4))
        m_curve_type = static_cast<CurveType>(current);

    ImGui::Separator();

    // ── Per-curve parameters ──────────────────────────────────────────────────
    switch (m_curve_type) {

    case CurveType::Parabola:
        ImGui::SeparatorText("Coefficients");
        ImGui::SliderFloat("a##p", &m_parabola.a, -3.f, 3.f, "%.2f");
        ImGui::SliderFloat("b##p", &m_parabola.b, -3.f, 3.f, "%.2f");
        ImGui::SliderFloat("c##p", &m_parabola.c, -2.f, 2.f, "%.2f");
        ImGui::SeparatorText("Parameter range");
        ImGui::SliderFloat("t min##p", &m_parabola.t_min, -3.f, 0.f, "%.2f");
        ImGui::SliderFloat("t max##p", &m_parabola.t_max,  0.f, 3.f, "%.2f");
        ImGui::SeparatorText("Tessellation");
        ImGui::SliderInt("steps##p", &m_parabola.steps, 32, 2048);
        if (ImGui::Button("Reset##p", ImVec2(-1.f, 0.f)))
            m_parabola = ParabolaParams{};
        ImGui::Text("y = %.2ft\xc2\xb2 + %.2ft + %.2f",
                    m_parabola.a, m_parabola.b, m_parabola.c);
        break;

    case CurveType::Hyperbola:
        ImGui::SeparatorText("Semi-axes");
        ImGui::SliderFloat("a##h", &m_hyperbola.a,      0.1f, 4.f, "%.2f");
        ImGui::SliderFloat("b##h", &m_hyperbola.b,      0.1f, 4.f, "%.2f");
        ImGui::SeparatorText("Parameter range");
        ImGui::SliderFloat("t half##h", &m_hyperbola.t_half, 0.1f, 1.55f, "%.3f");
        ImGui::SeparatorText("Branch");
        ImGui::RadioButton("Right##h", &m_hyperbola.branch, 0); ImGui::SameLine();
        ImGui::RadioButton("Left##h",  &m_hyperbola.branch, 1);
        ImGui::SeparatorText("Tessellation");
        ImGui::SliderInt("steps##h", &m_hyperbola.steps, 32, 2048);
        if (ImGui::Button("Reset##h", ImVec2(-1.f, 0.f)))
            m_hyperbola = HyperbolaParams{};
        ImGui::Text("x\xc2\xb2/%.2f\xc2\xb2 - y\xc2\xb2/%.2f\xc2\xb2 = 1",
                    m_hyperbola.a, m_hyperbola.b);
        break;

    // case CurveType::Ellipse: ...
    // case CurveType::Circle:  ...

    default:
        ImGui::TextDisabled("(not yet implemented)");
        break;
    }

    // ── Viewport ──────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Viewport");
    ImGui::Text("Pan: (%.2f, %.2f)   Zoom: %.2fx",
                m_view.pan_x, m_view.pan_y, m_view.zoom);
    if (ImGui::Button("Reset view", ImVec2(-1.f, 0.f)))
        m_view = ViewportState{};
    ImGui::TextDisabled("Middle-drag or Alt+drag to pan");
    ImGui::TextDisabled("Scroll wheel to zoom");

    ImGui::End();
}
```

**imgui ID disambiguation:** Notice `"a##p"` vs `"a##h"` — the `##suffix`
prevents imgui from confusing sliders with the same label across panels.

---

## Step 5 — Update rebuild_curve()

`rebuild_curve()` now switches on `m_curve_type`:

```cpp
void Application::rebuild_curve() {
    switch (m_curve_type) {

    case CurveType::Parabola: {
        if (m_parabola.t_min >= m_parabola.t_max) return;
        m_curve = std::make_unique<geometry::Parabola>(
            m_parabola.a, m_parabola.b, m_parabola.c,
            m_parabola.t_min, m_parabola.t_max);
        break;
    }

    case CurveType::Hyperbola: {
        auto branch = m_hyperbola.branch == 0
            ? geometry::HyperbolaBranch::Right
            : geometry::HyperbolaBranch::Left;
        m_curve = std::make_unique<geometry::Hyperbola>(
            m_hyperbola.a, m_hyperbola.b,
            m_hyperbola.t_half, branch);
        break;
    }

    default:
        return;
    }

    int steps = (m_curve_type == CurveType::Parabola)
        ? m_parabola.steps : m_hyperbola.steps;
    auto verts = m_curve->vertex_buffer(steps);
    m_renderer.upload_curve(m_ctx, verts);
}
```

---

## Step 6 — Update the dirty check in main_loop()

The dirty check needs to detect both a curve type change and a parameter
change within the active type:

```cpp
// In main_loop(), replace the old params dirty check with:
bool needs_rebuild = (m_curve_type != m_last_type);

switch (m_curve_type) {
case CurveType::Parabola:
    needs_rebuild |= (m_parabola != m_last_parabola);
    break;
case CurveType::Hyperbola:
    needs_rebuild |= (m_hyperbola != m_last_hyperbola);
    break;
default: break;
}

if (needs_rebuild) {
    rebuild_curve();
    m_last_type      = m_curve_type;
    m_last_parabola  = m_parabola;
    m_last_hyperbola = m_hyperbola;
}
```

---

## Checklist for each new conic

| Task | File(s) |
|---|---|
| Write `evaluate(t)` and parameter constructor | `geometry/MyConic.hpp/.cpp` |
| Add to `APP_SOURCES` | `src/CMakeLists.txt` |
| Add `MyConicParams` struct with `operator==` | `Application.hpp` |
| Add to `CurveType` enum | `Application.hpp` |
| Add `m_my_conic` and `m_last_my_conic` fields | `Application.hpp` |
| Add imgui sub-panel in `build_ui()` switch | `Application.cpp` |
| Add case in `rebuild_curve()` switch | `Application.cpp` |
| Extend dirty check in `main_loop()` | `Application.cpp` |
| Add GTest file | `tests/test_my_conic.cpp` |

---

## Standard parametrisations for reference

| Conic | Parametric form | Parameter range |
|---|---|---|
| Parabola | `(t, at²+bt+c, 0)` | `[t_min, t_max]` |
| Circle | `(r·cos(t), r·sin(t), 0)` | `[0, 2π]` |
| Ellipse | `(a·cos(t), b·sin(t), 0)` | `[0, 2π]` |
| Hyperbola (right) | `(a·sec(t), b·tan(t), 0)` | `(-π/2+ε, π/2-ε)` |
| Hyperbola (left) | `(-a·sec(t), b·tan(t), 0)` | `(-π/2+ε, π/2-ε)` |
| Parabola (focus form) | `(t²/4p, t, 0)` | `[t_min, t_max]` |

All of these map cleanly to the `Curve` interface: implement `evaluate(t)`
and specify `t_min()` / `t_max()`.  The base class `vertex_buffer(n)` does
the tessellation automatically.

---

## Notes on the w-channel

Your vertex buffer uses `Vec4` with `w = 1`.  When you eventually add NURBS,
rational weights live in `w` and the projection divides x, y, z by w in the
vertex shader.  For the standard conics above, `w = 1` is always correct.
For a rational Bézier circle (the exact parametrisation using weights), you
would set `w = cos(π/4)` at the middle control point — but that is a NURBS
topic, not a conic topic.

---

## Future: multiple simultaneous curves

The current architecture uploads one curve buffer.  When you want to show,
say, an ellipse and its tangent line simultaneously, the natural extension is:

- `Renderer` grows a `std::vector<GpuBuffer>` of curve slots
- `upload_curve(ctx, verts, slot_id)` addresses a specific slot
- `draw_frame` iterates the slots

This is a renderer change, not a geometry change, and is straightforward to
add when the need arises.