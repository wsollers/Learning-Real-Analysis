# src/app/legacy/

Dead code — preserved for reference only. **Not compiled.**

## Contents

| File | What it was |
|------|-------------|
| `Scene.cpp` | The original conics/analysis scene (parabolas, hyperbolas, helix, paraboloid). `m_scene->on_frame()` was commented out in `Engine::run_frame()` when `SurfaceSimScene` became the active scene. Moved here by E1 refactor. |
| `AnalysisPanel.cpp` | UI panel for the conics scene (AnalysisPanel::draw). Compiled only when Scene.cpp is compiled. Moved here together with Scene.cpp. |
| `Scene_on_frame_patch.bak` | A stale patch fragment from an earlier iteration of `Scene::on_frame()`. References `m_api.math_font_body()` which no longer exists in EngineAPI (removed by D3 refactor). |

## Headers stay in `src/app/`

`Scene.hpp` and `AnalysisPanel.hpp` remain at their original paths because
`Engine.cpp` forward-declares `class Scene` via `Engine.hpp` and constructs
`std::unique_ptr<Scene> m_scene` (the pointer is never dereferenced while
`m_scene->on_frame()` stays commented out). Moving the headers would break
that include chain.

## To re-enable the conics scene

1. Move `Scene.cpp` and `AnalysisPanel.cpp` back to `src/app/`.
2. Add them back to `add_executable(nurbs_dde ...)` in `src/CMakeLists.txt`.
3. Uncomment `m_scene->on_frame();` in `Engine::run_frame()`.
