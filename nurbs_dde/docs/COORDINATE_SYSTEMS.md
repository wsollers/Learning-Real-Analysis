# Coordinate Systems — NDDE Engine

**Repository:** `F:\repos\Learning-Real-Analysis\nurbs_dde`
**Last updated:** 2026-04-12

---

## Overview

The engine operates in three distinct coordinate spaces that are
transformed through a strict pipeline. Confusing any two of these is the
root cause of every cursor/snap/rendering offset bug documented in this
project. This document is the single authoritative reference.

```
  Input device         ImGui/OS           GPU              Analysis
  ───────────          ────────           ───              ────────
  Physical px   ──▶   Logical px   ──▶  World   ──▶   Snap / Geometry
  (hardware)     DPI   (MousePos)  MVP   (R^2)   cache    (Curves)
```

---

## The Three Spaces

### 1. Framebuffer Pixels (GPU)

- **Source:** `EngineAPI::viewport_size()` → `m_vp.fb_w / fb_h`
- **Used for:** Vulkan viewport dimensions, MVP aspect ratio
- **Convention:** origin at top-left, X right, Y **down** (Vulkan default),
  but corrected to Y-up by the negative-height viewport trick (see below)

### 2. Logical / Display Pixels (ImGui / OS)

- **Source:** `ImGui::GetIO().DisplaySize` → `m_vp.dp_w / dp_h`
- **Same space as:** `ImGui::GetIO().MousePos`, all drag deltas, all UI input
- **Used for:** `pixel_to_world()`, `world_to_pixel()`, pan/zoom input
- **Sentinel:** when the mouse is outside the window, ImGui sets `MousePos`
  to `(-FLT_MAX, -FLT_MAX)`. Guard with `Viewport::mouse_valid(lx, ly)`
  before calling any coordinate function.

> **On a standard 100% DPI display:** `fb_w == dp_w` and `fb_h == dp_h`.
> **On HiDPI / Windows scaled displays:** `fb_w > dp_w` by the DPI scale
> factor, but the aspect ratios are identical. The key invariant is that
> `(lx / dp_w)` gives the same fractional position as `(fb_px / fb_w)`.

### 3. World Space (R²)

- **Origin:** `(pan_x, pan_y)` is the world point at the screen centre
- **X-axis:** right is positive
- **Y-axis:** up is positive (right-handed)
- **Bounds:** computed from `zoom` and `base_extent`:
  ```
  half_h  = base_extent / zoom
  half_w  = half_h * (fb_w / fb_h)
  left    = pan_x - half_w       right  = pan_x + half_w
  bottom  = pan_y - half_h       top    = pan_y + half_h
  ```
- All curve geometry, snap caches, epsilon balls, and interval lines live
  in world space.

---

## The Single Source of Truth: `app/Viewport.hpp`

`struct Viewport` owns all coordinate math. **No other file may contain
coordinate arithmetic.** Scene holds one `Viewport m_vp`. Every frame,
`Scene::sync_viewport()` is called first to populate both pixel spaces.

```cpp
// Called once per frame, before any input processing
void Scene::sync_viewport() {
    const Vec2 fb = m_api.viewport_size();
    m_vp.fb_w = fb.x;  m_vp.fb_h = fb.y;          // GPU pixels

    const ImGuiIO& io = ImGui::GetIO();
    m_vp.dp_w = io.DisplaySize.x;                   // logical pixels
    m_vp.dp_h = io.DisplaySize.y;
}
```

### Key transforms provided by `Viewport`

| Function | Input | Output | Notes |
|---|---|---|---|
| `pixel_to_world(lx, ly)` | logical px | world | guards mouse_valid |
| `world_to_pixel(wx, wy)` | world | logical px | inverse of above |
| `ortho_mvp()` | — | Mat4 | uses fb_aspect for correct AR |
| `perspective_mvp()` | — | Mat4 | spherical orbit camera |
| `zoom_toward(lx, ly, t)` | logical px, ticks | — | anchor-zoom |
| `pan_by_pixels(dpx, dpy)` | logical px delta | — | 2D pan |
| `orbit(dpx, dpy)` | logical px delta | — | 3D orbit |

### `pixel_to_world` derivation

```
nx = lx / dp_w                         // fraction [0,1] across display
ny = ly / dp_h                         // fraction [0,1] down display
wx = left()  + nx * (right()  - left())
wy = top()   + ny * (bottom() - top()) // pixel Y down → world Y up
```

`bottom() - top()` is **negative** because `top > bottom`, so increasing
`ny` (cursor moving down) gives decreasing `wy` (world Y goes down). This
is the correct Y-flip.

### `world_to_pixel` derivation (exact inverse)

```
px = ((wx - left())  / (right()  - left())) * dp_w
py = ((wy - top())   / (bottom() - top()))  * dp_h
```

Round-trip error = `pixel_to_world(world_to_pixel(w)) - w` = 0 exactly
(verified by `CoordDebugPanel`).

---

## The Vulkan Y-Flip (Critical)

Vulkan's default NDC convention has **Y increasing downward** — the
opposite of OpenGL and `glm::ortho`. This means that without correction,
`glm::ortho(L, R, B, T)` maps world Y=+T to screen bottom and world
Y=B to screen top. The entire world is vertically flipped.

**Fix:** The negative-height `VkViewport` trick (in `Renderer.cpp`):

```cpp
// Renderer::begin_frame()
VkViewport viewport{
    .y      =  h,   // start at pixel row h (screen bottom)
    .height = -h,   // negative: rasterise upward, flipping Y
};
```

This makes Vulkan's rasterisation match `glm::ortho`:
```
NDC Y = +1  →  screen top     (world T)
NDC Y = -1  →  screen bottom  (world B)
```

After this fix, world +Y is at the top of the screen. `pixel_to_world`
mapping (pixel top → world top) is now consistent with what renders. This
fix is what resolved the epsilon-ball tracking inversion.

**Without this fix:** world point `(cx, cy)` renders at screen position
`(px, dp_h - py)` — vertically mirrored. Snap finds the correct world
point but the ball renders at the mirror position.

---

## Coordinate Flow Per Feature

### Mouse → Snap

```
ImGui::GetIO().MousePos (logical px)
  │
  ▼  Viewport::pixel_to_world()
World point (wx, wy)
  │
  ▼  Distance search over snap_cache[]
Nearest cache point (best_x, best_y) within snap_r_world
  │
  ▼  HoverResult::world_x, world_y
```

`snap_r_world` is computed from the screen-pixel snap threshold:
```cpp
const float world_per_px = (m_vp.top() - m_vp.bottom()) / m_vp.dp_h;
const float snap_r_world = m_analysis_panel.get_snap_px_radius() * world_per_px;
```

This keeps the snap threshold constant in screen pixels regardless of
zoom level or display resolution.

### World → Screen (Geometry Rendering)

```
Vertex (world space, z=0)
  │
  ▼  Viewport::ortho_mvp()  (push constant to shader)
NDC space  [-1, +1]²
  │
  ▼  VkViewport (negative height, y=h)
Screen pixels
```

All geometry submissions pass `m_vp.ortho_mvp()` as the MVP. No geometry
function computes its own MVP.

### World → Screen (ImGui Label Overlay)

```
World point (wx, wy)
  │
  ▼  Viewport::world_to_pixel()
Logical pixels (lx, ly)
  │
  ▼  ImGui::SetNextWindowPos(lx + offset, ly + offset)
Label rendered in ImGui draw list
```

ImGui renders in logical pixel space, which matches `dp_w/dp_h`.

### Epsilon Ball

The epsilon ball is a circle rendered in **world space** at
`(m_hover.world_x, m_hover.world_y)` with radius
`m_analysis_panel.get_epsilon_ball_radius()` (world units). The radius
is mathematically meaningful — it is the ε neighbourhood in the
definition of continuity.

It is **not** the same as the snap threshold (screen pixels).

---

## Two Radii That Must Not Be Confused

| Name | Space | Source | Purpose |
|---|---|---|---|
| `snap_px_radius` | screen pixels | `AnalysisPanel` slider | How close the cursor must be to a curve to snap (UI interaction) |
| `epsilon_ball_radius` | world units | `AnalysisPanel` slider | Size of the rendered ε neighbourhood (mathematical concept) |

At zoom=1 on a 3840-wide display, `0.1` world units ≈ 43 screen pixels.
A 0.1 snap radius would snap from 43px away — too large. The pixel
threshold decouples the mathematical ε from the UI interaction distance.

---

## Snap Cache Layout

Each `ConicEntry` maintains a `snap_cache: vector<pair<float,float>>` of
`(x, y)` world-space points sampled uniformly along the curve. The layout
matters for the secant walk:

| Curve type | Cache layout | Direction |
|---|---|---|
| Parabola (single branch) | `[0, n]` uniform in t | x increases |
| Hyperbola right branch | `[0, tessellation]` | t: -range → +range, y: bottom→top |
| Hyperbola left branch | `[tessellation+1, 2*tessellation+1]` | t: -range → +range, y: bottom→top |

**Both hyperbola branches are stored in the same t-direction** (bottom to
top in world Y). This ensures the secant walk (`li--`, `ri++`) traverses
the curve consistently for both branches.

The secant walk is bounded to `[branch_start, branch_end]` to prevent
crossing from the right branch into the left branch mid-walk.

---

## Component Responsibility Summary

| Component | Coordinate responsibility |
|---|---|
| `Viewport` | Single source of truth. All transforms live here. |
| `Scene::sync_viewport()` | Populates `m_vp.fb_w/h` and `m_vp.dp_w/h` each frame |
| `Scene::update_camera()` | Calls `m_vp.zoom_toward / pan_by_pixels / orbit` |
| `Scene::update_hover()` | Calls `m_vp.pixel_to_world(MousePos)` for snap search |
| `Scene::submit_*()` | All pass `m_vp.ortho_mvp()` or `perspective_mvp()` |
| `Scene::submit_interval_labels()` | Calls `m_vp.world_to_pixel()` to position ImGui windows |
| `Renderer::begin_frame()` | Sets negative-height `VkViewport` for Y-flip correction |
| `CoordDebugPanel` | Reads all coordinate values; calls `mouse_valid()` before transforms |

---

## Diagnostic: `CoordDebugPanel`

Toggle with **"Coord Debug Window"** in the Scene panel. Shows live +
120-frame latched snapshots of every coordinate value.

**Key checks:**
- `dp matches DisplaySize: OK` — `sync_viewport` is correct
- `vp.fb_w/h vs fb_w/h: OK` — framebuffer size matches
- `round-trip error: 0.000 px` (green) — coordinate math is exact
- `mouse-snap px offset` — should be `<= snap_px_radius` when touching a curve
- `Curve Snap Cache Sizes` — Hyperbola should show `514` (both branches)

**ImGui sentinel detection:** When `MousePos = (-FLT_MAX, -FLT_MAX)` (mouse
outside window), the panel shows `"OUT OF WINDOW"` in red instead of
astronomical garbage numbers.

---

## Historical Bugs and Their Coordinate Root Causes

| Bug | Root cause | Fix |
|---|---|---|
| Epsilon ball offset from cursor | Framebuffer px used instead of logical px in `pixel_to_world` | Use `dp_w/dp_h` (DisplaySize) not `fb_w/fb_h` |
| Ball tracking inverted (up/down) | Vulkan Y down vs OpenGL Y up: `glm::ortho` produces Y-up but Vulkan renders it Y-down | Negative-height `VkViewport` in `Renderer.cpp` |
| Hyperbola left branch snap inverted | Left branch stored in reversed t order → secant walk gives wrong direction | Both branches now stored bottom→top (same t direction) |
| Snap only near origin | Parabola `t_max=1.5` — no cache points beyond x=±1.5 | Auto-extend `par_tmin/tmax` to viewport bounds each frame |
| Hyperbola snaps to wrong branch | Snap cache only contained right branch | Two-branch cache: `[0..n]` right, `[n+1..2n+1]` left |
| ImGui sentinel causing ∞ | `(-FLT_MAX) / dp_w` = `-FLT_MAX`; overflow to NaN | `Viewport::mouse_valid()` guard in all input paths |
| Pan direction inverted | Wrong sign in `pan_by_pixels` | `pan_y -= dpy * wy_per_px` (negative because pixel Y is down) |
