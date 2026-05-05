# `nurbs_dde` — Architecture & Code Review

**Reviewer:** Claude (Sonnet 4.6)
**Date:** 2026-05-04
**Scope:** Full source tree (`src/`) — engine, renderer, sim, math, app, memory layers.
**Standard:** C++23, Clang-targeting, Vulkan 1.3 (volk / vk-bootstrap)

---

## Executive Summary

This is an impressively well-structured learning project. The layered library decomposition (`ndde_numeric → ndde_math → ndde_memory → ndde_platform → ndde_renderer → ndde_engine → nurbs_dde`) is clean, the separation of `IEquation` / `IIntegrator` is genuinely correct OOP, and the PMR-backed memory lifetime system is a sophisticated design choice that most commercial projects never bother with. The mathematical documentation inline (Itô SDE derivations, DDE formalism, predicate logic) is exceptional.

The issues below are graded by severity and grouped by theme. Most are correctness concerns or maintainability hazards; very few are outright bugs.

---

## Severity Key

| Symbol | Meaning |
|--------|---------|
| 🔴 **Critical** | Could cause UB, data corruption, or GPU validation errors in current code |
| 🟠 **Major** | Significant design smell, scalability blocker, or latent bug |
| 🟡 **Minor** | Violation of best practice; works now but will hurt you later |
| 🔵 **Suggestion** | Idiomatic improvement or missed modern C++ feature |

---

## 1. Memory & Lifetime Management

### 🔴 `ParticleSystem::bind_memory` uses `std::destroy_at` / `std::construct_at` on live PMR vectors

**File:** `src/app/ParticleSystem.hpp` — `rebind_vector()`

```cpp
std::destroy_at(&vector);
std::construct_at(&vector, std::move(rebound));
```

Calling `std::destroy_at` on a `LifetimeVector` (which is a `std::pmr::vector`) that **still holds elements** invokes destructors twice on the same memory. The elements are moved into `rebound` first, so they are technically in a "valid but unspecified" state — but the PMR allocator's accounting inside the old allocator is not released before `destroy_at` runs; the allocator sees a `deallocate` from a pointer it did not allocate (the new resource). This is **undefined behaviour** if the old and new `memory_resource` objects are not equal.

**Fix:** Use `std::swap` or assign directly:
```cpp
vector = std::pmr::vector<T>(resource);
vector.reserve(old_size);
// move elements in
```
Or simply don't rebind at all — freeze the resource at construction.

---

### 🟠 `SimulationHost` is returned by value from `EngineServices::simulation_host()`

**File:** `src/engine/SimulationHost.hpp` — `EngineServices::simulation_host()`

```cpp
[[nodiscard]] SimulationHost simulation_host() noexcept {
    return SimulationHost(m_panels, m_hotkeys, ...);
}
```

`SimulationHost` is a bag of **references**. Returning it by value is safe only if the caller stores it before the `EngineServices` object moves or is destroyed. The real danger: `Engine::m_simulation_host` is initialised in the constructor's member-initialiser list **before** `m_services` is fully constructed (it's a `EngineServices` value member). The initialisation order in `Engine.hpp` is:

```cpp
AppConfig               m_config;
EngineServices          m_services;          // constructed first (declared first)
SimulationHost          m_simulation_host;   // ... then this
```

This is **fine as declared**, but it is a hidden order dependency. If `m_simulation_host` is ever moved above `m_services` in the declaration list, this silently becomes a dangling-reference bug. Document the required order explicitly with a `static_assert` or a comment block that names the invariant.

---

### 🟠 `HistoryBuffer` grows its `m_records` PMR vector unboundedly until `capacity` is reached, then ring-buffers — but `capacity` is never validated against the chosen PMR arena

**File:** `src/sim/HistoryBuffer.hpp`

The buffer is allocated on a `HistoryVector<Record>` backed by `memory::history().resource()`. The simulation history arena is finite. If `capacity = 4096` and `sizeof(Record) = 12 bytes`, that is 48 KB — trivial. But `DelayInitialValueProblem::m_history_values` (a flat `f64` array) grows **without any cap at all**:

```cpp
void record_history(f64 t, std::span<const f64> state) {
    // Appends state.size() f64s every step — unbounded growth
    m_history_values.push_back(value);
}
```

For a 1-hour run at 120 Hz with a 6-dimensional system that is `1 hour × 120 × 6 × 8 bytes ≈ 20 MB` — which will silently exhaust the history PMR arena and fall back to the default allocator (or crash). Add a configurable cap with oldest-record eviction, matching the `HistoryBuffer` strategy.

---

### 🟡 `AnimatedCurve::next_id()` uses `static std::atomic<ParticleId>` — resets on every run

**File:** `src/app/AnimatedCurve.hpp`

```cpp
[[nodiscard]] static ParticleId next_id() noexcept {
    static std::atomic<ParticleId> id{0};
    return ++id;
}
```

IDs restart from 1 every process launch, so recorded sessions and serialised snapshots cannot be compared across runs. This is fine for a live sim, but if you ever add replay or export, IDs will collide. Consider seeding from a monotonic timestamp or a UUID.

---

### 🟡 `PersistentVector` and friends are all the same `LifetimeVector<T>` alias

**File:** `src/memory/Containers.hpp`

```cpp
template <class T> using FrameVector    = LifetimeVector<T>;
template <class T> using SimVector      = LifetimeVector<T>;
template <class T> using PersistentVector = LifetimeVector<T>;
// etc.
```

All five aliases are identical types. This means:
- A function taking `SimVector<T>&` will silently accept a `FrameVector<T>&`.
- The `LifetimeVector::bind_generation` / `assert_alive` mechanism is the only guard, and it is stripped in Release.

This is a **type-safety illusion**: the aliases communicate intent but provide no compiler-enforced barrier. For a learning project this is acceptable, but it is worth noting that a genuine type-distinct arena system (strong typedefs or separate template specialisations) would catch cross-lifetime usage at compile time rather than only via runtime asserts in Debug.

---

## 2. Vulkan / Renderer Correctness

### 🔴 `transition_image` uses `VK_PIPELINE_STAGE_ALL_COMMANDS_BIT` for every barrier

**File:** `src/renderer/Renderer.cpp` — `Renderer::transition_image()`

```cpp
vkCmdPipelineBarrier(m_cmd,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    ...);
```

`ALL_COMMANDS` as both `srcStageMask` and `dstStageMask` is a **full GPU pipeline stall**. For a single-command-buffer, single-queue renderer this is technically correct (no GPU parallelism is broken), but it is explicitly the pattern Vulkan's validation layers warn about for performance, and it prevents the driver from overlapping any GPU work. For your learning trajectory toward optimised Brownian-motion simulations, this will become a bottleneck.

**Better practice:** Use the tightest pair of stage masks for each transition:

| Transition | srcStage | dstStage |
|-----------|----------|----------|
| UNDEFINED → COLOR_ATTACHMENT | `TOP_OF_PIPE` | `COLOR_ATTACHMENT_OUTPUT` |
| COLOR_ATTACHMENT → TRANSFER_SRC | `COLOR_ATTACHMENT_OUTPUT` | `TRANSFER` |
| TRANSFER_SRC → PRESENT | `TRANSFER` | `BOTTOM_OF_PIPE` |

---

### 🔴 PNG capture path reads back BGRA assuming swapchain format — no format check

**File:** `src/renderer/Renderer.cpp` — `Renderer::end_frame()`

```cpp
rgba[i * 4u + 0u] = bgra[i * 4u + 2u];  // assumes B at offset 0
rgba[i * 4u + 1u] = bgra[i * 4u + 1u];
rgba[i * 4u + 2u] = bgra[i * 4u + 0u];  // assumes R at offset 2
```

The code **hardcodes a BGRA→RGBA swap** without checking `swapchain.format()`. On most NVIDIA/AMD drivers on Windows the surface is `VK_FORMAT_B8G8R8A8_UNORM` or `VK_FORMAT_B8G8R8A8_SRGB`, so this works in practice. On some platforms (Mesa/Intel) the surface format can be `VK_FORMAT_R8G8B8A8_*`, in which case the capture will have red and blue channels swapped.

**Fix:** Query `swapchain.format()` and branch, or use `VK_FORMAT_R8G8B8A8_UNORM` explicitly in the swapchain preferences.

---

### 🟠 Single command buffer / single fence — no CPU-GPU double buffering

**File:** `src/renderer/Renderer.hpp`, `Renderer.cpp`

The renderer uses **one** `VkCommandBuffer` and **one** `VkFence`. Every frame waits for the previous frame to finish (`vkWaitForFences`) before recording the new one. Per-image semaphores are allocated, but the command recording itself cannot overlap with GPU execution. For a pedagogical renderer this is fine, but:

1. The comment `// One semaphore per swapchain image...` implies the intent is to avoid the validation error for semaphore reuse — but with a single command buffer there is no actual pipelining benefit.
2. Moving to a `frames-in-flight = 2` model with two command buffers and two fences would immediately unlock CPU/GPU parallelism with minimal code change.

This is worth noting now so the architecture does not need to be ripped out later.

---

### 🟠 `Renderer::end_frame` allocates `VkBuffer` + `VkDeviceMemory` per capture on the hot path

**File:** `src/renderer/Renderer.cpp` — `end_frame()` capture block

```cpp
if (vkCreateBuffer(m_device, &bi, nullptr, &capture.buffer) != VK_SUCCESS) ...
if (vkAllocateMemory(m_device, &ai, nullptr, &capture.memory) != VK_SUCCESS) ...
```

`vkAllocateMemory` is a **driver-level heap allocation** — the spec recommends fewer than 4096 total allocations per application lifetime, and implementations may enforce this. Although captures are infrequent, allocating in `end_frame` is the wrong place for it: if an exception throws between `vkCreateBuffer` and `vkAllocateMemory`, the buffer leaks. The `CaptureReadback` struct on the stack with manual cleanup below is error-prone.

**Fix:** Pre-allocate a fixed capture staging buffer at init time (sized to the maximum swapchain extent) and reuse it. Use RAII wrappers for `VkBuffer` / `VkDeviceMemory`.

---

### 🟡 `m_sync_index` and `m_frame_sync` are redundant

**File:** `src/renderer/Renderer.hpp`, `Renderer.cpp`

`m_frame_sync` is set to `m_sync_index` at the top of `begin_frame` and then `m_sync_index` is incremented at the bottom of `end_frame`. The only purpose is to "snapshot" the sync index across the frame boundary. Since `begin_frame` and `end_frame` are always called in the same thread in strict alternation, `m_frame_sync` can just be `m_sync_index` throughout, and the copy can be removed.

---

### 🟡 `Renderer::pipeline_for` has a non-`default` fall-through that returns `m_pipeline_line_strip`

**File:** `src/renderer/Renderer.cpp`

```cpp
Pipeline& Renderer::pipeline_for(Topology topo) {
    switch (topo) { ... }
    return m_pipeline_line_strip;  // silent fallback
}
```

The enum is exhaustive, so the fall-through is unreachable — but the compiler cannot know that without a `default: std::unreachable()` or `[[unlikely]]` annotation. Add `default: std::unreachable();` (C++23) to make the intent explicit and enable the optimizer.

---

## 3. Simulation / Math Correctness

### 🔴 `DelayDifferentialSystem::query_history` uses a linear scan (`while hi < size`) not binary search

**File:** `src/sim/DelayDifferentialSystem.hpp` — `DelayInitialValueProblem::query_history()`

```cpp
std::size_t hi = 1u;
while (hi < m_history.size() && m_history[hi].t < t)
    ++hi;
```

This is an **O(n) linear scan** over the entire history. `HistoryBuffer::query()` in the same codebase correctly uses binary search. For a 1-hour run at 120 Hz this is 432,000 samples, and every step of the DDE solver calls `query_history` at least once. This will become **the primary performance bottleneck** as soon as simulation time grows.

**Fix:** Replace with `std::lower_bound` over `m_history` (which is always sorted by `t`):
```cpp
auto it = std::lower_bound(m_history.begin(), m_history.end(), t,
    [](const DdeHistorySample& s, f64 val){ return s.t < val; });
```

---

### 🟠 `MilsteinIntegrator::normal2()` uses `thread_local` RNG seeded from `s_global_seed` — but the seed is cast to `uint32_t`, discarding entropy

**File:** `src/sim/MilsteinIntegrator.hpp`

```cpp
thread_local std::mt19937 rng{ []() -> uint32_t {
    if (s_seed_set)
        return static_cast<uint32_t>(s_global_seed);  // truncates 64-bit seed
    return std::random_device{}();
}() };
```

`s_global_seed` is `uint64_t` but is truncated to `uint32_t` for `std::mt19937` seeding. The upper 32 bits are silently discarded, halving the effective seed space. `std::mt19937_64` or `std::seed_seq` should be used instead:

```cpp
std::seed_seq seq{
    static_cast<uint32_t>(s_global_seed >> 32),
    static_cast<uint32_t>(s_global_seed & 0xFFFFFFFF)
};
thread_local std::mt19937 rng{seq};
```

---

### 🟠 `MilsteinIntegrator::sigma_gradient` computes 8 equation evaluations per step, each possibly evaluating `surface.du`/`surface.dv` — O(1) but with large constant

**File:** `src/sim/MilsteinIntegrator.hpp` — `sigma_gradient()`

For the Brownian motion use-case, `noise_coefficient()` returns a constant `{sigma, sigma}`, so `sigma_gradient = {0, 0}` and the Milstein correction vanishes — but the 8 perturbed evaluations still run every step. This is a **pure dead computation** for the default case.

**Fix:** Add a `bool is_sigma_constant() const` to `IEquation` (defaulting to `false`). `BrownianMotion` can override it to `true`, and `MilsteinIntegrator::step` can skip the gradient evaluation entirely.

---

### 🟡 `EulerIntegrator` and `MilsteinIntegrator` both accumulate phase via `ops::length(velocity) * dt`

**File:** `src/sim/EulerIntegrator.hpp`, `MilsteinIntegrator.hpp`

For `MilsteinIntegrator`, the length is taken of the **drift** `mu` only, ignoring the stochastic `sigma * dW` contribution. This means a purely-diffusing particle (`mu = 0`) accumulates zero phase, which is probably intentional — but it is not documented. The mismatch between `EulerIntegrator::step` and `MilsteinIntegrator::step` in how they handle `state.phase` is a latent inconsistency: if a behaviour ever reads `phase` on a Milstein particle, it will get different semantics.

---

### 🟡 `DelayPursuitEquation` includes `numeric/ops.hpp` twice

**File:** `src/sim/DelayPursuitEquation.hpp`

```cpp
#include "numeric/ops.hpp"
// ... many lines later ...
#include "numeric/ops.hpp"
```

`#pragma once` protects against double-inclusion, so this is harmless — but it is a copy-paste error that should be cleaned up.

---

### 🟡 `DomainConfinement` and other constraint files are in `sim/` but are not reviewed here — confirm they wrap UV coordinates using the surface's own periodicity API, not a hardcoded `[0, 2π]` range

This was not confirmed from the files read. If confinement uses hardcoded bounds rather than `surface.u_min() / u_max()`, it will break when you change the surface domain (e.g., the torus target uses `[0, 2π]` but the Gaussian uses `[-3, 3]`).

---

## 4. Engine & Architecture

### 🟠 `Engine` has 15+ private member functions and renders its own ImGui panels — violates single-responsibility

**File:** `src/engine/Engine.hpp`, `Engine.cpp`

`Engine` is responsible for:
- Vulkan lifecycle (init, swapchain, resize)
- The simulation registry and switching
- Global ImGui panels (`draw_global_status_panel`, `draw_debug_coordinates_panel`, `draw_simulation_metadata_panel`, `draw_event_log_panel`)
- Input dispatch (`on_key_event`, `dispatch_global_hotkeys`, `update_render_view_input`)
- PNG capture (`request_capture`, `make_capture_path`)
- Frame pacing (`run_frame`)

This is 6+ responsibilities. The panel drawing in particular (`draw_global_status_panel` is 40 lines of ImGui) belongs in a dedicated `GlobalPanels` class, following the pattern already established by `SimulationSurfaceGaussian` which delegates to named `draw_*_panel()` methods but at least keeps them within the simulation class.

---

### 🟠 `g_hotkey_engines` is a module-level `std::unordered_map<GLFWwindow*, Engine*>` — global mutable state

**File:** `src/engine/Engine.cpp`

```cpp
namespace {
std::unordered_map<GLFWwindow*, Engine*> g_hotkey_engines;
}
```

This pattern prevents multiple `Engine` instances from coexisting and is not thread-safe. For a single-window tool this is fine, but the `SecondWindow` also registers key callbacks via GLFW — what happens if the second window receives key events? The map only covers the primary window. Use `glfwSetWindowUserPointer` / `glfwGetWindowUserPointer` instead: it is the canonical GLFW callback dispatch mechanism and requires no global state.

---

### 🟠 `EngineAPI` is a bag of `std::function` with large capture cost

**File:** `src/engine/EngineAPI.hpp`

`EngineAPI` is constructed per-call-site via `Engine::make_api()` (called from `SimulationRuntime::tick`?) and contains 9 `std::function` objects, each potentially with a heap-allocated closure that captures `this` (the `Engine` pointer). Every access through the API goes through a `std::function` vtable dispatch.

For a frame-rate-critical path this is measurably expensive. The pattern is used to decouple simulations from the engine — which is good — but `std::function` is the wrong tool here. Use either:
- A virtual interface `IEngineServices` that the engine implements (zero overhead), or
- A struct of raw function pointers with a `void* context` (C-style, but zero overhead).

The `std::function` overhead is especially wasteful for `acquire` and `submit_to`, which are called per-draw-packet.

---

### 🟡 `Engine::on_key_event` uses a chain of `if/else if` for Ctrl+1…5 simulation switching

**File:** `src/engine/Engine.cpp`

```cpp
if (ctrl && !shift && key == GLFW_KEY_1) { m_pending_sim = 0; return; }
if (ctrl && !shift && key == GLFW_KEY_2) { m_pending_sim = 1; return; }
// ...×5
```

This should be a range check:
```cpp
if (ctrl && !shift && key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
    const std::size_t index = static_cast<std::size_t>(key - GLFW_KEY_1);
    if (index < m_simulations.size()) m_pending_sim = index;
    return;
}
```

This is the kind of mechanical duplication that becomes a maintenance burden when you add a 6th or 7th simulation.

---

### 🟡 `make_capture_path` uses `localtime_s` — Windows-only

**File:** `src/engine/Engine.cpp`

```cpp
localtime_s(&tm, &now);
```

`localtime_s` is a Microsoft extension (`<ctime>`). POSIX uses `localtime_r`. This will fail to compile on Linux/macOS without an adapter. Replace with `std::chrono` + `std::format("{:%Y%m%d_%H%M%S}", ...)` (C++20 chrono formatting), which is portable and idiomatic.

---

### 🟡 `.bak` files committed to the repository

**Directory:** `src/app/`

The directory contains:
```
SurfaceSimController.hpp.bak
SurfaceSimScene.cpp.bak
SurfaceSimSceneBindings.cpp.bak
... (9 files)
```

These are dead code committed to source control. They inflate the repository and create cognitive noise during review. Add them to `.gitignore` or delete them permanently — your git history is a better archive than `.bak` files.

---

### 🟡 `SimulationSnapshotStore` mutex is held during `snapshot()` copy — frame-rate visible if snapshot is large

**File:** `src/engine/SimulationRuntime.hpp`

```cpp
[[nodiscard]] SimulationSnapshot snapshot() const {
    std::scoped_lock lock(m_mutex);
    return m_snapshot;  // copies the entire SceneSnapshot
}
```

`SceneSnapshot` contains `memory::FrameVector<ParticleSnapshot>` (a PMR vector of structs). Copying this under a lock on the render thread while the sim thread is writing it could cause frame hitches as particle count grows. The mutex is not currently needed because everything runs on one thread — but the mutex implies (falsely) that this is thread-safe for multi-threaded access. Either remove the mutex (document single-threaded use) or make `SimulationSnapshot` a cheap copyable value type (e.g. a flat `std::vector` not a PMR vector).

---

## 5. CMake & Build System

### 🟠 `imgui` target shadows the CMake FetchContent internal target name

**File:** `CMakeLists.txt`

```cmake
add_library(imgui STATIC ...)
```

FetchContent defines a variable `imgui_SOURCE_DIR` and populates it, but you also call `add_library(imgui ...)` yourself. If any transitive dependency also tries to `add_library(imgui ...)` (or if `imgui`'s own build is ever enabled), CMake will error with a duplicate target name. Prefer a namespaced target name like `ndde_imgui`.

---

### 🟡 Sanitisers are enabled unconditionally (`ENABLE_SANITIZERS ON` by default) but not applied to the targets

**File:** `CMakeLists.txt`

```cmake
option(ENABLE_SANITIZERS "Build with ASan + UBSan (Debug only)" ON)
```

The option is declared but there is no corresponding:
```cmake
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(... PRIVATE -fsanitize=address,undefined)
    target_link_options(... PRIVATE -fsanitize=address,undefined)
endif()
```

The option is currently **dead** — it has no effect on the build. Either wire it up or remove it to avoid misleading documentation.

---

### 🟡 `GIT_SHALLOW OFF` for ImGui is unusual and slow

**File:** `CMakeLists.txt`

```cmake
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    OFF)
```

`GIT_SHALLOW OFF` downloads the **full git history** for ImGui, which is a large repository with years of commits. Since you pin to a branch tip (`docking`) anyway, use `GIT_SHALLOW ON` (or the default) to dramatically reduce clone time in CI and for new contributors.

---

### 🟡 `SHADER_DIR` and `NDDE_PROJECT_DIR` are baked as absolute paths at compile time

**File:** `src/CMakeLists.txt`

```cmake
target_compile_definitions(nurbs_dde PRIVATE
    SHADER_DIR="${CMAKE_BINARY_DIR}/shaders"
    NDDE_PROJECT_DIR="${CMAKE_SOURCE_DIR}"
)
```

These macros embed absolute paths from the build machine into the binary. The resulting executable is **not relocatable** — moving the build directory or the source tree invalidates the baked paths. For a developer tool this is acceptable, but it means the binary cannot be distributed. Consider loading paths at runtime from a config file (you already have `engine_config.json`) or relative to `argv[0]`.

---

## 6. Code Style & Idiomatic C++23

### 🟡 `SimulationDelayDifferential2D::seed_history` is a static member acting as a C callback

**File:** `src/app/SimulationDelayDifferential2D.cpp`

```cpp
static void seed_history(f64 t, std::span<f64> out, void* user) {
    const auto* self = static_cast<const SimulationDelayDifferential2D*>(user);
    ...
}
```

The `void* user` pattern is C-style. The `HistoryInitializer` in `DelayInitialValueProblem` should accept `std::function<void(f64, std::span<f64>)>` instead. This is a **template parameter** in the existing code, so the change is purely at the callsite — the IVP constructor can take a `std::function` or a lambda and store it, eliminating the void-pointer cast.

---

### 🟡 `constexpr double kTwoPi` should use `std::numbers::pi` (C++20)

**File:** `src/app/SimulationDelayDifferential2D.cpp`

```cpp
constexpr double kTwoPi = 6.28318530717958647692;
```

C++20 introduced `<numbers>`:
```cpp
#include <numbers>
constexpr double kTwoPi = 2.0 * std::numbers::pi;
```

The magic literal is fragile (one wrong digit silently changes your DDE dynamics).

---

### 🟡 `AnimatedCurve::MAX_TRAIL`, `WALK_SPEED`, `MAX_SLOTS` are public `static constexpr` on a concrete class

**File:** `src/app/AnimatedCurve.hpp`

```cpp
static constexpr u32 MAX_TRAIL   = 1200;
static constexpr f32 WALK_SPEED  = 0.9f;
static constexpr u32 MAX_SLOTS   = 4u;
```

These are **implementation constants** exposed as part of the public API. Callers can read them but have no reason to — `MAX_TRAIL` is used internally in `tessellate_trail` and nowhere else outside the class. Move them to the `private:` section.

---

### 🟡 `role_name()` and `draw_*_panel()` free functions in `ParticleTypes.hpp` and various `.cpp` files are not marked `noexcept` consistently

The `[[nodiscard]] inline std::string_view role_name(ParticleRole)` in `ParticleTypes.hpp` is not `noexcept` despite returning a string literal (which cannot throw). The pattern in the engine is inconsistent: some free utility functions are `noexcept`, some are not.

---

### 🔵 `Engine::flush_render_service` manually copies a `std::span` into an arena slice with a raw loop

**File:** `src/engine/Engine.cpp`

```cpp
auto verts = slice.vertices();
for (u32 i = 0; i < static_cast<u32>(packet.vertices.size()); ++i)
    verts[i] = packet.vertices[i];
```

Replace with:
```cpp
std::ranges::copy(packet.vertices, slice.vertices().begin());
```

or `std::memcpy` if `Vertex` is trivially copyable (which it likely is given it is a GPU vertex type).

---

### 🔵 `SimulationRegistry::add_runtime` uses `std::tuple` + `std::apply` for perfect-forwarding capture — consider a simpler factory lambda

**File:** `src/engine/SimulationRuntime.hpp`

The `add_runtime` template is impressively written but introduces a subtle bug: `args_tuple` captures by move, but the lambda is `mutable` to allow `std::apply` to forward the args. However the lambda is called exactly once (at instantiation time), so the moves are safe. **Consider simplifying** to:

```cpp
template <class Simulation>
void add_runtime(std::string name) {
    auto runtime = m_memory.persistent().make_unique<SimulationRuntime>(
        std::move(name),
        [](memory::MemoryService& memory) -> memory::Unique<ISimulation> {
            return memory.simulation().make_unique_as<ISimulation, Simulation>(&memory);
        });
    add(std::move(runtime));
}
```

The extra `Args&&...` forwarding machinery only helps if simulations have constructor parameters — and currently none of the registered simulations use extra args.

---

## 7. Testing

### 🟠 The `tests/` directory exists but was not examined — confirm it covers the mathematical kernel

The PMR-backed memory system, the `HistoryBuffer` ring-buffer logic, and the DDE history interpolation are the most numerically sensitive components. Confirm there are unit tests for:
- `HistoryBuffer::query()` at both the ring-buffer wrap boundary and the clamping edges.
- `DelayInitialValueProblem::query_history()` near `t < oldest` and `t > newest`.
- `MilsteinIntegrator` on a linear SDE with known exact solution (variance growth).

If these are absent, they are the highest-value tests to write — they guard the mathematical correctness of every simulation.

---

## Summary Table

| # | File(s) | Severity | Issue |
|---|---------|----------|-------|
| 1 | `ParticleSystem.hpp` | 🔴 | `destroy_at`/`construct_at` on live PMR vector — potential UB |
| 2 | `Renderer.cpp` | 🔴 | `ALL_COMMANDS` pipeline stall in every image barrier |
| 3 | `Renderer.cpp` | 🔴 | Hardcoded BGRA swap in PNG capture — format-dependent, unguarded |
| 4 | `DelayDifferentialSystem.hpp` | 🔴 | O(n) linear scan in `query_history` — will dominate CPU time |
| 5 | `SimulationHost.hpp` | 🟠 | `SimulationHost` value return conceals hidden init-order dependency |
| 6 | `DelayDifferentialSystem.hpp` | 🟠 | `m_history_values` grows without a cap — unbounded arena exhaustion |
| 7 | `Renderer.hpp/.cpp` | 🟠 | No CPU-GPU double buffering; single command buffer stalls every frame |
| 8 | `Renderer.cpp` | 🟠 | Per-capture `vkAllocateMemory` — driver allocation count concern + exception-unsafe |
| 9 | `MilsteinIntegrator.hpp` | 🟠 | 64-bit seed truncated to 32-bit — reduced RNG entropy |
| 10 | `MilsteinIntegrator.hpp` | 🟠 | 8 dead equation evaluations per step when `sigma` is constant |
| 11 | `Engine.hpp/.cpp` | 🟠 | `Engine` class has too many responsibilities; ImGui panels should be extracted |
| 12 | `Engine.cpp` | 🟠 | Global `g_hotkey_engines` map — should use `glfwSetWindowUserPointer` |
| 13 | `EngineAPI.hpp` | 🟠 | `std::function` for per-frame hot path (`acquire`, `submit_to`) — virtual or fn-ptr preferred |
| 14 | `CMakeLists.txt` | 🟠 | `imgui` target name conflicts with FetchContent namespace |
| 15 | `Containers.hpp` | 🟡 | All vector aliases are the same type — no compile-time lifetime enforcement |
| 16 | `AnimatedCurve.hpp` | 🟡 | `next_id()` atomic restarts each run — breaks serialisation/replay |
| 17 | `Renderer.cpp` | 🟡 | `m_frame_sync` / `m_sync_index` redundancy |
| 18 | `Renderer.cpp` | 🟡 | `pipeline_for` missing `default: std::unreachable()` |
| 19 | `Engine.cpp` | 🟡 | `localtime_s` is Windows-only — use `std::chrono` |
| 20 | `Engine.cpp` | 🟡 | Ctrl+1…5 chain of `if`s — replace with range check |
| 21 | `SimulationRuntime.hpp` | 🟡 | Mutex held during `SceneSnapshot` copy — potential hitch as particle count grows |
| 22 | `src/app/` | 🟡 | 9 `.bak` files committed to repository |
| 23 | `CMakeLists.txt` | 🟡 | `ENABLE_SANITIZERS` option is declared but never applied |
| 24 | `CMakeLists.txt` | 🟡 | `GIT_SHALLOW OFF` for ImGui — slow clone |
| 25 | `CMakeLists.txt` | 🟡 | Absolute `SHADER_DIR` / `NDDE_PROJECT_DIR` embedded in binary |
| 26 | `DelayPursuitEquation.hpp` | 🟡 | `#include "numeric/ops.hpp"` duplicated |
| 27 | `SimulationDelayDifferential2D.cpp` | 🟡 | C-style `void*` callback for history seeder — use `std::function` |
| 28 | `SimulationDelayDifferential2D.cpp` | 🟡 | Magic literal `kTwoPi` — use `std::numbers::pi` |
| 29 | `AnimatedCurve.hpp` | 🟡 | `MAX_TRAIL`, `WALK_SPEED`, `MAX_SLOTS` should be private |
| 30 | `Engine.cpp` | 🔵 | Manual copy loop in `flush_render_service` — use `std::ranges::copy` |
| 31 | `SimulationRuntime.hpp` | 🔵 | `add_runtime` tuple-apply complexity — likely simplifiable |
| 32 | `tests/` | 🟠 | Mathematical kernel tests (HistoryBuffer, DDE interpolation, Milstein) should be confirmed present |

---

*End of review. Total issues: 32 across 7 categories.*
