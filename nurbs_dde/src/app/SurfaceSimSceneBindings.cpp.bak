#include "app/SurfaceSimScene.hpp"
#include <imgui.h>

namespace ndde {

void SurfaceSimScene::register_bindings() {
    // Scene-specific bindings. HotkeyManager owns delivery and panel display.
    // Groups appear as section headers in draw_hotkey_panel().

    // Overlays
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_F), "Frenet frame  (T, N, B)",
                               m_overlays.show_frenet,       "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_D), "Surface frame  (Dx, Dy)",
                               m_overlays.show_dir_deriv,    "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_N), "Normal plane patch",
                               m_overlays.show_normal_plane, "Overlays");
    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_T), "Torsion ribbon",
                               m_overlays.show_torsion,      "Overlays");

    // Simulation
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_P), "Pause / unpause",
        [this]{ m_paused = !m_paused; }, "Simulation");

    // Spawn
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_L), "Leader particle  (blue)",
        [this]{
            m_spawner.spawn_gradient_particle(ParticleRole::Leader,
                m_spawner.offset_spawn(m_spawner.reference_uv(), 1.5f, static_cast<float>(m_spawn.leader_count) * 1.1f));
        }, "Spawn");

    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_C), "Chaser particle  (red)",
        [this]{
            m_spawner.spawn_gradient_particle(ParticleRole::Chaser,
                m_spawner.offset_spawn(m_spawner.reference_uv(), 2.0f, static_cast<float>(m_spawn.chaser_count) * 1.3f + 0.5f));
        }, "Spawn");

    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_B), "Brownian particle  (Milstein)",
        [this]{
            m_spawner.spawn_brownian_particle(m_spawner.offset_spawn(m_spawner.reference_uv(), 1.8f,
                static_cast<float>(m_spawn.chaser_count + m_spawn.leader_count) * 0.7f + 1.0f));
        }, "Spawn");

    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_R), "Delay-pursuit chaser",
        [this]{
            if (m_curves.empty()) return;
            const glm::vec2 ref = m_curves[0].head_uv();
            m_spawner.spawn_delay_pursuit_particle(m_spawner.offset_spawn(ref, 2.0f,
                static_cast<float>(m_spawn.delay_pursuit_count) * 1.1f + 0.3f));
        }, "Spawn");

    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_A), "Leader seeker / pursuer  [Ctrl+A]",
        [this]{
            if (!m_spawn.spawning_pursuer) m_spawner.spawn_leader_seeker();
            else                           m_spawner.spawn_pursuit_particle();
        }, "Spawn");

    // Panels
    m_hotkeys.register_action(Chord::ctrl(ImGuiKey_Q), "Coordinate debug panel",
        [this]{
            m_ui.debug_open = !m_ui.debug_open;
            m_coord_debug.visible() = m_ui.debug_open;
        }, "Panels");

    m_hotkeys.register_toggle(Chord::ctrl(ImGuiKey_H), "This hotkey panel",
                               m_ui.hotkey_panel_open, "Panels");
}

void SurfaceSimScene::on_key_event(int key, int action, int mods) {
    (void)m_hotkeys.handle_key_event(key, action, mods);
}

} // namespace ndde
