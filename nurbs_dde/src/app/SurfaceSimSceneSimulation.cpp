#include "app/SurfaceSimScene.hpp"
#include <fstream>
#include <iomanip>

namespace ndde {

// ── export_session ────────────────────────────────────────────────────────────

void SurfaceSimScene::export_session(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << std::fixed << std::setprecision(6);
    f << "particle_id,role,equation,record_type,step,a,b,c\n";

    for (u32 ci = 0; ci < static_cast<u32>(m_curves.size()); ++ci) {
        const auto& c = m_curves[ci];
        const std::string role = (c.role() == AnimatedCurve::Role::Leader) ? "leader" : "chaser";
        const std::string eq   = c.equation() ? c.equation()->name() : "unknown";

        for (u32 i = 0; i < c.trail_size(); ++i) {
            const Vec3 p = c.trail_pt(i);
            f << ci << ',' << role << ',' << eq << ",trail,"
              << i << ',' << p.x << ',' << p.y << ',' << p.z << '\n';
        }
        if (c.history()) {
            const auto recs = c.history()->to_vector();
            for (std::size_t i = 0; i < recs.size(); ++i) {
                f << ci << ',' << role << ',' << eq << ",history,"
                  << i << ',' << recs[i].t << ','
                  << recs[i].uv.x << ',' << recs[i].uv.y << '\n';
            }
        }
    }
}

} // namespace ndde
