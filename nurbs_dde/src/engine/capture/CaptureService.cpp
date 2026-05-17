// engine/capture/CaptureService.cpp

#include "engine/capture/CaptureService.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace ndde {

namespace {

[[nodiscard]] std::vector<CaptureTarget> expand_targets(CaptureTarget target) {
    if (target == CaptureTarget::BothWindows)
        return {CaptureTarget::MainWindow, CaptureTarget::AlternateWindow};
    return {target};
}

[[nodiscard]] std::string mode_name(CaptureMode mode) {
    switch (mode) {
        case CaptureMode::StillPng: return "StillPng";
        case CaptureMode::MovieFrameSequence: return "MovieFrameSequence";
    }
    return "Unknown";
}

} // namespace

void CaptureService::request_still(CaptureRequest request, CaptureRunMetadata metadata) {
    request.mode = CaptureMode::StillPng;

    const std::string run_id = make_run_id(metadata);
    if (metadata.run_id.empty())
        metadata.run_id = run_id;

    const std::filesystem::path run_dir = m_output_dir / run_id;
    std::filesystem::create_directories(run_dir);

    const std::filesystem::path manifest_path = request.include_manifest
        ? make_manifest_path(run_dir, run_id)
        : std::filesystem::path{};

    std::vector<CaptureArtifact> artifacts;
    for (const CaptureTarget target : expand_targets(request.target)) {
        artifacts.push_back(CaptureArtifact{
            .mode = CaptureMode::StillPng,
            .target = target,
            .path = make_still_path(run_dir, run_id, target, metadata),
            .manifest_path = manifest_path,
            .tick = metadata.tick,
            .sim_time = metadata.sim_time
        });
    }

    if (request.include_manifest)
        write_manifest(manifest_path, request, metadata, artifacts);

    for (const CaptureArtifact& artifact : artifacts) {
        m_pending_stills.push_back(artifact);
        m_completed.push_back(artifact);
    }
}

std::vector<CaptureArtifact> CaptureService::consume_pending_stills() {
    std::vector<CaptureArtifact> out;
    out.swap(m_pending_stills);
    return out;
}

bool CaptureService::start_movie_frames(MovieFrameSequenceOptions options,
                                        CaptureRunMetadata metadata) {
    if (m_movie_status.active)
        return false;

    const std::string run_id = make_run_id(metadata);
    if (metadata.run_id.empty())
        metadata.run_id = run_id;

    const std::filesystem::path run_dir = m_output_dir / run_id;
    std::filesystem::create_directories(run_dir);

    m_movie_options = options;
    m_movie_status = MovieFrameSequenceStatus{
        .active = true,
        .target = options.target,
        .frames_written = u64(0),
        .elapsed_seconds = f32(0),
        .manifest_path = unique_path(run_dir / std::format("{}_movie_frames_manifest.md", run_id))
    };

    if (options.target == CaptureTarget::MainWindow || options.target == CaptureTarget::BothWindows) {
        m_movie_status.main_frame_dir = unique_path(run_dir / "main_frames");
        std::filesystem::create_directories(m_movie_status.main_frame_dir);
    }
    if (options.target == CaptureTarget::AlternateWindow || options.target == CaptureTarget::BothWindows) {
        m_movie_status.alternate_frame_dir = unique_path(run_dir / "alternate_frames");
        std::filesystem::create_directories(m_movie_status.alternate_frame_dir);
    }

    write_movie_frame_manifest(metadata, options, m_movie_status);
    return true;
}

void CaptureService::stop_movie_frames() noexcept {
    m_movie_status.active = false;
}

std::string CaptureService::normalize_stem(std::string_view text) {
    std::string name{text};
    for (char& c : name) {
        const auto uc = static_cast<byte>(c);
        c = std::isalnum(static_cast<int>(uc))
            ? static_cast<char>(std::tolower(static_cast<int>(uc)))
            : '_';
    }

    name.erase(std::unique(name.begin(), name.end(),
                           [](char a, char b) { return a == '_' && b == '_'; }),
               name.end());

    while (!name.empty() && name.front() == '_')
        name.erase(name.begin());
    while (!name.empty() && name.back() == '_')
        name.pop_back();

    if (name.empty())
        name = "simulation";
    return name;
}

std::string CaptureService::target_token(CaptureTarget target) {
    switch (target) {
        case CaptureTarget::MainWindow: return "main";
        case CaptureTarget::AlternateWindow: return "alternate";
        case CaptureTarget::BothWindows: return "both";
    }
    return "unknown";
}

std::string CaptureService::make_run_id(const CaptureRunMetadata& metadata) const {
    if (!metadata.run_id.empty())
        return normalize_stem(metadata.run_id);

    const std::string base = !metadata.simulation_name.empty()
        ? normalize_stem(metadata.simulation_name)
        : normalize_stem(metadata.scenario_name);
    return std::format("{}_{}_{}", base, metadata.simulation_index, local_timestamp());
}

std::filesystem::path CaptureService::make_manifest_path(const std::filesystem::path& run_dir,
                                                         const std::string& run_id) const {
    return unique_path(run_dir / std::format("{}_capture_manifest.md", run_id));
}

std::filesystem::path CaptureService::make_still_path(const std::filesystem::path& run_dir,
                                                      const std::string& run_id,
                                                      CaptureTarget target,
                                                      const CaptureRunMetadata& metadata) const {
    const auto sim_ms = static_cast<i32>(metadata.sim_time * f32(1000));
    const i32 sim_whole = sim_ms / 1000;
    const i32 sim_frac = std::abs(sim_ms % 1000);
    const std::string filename = std::format(
        "{}_{}_tick{:07}_t{:04}.{:03}.png",
        run_id,
        target_token(target),
        metadata.tick,
        sim_whole,
        sim_frac);
    return unique_path(run_dir / filename);
}

std::filesystem::path CaptureService::unique_path(std::filesystem::path path) {
    if (!std::filesystem::exists(path))
        return path;

    const std::filesystem::path parent = path.parent_path();
    const std::string stem = path.stem().string();
    const std::string ext = path.extension().string();
    for (u32 i = u32(2); i < u32(10000); ++i) {
        std::filesystem::path candidate = parent / std::format("{}_{:03}{}", stem, i, ext);
        if (!std::filesystem::exists(candidate))
            return candidate;
    }
    return path;
}

std::string CaptureService::local_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

void CaptureService::write_manifest(const std::filesystem::path& manifest_path,
                                    const CaptureRequest& request,
                                    const CaptureRunMetadata& metadata,
                                    std::span<const CaptureArtifact> artifacts) const {
    std::ofstream out(manifest_path, std::ios::out | std::ios::trunc);
    if (!out.is_open())
        return;

    out << std::format("# Capture: {}\n\n",
                       metadata.simulation_name.empty() ? metadata.scenario_name : metadata.simulation_name);
    out << std::format("- Run ID: {}\n", metadata.run_id);
    out << std::format("- Scenario: {}\n", metadata.scenario_name);
    out << std::format("- Simulation index: {}\n", metadata.simulation_index);
    out << std::format("- Tick: {}\n", metadata.tick);
    out << std::format("- Sim time: {:.3f}\n", metadata.sim_time);
    out << std::format("- Target: {}\n", target_token(request.target));
    out << std::format("- Mode: {}\n", mode_name(request.mode));
    if (!request.label.empty())
        out << std::format("- Label: {}\n", request.label);

    out << "\n## Artifacts\n\n";
    for (const CaptureArtifact& artifact : artifacts) {
        out << std::format("- {}: {}\n",
                           target_token(artifact.target),
                           artifact.path.filename().string());
    }

    out << "\n## Offline MP4 Conversion\n\n";
    out << "Use `tools/Convert-ToMp4.ps1` on a captured frame directory when movie frames are recorded.\n";
}

void CaptureService::write_movie_frame_manifest(const CaptureRunMetadata& metadata,
                                                const MovieFrameSequenceOptions& options,
                                                const MovieFrameSequenceStatus& status) const {
    std::ofstream out(status.manifest_path, std::ios::out | std::ios::trunc);
    if (!out.is_open())
        return;

    out << std::format("# Movie Frame Capture: {}\n\n",
                       metadata.simulation_name.empty() ? metadata.scenario_name : metadata.simulation_name);
    out << std::format("- Run ID: {}\n", metadata.run_id);
    out << std::format("- Scenario: {}\n", metadata.scenario_name);
    out << std::format("- Simulation index: {}\n", metadata.simulation_index);
    out << std::format("- Start tick: {}\n", metadata.tick);
    out << std::format("- Start sim time: {:.3f}\n", metadata.sim_time);
    out << std::format("- Target: {}\n", target_token(options.target));
    out << std::format("- FPS: {}\n", options.fps);

    out << "\n## Frame Directories\n\n";
    if (!status.main_frame_dir.empty())
        out << std::format("- Main: {}\n", status.main_frame_dir.filename().string());
    if (!status.alternate_frame_dir.empty())
        out << std::format("- Alternate: {}\n", status.alternate_frame_dir.filename().string());

    out << "\n## Offline MP4 Conversion\n\n";
    if (!status.main_frame_dir.empty()) {
        out << std::format(
            "```powershell\n.\\tools\\Convert-ToMp4.ps1 -InputPath .\\{} -FrameRate {}\n```\n\n",
            status.main_frame_dir.string(), options.fps);
    }
    if (!status.alternate_frame_dir.empty()) {
        out << std::format(
            "```powershell\n.\\tools\\Convert-ToMp4.ps1 -InputPath .\\{} -FrameRate {}\n```\n",
            status.alternate_frame_dir.string(), options.fps);
    }
}

} // namespace ndde
