#pragma once
// engine/threading/ThreadTypes.hpp
// Engine-owned thread roles, worker job descriptors, status records, and results.

#include "engine/RuntimeIds.hpp"
#include "engine/diagnostics/DiagnosticsTypes.hpp"
#include "simulation/events/EventRecord.hpp"

#include <chrono>
#include <string>
#include <variant>

namespace ndde {

struct ThreadJobId {
    u64 value = u64(0);

    friend constexpr bool operator==(ThreadJobId, ThreadJobId) noexcept = default;
};

enum class ThreadRole : u8 {
    Main,
    Gui,
    Simulation,
    Renderer,
    Logger,
    Worker,
    Io,
    Telemetry,
    Unknown
};

enum class ThreadJobPriority : u8 {
    Low,
    Normal,
    High
};

enum class ThreadJobState : u8 {
    Queued,
    Running,
    Completed,
    CancelRequested,
    Cancelled,
    Failed
};

struct ThreadJobDescriptor {
    ThreadJobId id = {};
    ComponentId owner = ids::unknown_component;
    RuntimeNodeId node = {};
    ThreadJobPriority priority = ThreadJobPriority::Normal;
    bool cancellable = true;
};

struct ThreadJobStatus {
    ThreadJobId id = {};
    ComponentId owner = ids::unknown_component;
    RuntimeNodeId node = {};
    ThreadJobPriority priority = ThreadJobPriority::Normal;
    ThreadJobState state = ThreadJobState::Queued;
    bool cancellable = true;
    u64 queued_order = u64(0);
    u64 worker_index = u64(0);
};

struct GeometryBuildResult {
    ThreadJobId job = {};
    ResourceId resource = {};
    u64 vertex_count = u64(0);
    u64 index_count = u64(0);
};

struct ValidationJobResult {
    ThreadJobId job = {};
    RuntimeNodeId scenario = {};
    u64 warning_count = u64(0);
    u64 error_count = u64(0);
};

struct ThreadTextResult {
    ThreadJobId job = {};
    std::string message;
};

using ThreadJobResult = std::variant<
    GeometryBuildResult,
    ValidationJobResult,
    DiagnosticId,
    ResourceId,
    ThreadTextResult
>;

struct ThreadPoolConfig {
    u32 worker_count = u32(0);
    u64 max_queued_jobs = u64(256);
    u64 max_completed_results = u64(256);
    u64 max_mailbox_records = u64(512);
    bool enable_simulation_thread = false;
    bool enable_render_thread = false;
    bool enable_logger_thread = false;
    bool enable_background_workers = true;
};

struct ThreadStats {
    u32 worker_count = u32(0);
    u64 queued_jobs = u64(0);
    u64 completed_results = u64(0);
    u64 dropped_results = u64(0);
    u64 dropped_logs = u64(0);
    u64 dropped_diagnostics = u64(0);
    u64 dropped_events = u64(0);
};

} // namespace ndde
