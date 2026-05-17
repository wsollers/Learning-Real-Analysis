#pragma once
// engine/diagnostics/DiagnosticsService.hpp
// Engine-owned structured diagnostics service.

#include "engine/diagnostics/DiagnosticsTypes.hpp"

#include <span>
#include <vector>

namespace ndde {

class DiagnosticsService {
public:
    [[nodiscard]] DiagnosticId report(DiagnosticReport report, f64 now_seconds = f64(0));
    void report(const ValidationReport& report, f64 now_seconds = f64(0));

    void resolve(DiagnosticId id);
    void resolve_for(ComponentId component);
    void acknowledge(DiagnosticId id);

    void clear_frame_diagnostics();
    void clear_scenario_diagnostics();
    void clear_all();

    [[nodiscard]] std::span<const Diagnostic> active() const noexcept { return m_active; }
    [[nodiscard]] std::span<const Diagnostic> history() const noexcept { return m_history; }

    [[nodiscard]] std::vector<Diagnostic> active_for(ComponentId component) const;
    [[nodiscard]] std::vector<Diagnostic> active_for(RuntimeNodeId node) const;
    [[nodiscard]] std::vector<Diagnostic> active_with(ErrorCode code) const;
    [[nodiscard]] std::vector<Diagnostic> active_at_or_above(DiagnosticSeverity severity) const;

    [[nodiscard]] bool has_errors() const noexcept;
    [[nodiscard]] bool has_fatal() const noexcept;

private:
    std::vector<Diagnostic> m_active;
    std::vector<Diagnostic> m_history;
    u64 m_next_id = u64(1);

    [[nodiscard]] DiagnosticId next_id() noexcept { return DiagnosticId{m_next_id++}; }
    [[nodiscard]] Diagnostic* find_active(DiagnosticId id) noexcept;
    [[nodiscard]] const Diagnostic* find_duplicate(const DiagnosticReport& report) const noexcept;
    [[nodiscard]] Diagnostic* find_duplicate(const DiagnosticReport& report) noexcept;

    void resolve_at(std::vector<Diagnostic>::iterator it);

    [[nodiscard]] static bool same_component(std::optional<ComponentId> lhs,
                                             std::optional<ComponentId> rhs) noexcept;
    [[nodiscard]] static bool same_node(std::optional<RuntimeNodeId> lhs,
                                        std::optional<RuntimeNodeId> rhs) noexcept;
};

[[nodiscard]] constexpr int severity_rank(DiagnosticSeverity severity) noexcept {
    switch (severity) {
        case DiagnosticSeverity::Info: return 0;
        case DiagnosticSeverity::Warning: return 1;
        case DiagnosticSeverity::Error: return 2;
        case DiagnosticSeverity::Fatal: return 3;
    }
    return 0;
}

} // namespace ndde
