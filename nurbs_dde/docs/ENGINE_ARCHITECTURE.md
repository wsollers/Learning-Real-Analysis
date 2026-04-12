# NDDE Engine — Architecture Reference

## Layer Map

```
app/main.cpp
    └── Engine::start() / run()          [engine/]
          ├── AppConfig                  JSON → typed config struct
          ├── GlfwContext                window, resize flag         [platform/]
          ├── VulkanContext              instance, device, queues    [platform/]
          ├── Swapchain                  images, format              [renderer/]
          ├── Renderer                   frame loop, pipelines, ImGui [renderer/]
          │     ├── Pipeline × 3         LineList / LineStrip / TriangleList
          │     └── ImGuiLayer           descriptor pool, fonts
          ├── BufferManager              per-frame vertex arena      [memory/]
          └── Scene(EngineAPI)           application logic           [app/]
                ├── math::Axes           grid + axis geometry        [math/]
                └── math::IConic         curves + surfaces           [math/]
```

The dependency rule is strict: each layer may only include headers from
layers **below** it in the diagram. `math/` has no Vulkan. `Scene` has no
Vulkan. The CMake static-library graph enforces this at link time.

---

## The EngineAPI Contract

`Scene` receives an `EngineAPI` struct of `std::function` closures. It sees
nothing else from the engine. The complete interface:

```cpp
api.acquire(n)                        // get n Vertex slots from the arena
api.submit(slice, topology, mode, color, mvp)  // record a draw call
api.math_font_body()                  // ImFont* for the STIX math font
api.viewport_size()                   // Vec2 live window dimensions
api.config()                          // const AppConfig& startup config
```

`acquire` and `submit` are the only two operations a math component needs.

---

## Frame Sequencing

Every frame follows this exact order, enforced by `Engine::run_frame()`:

```
1. BufferManager::reset()         — cursor back to 0, arena reused
2. Renderer::begin_frame()        — wait fence, acquire image, begin cmd
3. Renderer::imgui_new_frame()    — ImGui input snapshot
4. Scene::on_frame()              — ALL user code runs here
     a. update_camera()           — pan/zoom input
     b. update_hover()            — hit test
     c. draw_*_panel()            — ImGui UI
     d. submit_*()                — acquire → write → submit geometry
5. Renderer::imgui_render()       — flush ImGui draw data
6. Renderer::end_frame()          — end rendering, submit queue, present
```

Nothing outside `Scene::on_frame()` is user-accessible. The engine is a
black box that calls `on_frame()` once per frame and handles everything else.

---

## Memory Model

The vertex arena is a single 128 MB host-visible, host-coherent Vulkan
buffer, persistently mapped for the lifetime of the application. Each frame:

- `reset()` moves the bump cursor back to byte 0 (O(1), no free)
- `acquire(n)` atomically advances the cursor by `n * sizeof(Vertex)`
  and returns a pointer directly into GPU-visible memory
- `submit()` records `vkCmdBindVertexBuffers` + `vkCmdDraw` referencing
  the slice's byte offset in that buffer

Zero copies. No staging. The GPU reads the memory the CPU just wrote.
The render fence in `begin_frame()` guarantees the GPU has finished
reading the previous frame before `reset()` reclaims the memory.

---

## The Orthographic Camera

World space is a right-handed coordinate system: X right, Y up, Z toward viewer.

The MVP matrix is a pure orthographic projection with pan and zoom:

```
half_h = base_extent / zoom
half_w = half_h * aspect          ← aspect ratio correction
left   = -half_w + pan_x
right  =  half_w + pan_x
bottom = -half_h + pan_y
top    =  half_h + pan_y

MVP = glm::ortho(left, right, bottom, top, -10, 10)
```

Crucially, `half_w` is scaled by the aspect ratio so one world unit maps
to the same physical distance in both X and Y — circles are circles, not
ellipses. The old code used a square frustum (`[-e, e, -e, e]`) which
stretched everything horizontally on non-square windows.

To unproject a pixel `(px, py)` back to world space, `Scene::pixel_to_world()`
inverts this matrix via `glm::inverse(ortho_mvp())`.

---

## Pipelines

Three `Pipeline` objects are created at startup, one per `Topology`:

| Topology | VkPrimitiveTopology | Use |
|---|---|---|
| `LineList` | LINE_LIST | Axes, grid, secant, tangent (vertex pairs) |
| `LineStrip` | LINE_STRIP | Curves, epsilon ball (contiguous chain) |
| `TriangleList` | TRIANGLE_LIST | Surfaces (future) |

The pipeline selection is made in `Renderer::pipeline_for(topology)`. The
math layer communicates its topology requirement via the `Topology` enum —
it never sees a `VkPipeline` handle.

---

## Adding a Component

See `docs/ADDING_A_MATH_COMPONENT.md` for the full walkthrough.
The short version: write a class in `math/`, add it to `ndde_math` in
`src/CMakeLists.txt`, wire it in `app/Scene`. Touch nothing else.
