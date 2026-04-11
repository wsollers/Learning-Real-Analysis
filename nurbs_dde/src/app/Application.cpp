// app/Application.cpp
#include "Application.hpp"
#include "geometry/Axes2D.hpp"
#include "geometry/Parabola.hpp"
#include "geometry/Hyperbola.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace ndde::app {

// ── Constructor ────────────────────────────────────────────────────────────────

Application::Application() {
    init_window();
    init_vulkan();
    init_curves();
    m_last_time = glfwGetTime();
}

Application::~Application() { cleanup(); }
void Application::run() { main_loop(); }

// ── init_window ────────────────────────────────────────────────────────────────

void Application::init_window() {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_TRUE);

    GLFWmonitor*       mon  = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);
    m_width  = mode->width;
    m_height = mode->height;

    m_window = glfwCreateWindow(m_width, m_height, "nurbs_dde", mon, nullptr);
    if (!m_window) throw std::runtime_error("glfwCreateWindow failed");

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetCursorPosCallback(m_window, cursor_pos_callback);
}

// ── init_vulkan ────────────────────────────────────────────────────────────────

void Application::init_vulkan() {
    m_ctx.init(m_window);
    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    m_swapchain.init(m_ctx, m_width, m_height);

    const std::string shader_dir = SHADER_DIR;
    m_renderer.init(m_window, m_ctx, m_swapchain,
                    shader_dir + "/curve.vert.spv",
                    shader_dir + "/curve.frag.spv",
                    std::string(ASSETS_DIR));
}

// ── init_curves ────────────────────────────────────────────────────────────────

void Application::init_curves() {
    {
        CurveEntry e;
        e.type      = CurveType::Parabola;
        e.name      = "Parabola";
        e.colour[0] = 0.f; e.colour[1] = 1.f; e.colour[2] = 1.f;
        m_curves.push_back(std::move(e));
    }
    {
        CurveEntry e;
        e.type      = CurveType::Hyperbola;
        e.name      = "Hyperbola";
        e.colour[0] = 1.f; e.colour[1] = 0.4f; e.colour[2] = 0.1f;
        m_curves.push_back(std::move(e));
    }

    rebuild_axes();
}

// ── rebuild_curve ──────────────────────────────────────────────────────────────

void Application::rebuild_curve(std::size_t idx) {
    CurveEntry& e = m_curves[idx];

    if (e.type == CurveType::Parabola) {
        if (e.par_tmin >= e.par_tmax) return;
        auto p = std::make_unique<geometry::Parabola>(
            e.par_a, e.par_b, e.par_c, e.par_tmin, e.par_tmax);
        e.verts = p->vertex_buffer(512);
        e.curve = std::move(p);
        m_renderer.upload_curve(m_ctx, idx, e.verts, e.colour, false);
    } else {
        auto h = std::make_unique<geometry::Hyperbola>(
            e.hyp_a, e.hyp_b, e.hyp_h, e.hyp_k,
            e.hyp_axis == 0 ? geometry::HyperbolaAxis::Horizontal
                            : geometry::HyperbolaAxis::Vertical,
            e.hyp_range);
        e.verts = h->two_branch_buffer(512);
        e.curve = std::move(h);
        m_renderer.upload_curve(m_ctx, idx, e.verts, e.colour, true);
    }
    e.draw_fraction = 0.f;
    e.playing       = true;
}

// ── rebuild_axes ──────────────────────────────────────────────────────────────

void Application::rebuild_axes() {
    float ext = std::max({std::abs(view_left()),  std::abs(view_right()),
                          std::abs(view_bottom()), std::abs(view_top())}) * 1.05f;
    geometry::AxesConfig cfg;
    cfg.extent       = ext;
    cfg.tick_spacing = 0.25f;
    cfg.tick_size    = 0.025f * m_zoom;
    m_renderer.upload_axes(m_ctx, geometry::build_axes(cfg));
}

// ── Viewport ──────────────────────────────────────────────────────────────────

float Application::view_left() const {
    return m_pan_x - m_zoom * (static_cast<float>(m_width) /
                                static_cast<float>(m_height));
}
float Application::view_right() const {
    return m_pan_x + m_zoom * (static_cast<float>(m_width) /
                                static_cast<float>(m_height));
}
float Application::view_bottom() const { return m_pan_y - m_zoom; }
float Application::view_top()    const { return m_pan_y + m_zoom; }

math::Mat4 Application::build_mvp() const {
    return glm::ortho(view_left(), view_right(),
                      view_top(), view_bottom(), -1.f, 1.f);
}

// ── Hover / epsilon ball ───────────────────────────────────────────────────────

HoverResult Application::compute_hover() const {
    if (ImGui::GetIO().WantCaptureMouse) return {};

    double mx, my;
    glfwGetCursorPos(m_window, &mx, &my);

    int win_w, win_h;
    glfwGetWindowSize(m_window, &win_w, &win_h);

    const float nx = static_cast<float>(mx) / static_cast<float>(win_w);
    const float ny = static_cast<float>(my) / static_cast<float>(win_h);

    const float wx = view_left() + nx * (view_right() - view_left());
    const float wy = view_bottom() + ny * (view_top() - view_bottom());

    HoverResult best;
    float best_dist = m_analysis_panel.get_epsilon_ball_radius();

    for (int ci = 0; ci < static_cast<int>(m_curves.size()); ++ci) {
        const CurveEntry& e = m_curves[ci];
        if (!e.enabled || e.verts.empty()) continue;

        const std::uint32_t draw_count = (e.draw_fraction >= 1.f)
            ? static_cast<std::uint32_t>(e.verts.size())
            : static_cast<std::uint32_t>(
                std::ceil(e.draw_fraction * static_cast<float>(e.verts.size())));

        for (std::uint32_t i = 0; i < draw_count; ++i) {
            if (e.verts[i].w < 0.5f) continue;
            const float dx = e.verts[i].x - wx;
            const float dy = e.verts[i].y - wy;
            const float d  = std::sqrt(dx * dx + dy * dy);
            if (d < best_dist) {
                best_dist      = d;
                best.hit       = true;
                best.world_x   = e.verts[i].x;
                best.world_y   = e.verts[i].y;
                best.curve_idx = ci;
                best.snap_idx  = static_cast<int>(i);
            }
        }
    }

    // ── Secant calculation ────────────────────────────────────────────────────
    if (best.hit) {
        const CurveEntry& ce  = m_curves[best.curve_idx];
        const int  snap       = best.snap_idx;
        const int  total      = static_cast<int>(ce.verts.size());
        const float eps       = m_analysis_panel.get_epsilon_ball_radius();

        // Walk left from snap_idx staying within epsilon
        int li = snap;
        while (li > 0) {
            int prev = li - 1;
            if (ce.verts[prev].w < 0.5f) break;  // sentinel guard
            float dx = ce.verts[prev].x - best.world_x;
            float dy = ce.verts[prev].y - best.world_y;
            if (std::sqrt(dx * dx + dy * dy) > eps) break;
            li = prev;
        }

        // Walk right from snap_idx staying within epsilon
        int ri = snap;
        while (ri < total - 1) {
            int next = ri + 1;
            if (ce.verts[next].w < 0.5f) break;  // sentinel guard
            float dx = ce.verts[next].x - best.world_x;
            float dy = ce.verts[next].y - best.world_y;
            if (std::sqrt(dx * dx + dy * dy) > eps) break;
            ri = next;
        }

        if (li != ri) {
            best.has_secant = true;
            best.secant_x0  = ce.verts[li].x;
            best.secant_y0  = ce.verts[li].y;
            best.secant_x1  = ce.verts[ri].x;
            best.secant_y1  = ce.verts[ri].y;
            const float dx  = best.secant_x1 - best.secant_x0;
            const float dy  = best.secant_y1 - best.secant_y0;
            best.slope      = (std::abs(dx) > 1e-6f) ? dy / dx : 0.f;
            best.intercept  = best.secant_y0 - best.slope * best.secant_x0;
        }
    }

    return best;
}

std::vector<math::Vec4> Application::build_epsilon_circle(
    float cx, float cy, float r) const
{
    constexpr int N = 64;
    std::vector<math::Vec4> v;
    v.reserve(N + 1);
    for (int i = 0; i <= N; ++i) {
        float angle = 2.f * std::numbers::pi_v<float> * static_cast<float>(i) /
                      static_cast<float>(N);
        v.emplace_back(cx + r * std::cos(angle),
                       cy + r * std::sin(angle),
                       0.f, 1.f);
    }
    return v;
}

// ── tick ──────────────────────────────────────────────────────────────────────

void Application::tick(double now) {
    const float dt = static_cast<float>(now - m_last_time);
    m_last_time = now;

    for (std::size_t i = 0; i < m_curves.size(); ++i) {
        CurveEntry& e = m_curves[i];
        if (!e.enabled || !e.playing) continue;

        e.draw_fraction += m_sim_speed * dt;
        if (e.draw_fraction >= 1.f) {
            e.draw_fraction = 1.f;
            e.playing = false;
        }
        m_renderer.set_curve_fraction(i, e.draw_fraction);
    }
}

// ── UI ────────────────────────────────────────────────────────────────────────

void Application::build_ui() {
    build_master_panel();

    for (std::size_t i = 0; i < m_curves.size(); ++i) {
        if (m_curves[i].enabled && m_curves[i].window_open)
            build_curve_window(i);
    }

    m_analysis_panel.draw(m_hover, m_renderer.font_math_body());

    if (m_hover.hit) {
        ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Always,
                                ImVec2(1.1f, 1.1f));
        ImGui::SetNextWindowBgAlpha(0.7f);
        ImGui::Begin("##hover", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoMove);
        ImGui::Text("(%.4f,  %.4f)", m_hover.world_x, m_hover.world_y);
        ImGui::Text("curve: %s", m_curves[m_hover.curve_idx].name.c_str());
        ImGui::PushFont(m_renderer.font_math_body());
        ImGui::Text("\xce\xb5 = %.4f   f(c) = %.4f",
                    m_analysis_panel.get_epsilon_ball_radius(), m_hover.world_y);
        ImGui::PopFont();
        ImGui::End();
    }
}

void Application::build_master_panel() {
    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowSize(ImVec2(280.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(24.f, 24.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Curves");

    ImGui::SeparatorText("Active curves");

    for (std::size_t i = 0; i < m_curves.size(); ++i) {
        CurveEntry& e = m_curves[i];
        bool was_enabled = e.enabled;

        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Checkbox(e.name.c_str(), &e.enabled)) {
            if (e.enabled && !was_enabled) {
                rebuild_curve(i);
                e.draw_fraction = 0.f;
                e.playing = false;
                m_renderer.set_curve_fraction(i, 0.f);
            } else if (!e.enabled) {
                m_renderer.set_curve_fraction(i, 0.f);
            }
        }
        if (e.enabled) {
            ImGui::SameLine();
            if (ImGui::SmallButton(e.window_open ? "hide##w" : "show##w"))
                e.window_open = !e.window_open;
        }
        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Simulation");

    ImGui::Text("Speed  (fraction/sec)");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("##speed", &m_sim_speed, 0.01f, 2.0f, "%.2f",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::Spacing();

    if (ImGui::Button("Start All", ImVec2(-1.f, 0.f))) {
        for (std::size_t i = 0; i < m_curves.size(); ++i) {
            if (!m_curves[i].enabled) continue;
            m_curves[i].draw_fraction = 0.f;
            m_curves[i].playing = true;
            m_renderer.set_curve_fraction(i, 0.f);
        }
    }
    if (ImGui::Button("Pause All", ImVec2(-1.f, 0.f))) {
        for (auto& e : m_curves) e.playing = false;
    }
    if (ImGui::Button("Reset All", ImVec2(-1.f, 0.f))) {
        for (std::size_t i = 0; i < m_curves.size(); ++i) {
            if (!m_curves[i].enabled) continue;
            m_curves[i].draw_fraction = 0.f;
            m_curves[i].playing = false;
            m_renderer.set_curve_fraction(i, 0.f);
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Viewport");
    ImGui::Text("Zoom %.2f  Pan (%.2f, %.2f)", m_zoom, m_pan_x, m_pan_y);
    if (ImGui::Button("Reset view", ImVec2(-1.f, 0.f))) {
        m_zoom = 1.4f; m_pan_x = 0.f; m_pan_y = 0.f;
        rebuild_axes();
    }
    ImGui::TextDisabled("Scroll=zoom  Middle-drag=pan");

    ImGui::End();
}

void Application::build_curve_window(std::size_t idx) {
    CurveEntry& e = m_curves[idx];

    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowSize(ImVec2(300.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        ImVec2(320.f + static_cast<float>(idx) * 20.f,
               24.f  + static_cast<float>(idx) * 20.f),
        ImGuiCond_FirstUseEver);

    ImGui::PushID(static_cast<int>(idx));
    ImGui::Begin(e.name.c_str(), &e.window_open);

    bool dirty = false;

    ImGui::Text("Colour");
    if (ImGui::ColorEdit3("##col", e.colour))
        m_renderer.upload_curve(m_ctx, idx, e.verts, e.colour,
                                e.type == CurveType::Hyperbola);
    ImGui::Spacing();

    if (e.type == CurveType::Parabola) {
        ImGui::SeparatorText("y = at\xc2\xb2 + bt + c");
        dirty |= ImGui::SliderFloat("a", &e.par_a, -4.f, 4.f, "%.2f");
        dirty |= ImGui::SliderFloat("b", &e.par_b, -4.f, 4.f, "%.2f");
        dirty |= ImGui::SliderFloat("c", &e.par_c, -4.f, 4.f, "%.2f");
        ImGui::Spacing();
        dirty |= ImGui::SliderFloat("t min", &e.par_tmin, -4.f, 0.f, "%.2f");
        dirty |= ImGui::SliderFloat("t max", &e.par_tmax,  0.f, 4.f, "%.2f");
    } else {
        ImGui::SeparatorText("(x-h)\xc2\xb2/a\xc2\xb2 \xe2\x88\x92 (y-k)\xc2\xb2/b\xc2\xb2 = 1");
        dirty |= ImGui::SliderFloat("a",     &e.hyp_a,     0.1f, 4.f, "%.2f");
        dirty |= ImGui::SliderFloat("b",     &e.hyp_b,     0.1f, 4.f, "%.2f");
        dirty |= ImGui::SliderFloat("h",     &e.hyp_h,    -4.f,  4.f, "%.2f");
        dirty |= ImGui::SliderFloat("k",     &e.hyp_k,    -4.f,  4.f, "%.2f");
        dirty |= ImGui::SliderFloat("range", &e.hyp_range,  0.5f, 5.f, "%.2f");
        ImGui::Text("Axis");
        ImGui::SameLine();
        dirty |= ImGui::RadioButton("Horizontal", &e.hyp_axis, 0);
        ImGui::SameLine();
        dirty |= ImGui::RadioButton("Vertical",   &e.hyp_axis, 1);
    }

    if (dirty) {
        rebuild_curve(idx);
        m_renderer.set_curve_fraction(idx, e.draw_fraction);
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Animation");

    ImGui::ProgressBar(e.draw_fraction, ImVec2(-1.f, 0.f));
    ImGui::Spacing();

    if (e.playing) {
        if (ImGui::Button("Pause", ImVec2(-1.f, 0.f))) e.playing = false;
    } else {
        const char* lbl = e.draw_fraction >= 1.f ? "Restart" : "Play";
        if (ImGui::Button(lbl, ImVec2(-1.f, 0.f))) {
            if (e.draw_fraction >= 1.f) {
                e.draw_fraction = 0.f;
                m_renderer.set_curve_fraction(idx, 0.f);
            }
            e.playing = true;
        }
    }

    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderFloat("##scrub", &e.draw_fraction, 0.f, 1.f, "%.3f")) {
        e.playing = false;
        m_renderer.set_curve_fraction(idx, e.draw_fraction);
    }

    ImGui::End();
    ImGui::PopID();
}

// ── main_loop ──────────────────────────────────────────────────────────────────

void Application::main_loop() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);

        static bool space_was_down = false;
        bool space_down = glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (space_down && !space_was_down) {
            for (auto& e : m_curves) if (e.enabled) e.playing = !e.playing;
        }
        space_was_down = space_down;

        if (m_resized) {
            m_resized = false;
            glfwGetFramebufferSize(m_window, &m_width, &m_height);
            while (m_width == 0 || m_height == 0) {
                glfwGetFramebufferSize(m_window, &m_width, &m_height);
                glfwWaitEvents();
            }
            m_renderer.on_resize(m_ctx, m_swapchain, m_width, m_height);
            rebuild_axes();
            continue;
        }

        tick(glfwGetTime());

        // ── Hover + epsilon ball ───────────────────────────────────────────────
        m_hover = m_analysis_panel.show_epsilon_ball()
                  ? compute_hover()
                  : HoverResult{};

        if (m_hover.hit) {
            auto circle = build_epsilon_circle(
                m_hover.world_x, m_hover.world_y,
                m_analysis_panel.get_epsilon_ball_radius());
            m_renderer.upload_epsilon_ball(m_ctx, circle);
        } else {
            m_renderer.clear_epsilon_ball();
        }

        // ── Interval lines ────────────────────────────────────────────────────
        if (m_hover.hit && m_analysis_panel.show_interval_lines()) {
            m_renderer.upload_interval_lines(m_ctx, build_interval_lines());
            m_renderer.upload_centre_lines(m_ctx, build_centre_lines());
        } else {
            m_renderer.clear_interval_lines();
            m_renderer.clear_centre_lines();
        }

        // ── Secant line ───────────────────────────────────────────────────────
        if (m_hover.hit && m_hover.has_secant &&
            m_analysis_panel.show_secant()) {
            const float x0 = view_left();
            const float y0 = m_hover.slope * x0 + m_hover.intercept;
            const float x1 = view_right();
            const float y1 = m_hover.slope * x1 + m_hover.intercept;
            std::vector<math::Vec4> seg = {
                {x0, y0, 0.f, 1.f},
                {x1, y1, 0.f, 1.f}
            };
            m_renderer.upload_secant_line(m_ctx, seg,
                                          m_analysis_panel.secant_colour());
        } else {
            m_renderer.clear_secant_line();
        }

        if (!m_renderer.begin_frame(m_ctx, m_swapchain)) {
            m_renderer.on_resize(m_ctx, m_swapchain, m_width, m_height);
            rebuild_axes();
            continue;
        }

        build_ui();
        m_renderer.draw_scene(build_mvp());

        if (!m_renderer.end_frame(m_ctx, m_swapchain)) {
            m_renderer.on_resize(m_ctx, m_swapchain, m_width, m_height);
            rebuild_axes();
        }
    }
}

// ── cleanup ────────────────────────────────────────────────────────────────────

void Application::cleanup() {
    m_ctx.destroy();
    if (m_window) { glfwDestroyWindow(m_window); m_window = nullptr; }
    glfwTerminate();
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void Application::framebuffer_resize_callback(GLFWwindow* w, int, int) {
    static_cast<Application*>(glfwGetWindowUserPointer(w))->m_resized = true;
}

void Application::scroll_callback(GLFWwindow* w, double, double yoffset) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w));
    if (ImGui::GetIO().WantCaptureMouse) return;
    constexpr float spd = 0.1f;
    float factor = (yoffset > 0) ? (1.f - spd) : (1.f + spd);
    app->m_zoom = std::clamp(app->m_zoom * factor, 0.05f, 50.f);
    app->rebuild_axes();
}

void Application::mouse_button_callback(GLFWwindow* w, int button,
                                         int action, int) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w));
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        app->m_panning = (action == GLFW_PRESS);
        if (app->m_panning)
            glfwGetCursorPos(w, &app->m_pan_last_x, &app->m_pan_last_y);
    }
}

void Application::cursor_pos_callback(GLFWwindow* w, double xpos, double ypos) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w));
    if (!app->m_panning) return;
    const double dx = xpos - app->m_pan_last_x;
    const double dy = ypos - app->m_pan_last_y;
    app->m_pan_last_x = xpos;
    app->m_pan_last_y = ypos;

    const float aspect = static_cast<float>(app->m_width) /
                         static_cast<float>(app->m_height);
    app->m_pan_x -= static_cast<float>(dx) / app->m_width  * 2.f * app->m_zoom * aspect;
    app->m_pan_y += static_cast<float>(dy) / app->m_height * 2.f * app->m_zoom;
    app->rebuild_axes();
}

// ── Interval / centre line builders ───────────────────────────────────────────

std::vector<math::Vec4> Application::build_interval_lines() const {
    if (!m_hover.hit) return {};

    const float cx  = m_hover.world_x;
    const float cy  = m_hover.world_y;
    const float eps = m_analysis_panel.get_epsilon_interval();

    std::vector<math::Vec4> v;
    auto push = [&](float x0, float y0, float x1, float y1) {
        v.emplace_back(x0, y0, 0.f, 1.f);
        v.emplace_back(x1, y1, 0.f, 1.f);
    };

    // Verticals from x-axis to ±ε boundary
    push(cx - eps, 0.f, cx - eps, cy - eps);
    push(cx - eps, 0.f, cx - eps, cy + eps);
    push(cx + eps, 0.f, cx + eps, cy - eps);
    push(cx + eps, 0.f, cx + eps, cy + eps);

    // Horizontals from y-axis to ±ε boundary
    push(0.f, cy - eps, cx - eps, cy - eps);
    push(0.f, cy - eps, cx + eps, cy - eps);
    push(0.f, cy + eps, cx - eps, cy + eps);
    push(0.f, cy + eps, cx + eps, cy + eps);

    return v;
}

std::vector<math::Vec4> Application::build_centre_lines() const {
    if (!m_hover.hit) return {};

    const float cx = m_hover.world_x;
    const float cy = m_hover.world_y;

    std::vector<math::Vec4> v;
    auto push = [&](float x0, float y0, float x1, float y1) {
        v.emplace_back(x0, y0, 0.f, 1.f);
        v.emplace_back(x1, y1, 0.f, 1.f);
    };

    push(cx, 0.f, cx, cy);   // c vertical to x-axis
    push(0.f, cy, cx, cy);   // f(c) horizontal to y-axis

    return v;
}

} // namespace ndde::app