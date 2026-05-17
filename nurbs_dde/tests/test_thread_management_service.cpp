#include "engine/SimulationHost.hpp"
#include "engine/threading/ThreadManagementService.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <variant>

namespace {

using namespace ndde;
using namespace std::chrono_literals;

[[nodiscard]] bool wait_for_state(ThreadManagementService& threads,
                                  ThreadJobId id,
                                  ThreadJobState state,
                                  std::chrono::milliseconds timeout = 2s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto status = threads.status(id);
        if (status && status->state == state) {
            return true;
        }
        std::this_thread::sleep_for(5ms);
    }
    return false;
}

TEST(ThreadManagementService, InitShutdownWithZeroWorkersIsSafe) {
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(0), .enable_background_workers = false});

    const ThreadJobId id = threads.submit(ThreadJobDescriptor{}, [](ThreadJobContext&) {});
    ASSERT_NE(id.value, u64(0));
    EXPECT_EQ(threads.status(id)->state, ThreadJobState::Queued);

    threads.shutdown();
    EXPECT_FALSE(threads.initialised());
    EXPECT_EQ(threads.status(id)->state, ThreadJobState::Cancelled);
}

TEST(ThreadManagementService, SubmittedJobCompletesAndReturnsResult) {
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(1)});

    const ThreadJobId id = threads.submit(ThreadJobDescriptor{}, [](ThreadJobContext& context) {
        context.complete(ResourceId{u64(902)});
    });

    ASSERT_TRUE(wait_for_state(threads, id, ThreadJobState::Completed));
    const auto results = threads.consume_completed_results();
    ASSERT_EQ(results.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<ResourceId>(results.front()));
    EXPECT_EQ(std::get<ResourceId>(results.front()).value, u64(902));
}

TEST(ThreadManagementService, CancellationIsObservableThroughContext) {
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(1)});
    std::atomic<bool> started = false;

    const ThreadJobId id = threads.submit(ThreadJobDescriptor{}, [&started](ThreadJobContext& context) {
        started.store(true);
        while (!context.stop_requested()) {
            std::this_thread::sleep_for(1ms);
        }
    });

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!started.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(1ms);
    }
    ASSERT_TRUE(started.load());

    threads.request_cancel(id);
    ASSERT_TRUE(wait_for_state(threads, id, ThreadJobState::Cancelled));
}

TEST(ThreadManagementService, RequireThreadRolePassesOnMainThread) {
    DiagnosticsService diagnostics;
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(0), .enable_background_workers = false},
                 ThreadServiceBindings{.diagnostics = &diagnostics});

    EXPECT_TRUE(threads.require_thread_role(ThreadRole::Main, "main-only api"));
    threads.drain_service_mailboxes();
    EXPECT_TRUE(diagnostics.active().empty());
}

TEST(ThreadManagementService, RequireThreadRoleReportsWorkerViolation) {
    DiagnosticsService diagnostics;
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(1)},
                 ThreadServiceBindings{.diagnostics = &diagnostics});

    std::atomic<bool> allowed = true;
    const ThreadJobId id = threads.submit(ThreadJobDescriptor{}, [&threads, &allowed](ThreadJobContext&) {
        allowed.store(threads.require_thread_role(ThreadRole::Main, "main-only api"));
    });

    ASSERT_TRUE(wait_for_state(threads, id, ThreadJobState::Completed));
    threads.drain_service_mailboxes();

    EXPECT_FALSE(allowed.load());
    ASSERT_EQ(diagnostics.active().size(), 1u);
    EXPECT_EQ(diagnostics.active().front().code, ErrorCode::ThreadRoleViolation);
    EXPECT_NE(diagnostics.active().front().message.find("expected Main thread"), std::string::npos);
    EXPECT_NE(diagnostics.active().front().message.find("Worker thread"), std::string::npos);
}

TEST(ThreadManagementService, WorkerMailboxesDrainIntoBoundServices) {
    DiagnosticsService diagnostics;
    EventBusService events;
    LoggerService logger;
    events.init();
    logger.init();

    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(1)},
                 ThreadServiceBindings{
                     .diagnostics = &diagnostics,
                     .events = &events,
                     .logger = &logger
                 });

    const ThreadJobId id = threads.submit(ThreadJobDescriptor{}, [](ThreadJobContext& context) {
        context.log(LogSeverity::Info, LogCategory::Worker, "worker says hello");

        DiagnosticReport report;
        report.severity = DiagnosticSeverity::Warning;
        report.code = ErrorCode::ThreadFault;
        report.source.subsystem = DiagnosticSubsystem::Threading;
        report.title = "thread warning";
        report.message = "worker diagnostic";
        context.report_diagnostic(std::move(report));

        events::EventRecord record;
        record.kind = events::EventKind::AlertCustom;
        record.severity = events::EventSeverity::Notice;
        record.set_label("WorkerJob");
        context.publish_worker_record(record);
    });

    ASSERT_TRUE(wait_for_state(threads, id, ThreadJobState::Completed));
    threads.drain_service_mailboxes();
    events.drain(EventChannelId::Worker, f32(0), u64(0));

    ASSERT_EQ(logger.records().size(), 1u);
    EXPECT_EQ(logger.message(logger.records().front().id), "worker says hello");
    ASSERT_EQ(diagnostics.active().size(), 1u);
    EXPECT_EQ(diagnostics.active().front().message, "worker diagnostic");
    ASSERT_EQ(events.log(EventChannelId::Worker).entries().size(), 1u);
}

TEST(ThreadManagementService, EventMailboxDrainsPublishedChannel) {
    EventBusService events;
    events.init();

    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(0)},
                 ThreadServiceBindings{.events = &events});

    events::EventRecord record;
    record.kind = events::EventKind::AppStarted;
    record.severity = events::EventSeverity::Info;
    record.set_label("AppChannelRecord");

    threads.publish_event_record(EventChannelId::App, record);
    threads.drain_service_mailboxes();
    events.drain(EventChannelId::App, f32(0), u64(0));

    ASSERT_EQ(events.log(EventChannelId::App).entries().size(), 1u);
    EXPECT_EQ(events.log(EventChannelId::App).entries().front().kind, events::EventKind::AppStarted);
}

TEST(ThreadManagementService, LoggerThreadDrainsWorkerLogsAsynchronously) {
    LoggerService logger;
    logger.init();

    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{
                     .worker_count = u32(1),
                     .enable_logger_thread = true
                 },
                 ThreadServiceBindings{.logger = &logger});

    const ThreadJobId id = threads.submit(ThreadJobDescriptor{}, [](ThreadJobContext& context) {
        context.log(LogSeverity::Info, LogCategory::Worker, "async worker log");
    });

    ASSERT_TRUE(wait_for_state(threads, id, ThreadJobState::Completed));

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (logger.snapshot().empty() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(5ms);
    }

    const auto snapshot = logger.snapshot();
    ASSERT_EQ(snapshot.size(), 1u);
    EXPECT_EQ(snapshot.front().message, "async worker log");
}

TEST(ThreadManagementService, WorkerExceptionBecomesDiagnostic) {
    DiagnosticsService diagnostics;
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(1)},
                 ThreadServiceBindings{.diagnostics = &diagnostics});

    const ThreadJobId id = threads.submit(ThreadJobDescriptor{}, [](ThreadJobContext&) {
        throw std::runtime_error{"kaboom"};
    });

    ASSERT_TRUE(wait_for_state(threads, id, ThreadJobState::Failed));
    threads.drain_service_mailboxes();

    ASSERT_EQ(diagnostics.active().size(), 1u);
    EXPECT_EQ(diagnostics.active().front().code, ErrorCode::ThreadFault);
}

TEST(ThreadManagementService, QueueOverflowRejectsNewJobs) {
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{
        .worker_count = u32(0),
        .max_queued_jobs = u64(1),
        .enable_background_workers = false
    });

    const ThreadJobId first = threads.submit(ThreadJobDescriptor{}, [](ThreadJobContext&) {});
    const ThreadJobId second = threads.submit(ThreadJobDescriptor{}, [](ThreadJobContext&) {});

    EXPECT_NE(first.value, u64(0));
    EXPECT_EQ(second.value, u64(0));
    EXPECT_EQ(threads.dropped_events(), u64(1));
}

TEST(ThreadManagementService, SimulationCommandQueuePreservesOrderAndDrains) {
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{
        .worker_count = u32(0),
        .max_queued_jobs = u64(3),
        .enable_background_workers = false
    });

    EXPECT_TRUE(threads.enqueue_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::Pause
    }));
    EXPECT_TRUE(threads.enqueue_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::Tick,
        .tick = TickInfo{.tick_index = u64(7), .dt = f32(0.25f), .time = f32(1.5f)}
    }));
    EXPECT_TRUE(threads.enqueue_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::SwitchSimulation,
        .simulation_index = u64(2)
    }));
    EXPECT_FALSE(threads.enqueue_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::Resume
    }));

    const auto commands = threads.consume_simulation_commands();
    ASSERT_EQ(commands.size(), 3u);
    EXPECT_EQ(commands[0].kind, SimulationThreadCommandKind::Pause);
    EXPECT_EQ(commands[1].kind, SimulationThreadCommandKind::Tick);
    EXPECT_EQ(commands[1].tick.tick_index, u64(7));
    EXPECT_EQ(commands[2].kind, SimulationThreadCommandKind::SwitchSimulation);
    EXPECT_EQ(commands[2].simulation_index, u64(2));
    EXPECT_TRUE(threads.consume_simulation_commands().empty());
}

TEST(ThreadManagementService, SimulationSnapshotMailboxKeepsLatestImmutableCopy) {
    ndde::memory::MemoryService memory;
    memory.begin_frame();

    SceneSnapshot scene;
    scene.name = "Sim A";
    scene.paused = true;
    scene.sim_time = 3.25f;
    scene.sim_speed = 2.0f;
    scene.status = "Running";
    scene.particles = memory.frame().make_vector<ParticleSnapshot>();
    scene.particles.push_back(ParticleSnapshot{.id = 11u, .label = "first", .x = 1.f});
    scene.particle_count = scene.particles.size();

    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{.worker_count = u32(0), .enable_background_workers = false});
    threads.publish_simulation_snapshot(make_simulation_render_snapshot(scene));

    scene.name = "mutated";
    scene.particles.front().label = "mutated";

    const auto snapshot = threads.latest_simulation_snapshot();
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->name, "Sim A");
    EXPECT_TRUE(snapshot->paused);
    EXPECT_FLOAT_EQ(snapshot->sim_time, f32(3.25f));
    ASSERT_EQ(snapshot->particles.size(), 1u);
    EXPECT_EQ(snapshot->particles.front().label, "first");

    threads.publish_simulation_snapshot(SimulationRenderSnapshot{.name = "Sim B"});
    const auto latest = threads.latest_simulation_snapshot();
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->name, "Sim B");
}

TEST(ThreadManagementService, SimulationThreadConsumesCommandBatches) {
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{
        .worker_count = u32(0),
        .enable_background_workers = false
    });

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<SimulationThreadCommandKind> seen;
    ThreadRole callback_role = ThreadRole::Unknown;

    ASSERT_TRUE(threads.start_simulation_thread(
        [&threads, &mutex, &cv, &seen, &callback_role]
        (std::stop_token, std::span<const SimulationThreadCommand> commands) {
            {
                std::scoped_lock lock(mutex);
                callback_role = threads.current_thread_role();
                for (const SimulationThreadCommand& command : commands) {
                    seen.push_back(command.kind);
                }
            }
            cv.notify_one();
        }));
    EXPECT_TRUE(threads.simulation_thread_running());
    EXPECT_FALSE(threads.start_simulation_thread([](std::stop_token, std::span<const SimulationThreadCommand>) {}));

    ASSERT_TRUE(threads.enqueue_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::Pause
    }));
    ASSERT_TRUE(threads.enqueue_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::Resume
    }));

    std::unique_lock lock(mutex);
    ASSERT_TRUE(cv.wait_for(lock, 2s, [&seen] { return seen.size() >= 2u; }));
    EXPECT_EQ(callback_role, ThreadRole::Simulation);
    EXPECT_EQ(seen[0], SimulationThreadCommandKind::Pause);
    EXPECT_EQ(seen[1], SimulationThreadCommandKind::Resume);
    lock.unlock();

    threads.stop_simulation_thread();
    EXPECT_FALSE(threads.simulation_thread_running());
}

TEST(ThreadManagementService, RequireThreadRolePassesOnSimulationThread) {
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{
        .worker_count = u32(0),
        .enable_background_workers = false
    });

    std::mutex mutex;
    std::condition_variable cv;
    bool checked = false;
    bool allowed = false;

    ASSERT_TRUE(threads.start_simulation_thread(
        [&threads, &mutex, &cv, &checked, &allowed]
        (std::stop_token, std::span<const SimulationThreadCommand>) {
            {
                std::scoped_lock lock(mutex);
                allowed = threads.require_thread_role(ThreadRole::Simulation, "simulation-only api");
                checked = true;
            }
            cv.notify_one();
        }));

    ASSERT_TRUE(threads.enqueue_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::Tick
    }));

    std::unique_lock lock(mutex);
    ASSERT_TRUE(cv.wait_for(lock, 2s, [&checked] { return checked; }));
    EXPECT_TRUE(allowed);
    lock.unlock();

    threads.stop_simulation_thread();
}

TEST(ThreadManagementService, SimulationThreadExceptionBecomesDiagnostic) {
    DiagnosticsService diagnostics;
    ThreadManagementService threads;
    threads.init(ThreadPoolConfig{
                     .worker_count = u32(0),
                     .enable_background_workers = false
                 },
                 ThreadServiceBindings{.diagnostics = &diagnostics});

    ASSERT_TRUE(threads.start_simulation_thread(
        [](std::stop_token, std::span<const SimulationThreadCommand>) {
            throw std::runtime_error{"simulation boom"};
        }));
    ASSERT_TRUE(threads.enqueue_simulation_command(SimulationThreadCommand{
        .kind = SimulationThreadCommandKind::Tick
    }));

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (diagnostics.active().empty() && std::chrono::steady_clock::now() < deadline) {
        threads.drain_service_mailboxes();
        std::this_thread::sleep_for(5ms);
    }

    threads.stop_simulation_thread();
    threads.drain_service_mailboxes();
    ASSERT_FALSE(diagnostics.active().empty());
    EXPECT_EQ(diagnostics.active().front().code, ErrorCode::ThreadFault);
}

TEST(RenderService, WorkerThreadMutationReportsThreadRoleViolation) {
    EngineServices services;
    std::atomic<bool> registered = true;

    const ThreadJobId id = services.threads().submit(
        ThreadJobDescriptor{},
        [&services, &registered](ThreadJobContext&) {
            RenderViewId view_id = u64(0);
            auto handle = services.render().register_view(
                RenderViewDescriptor{.title = "Worker View"},
                &view_id);
            registered.store(static_cast<bool>(handle));
        });

    ASSERT_TRUE(wait_for_state(services.threads(), id, ThreadJobState::Completed));
    services.threads().drain_service_mailboxes();

    EXPECT_FALSE(registered.load());
    EXPECT_EQ(services.render().active_view_count(), 0u);
    ASSERT_FALSE(services.diagnostics().active().empty());
    EXPECT_EQ(services.diagnostics().active().back().code, ErrorCode::ThreadRoleViolation);
}

TEST(EngineGuiServices, WorkerThreadMutationReportsThreadRoleViolations) {
    EngineServices services;
    std::atomic<bool> panel_registered = true;
    std::atomic<bool> hotkey_registered = true;

    const ThreadJobId id = services.threads().submit(
        ThreadJobDescriptor{},
        [&services, &panel_registered, &hotkey_registered](ThreadJobContext&) {
            auto panel = services.panels().register_panel(PanelDescriptor{
                .title = "Worker Panel",
                .draw = [] {}
            });
            auto hotkey = services.hotkeys().register_action(HotkeyDescriptor{
                .chord = {.key = 77, .mods = 0},
                .label = "Worker Hotkey",
                .callback = [] {}
            });
            services.interaction().set_mouse(u64(42), Vec2{10.f, 20.f}, Vec2{}, true);

            panel_registered.store(static_cast<bool>(panel));
            hotkey_registered.store(static_cast<bool>(hotkey));
        });

    ASSERT_TRUE(wait_for_state(services.threads(), id, ThreadJobState::Completed));
    services.threads().drain_service_mailboxes();

    EXPECT_FALSE(panel_registered.load());
    EXPECT_FALSE(hotkey_registered.load());
    EXPECT_EQ(services.panels().active_count(), 0u);
    EXPECT_EQ(services.hotkeys().active_count(), 0u);
    EXPECT_FALSE(services.interaction().mouse_state(u64(42)).enabled);

    const auto role_violations =
        services.diagnostics().active_with(ErrorCode::ThreadRoleViolation);
    ASSERT_EQ(role_violations.size(), 1u);
    EXPECT_GE(role_violations.front().occurrence_count, u64(3));
}

TEST(CameraService, WorkerThreadMutationReportsThreadRoleViolation) {
    EngineServices services;
    RenderViewId view_id = u64(0);
    auto view = services.render().register_view(RenderViewDescriptor{
        .title = "Camera View",
        .kind = RenderViewKind::Main
    }, &view_id);
    ASSERT_NE(view_id, u64(0));

    const CameraState before = services.camera().camera(view_id);

    const ThreadJobId id = services.threads().submit(
        ThreadJobDescriptor{},
        [&services](ThreadJobContext&) {
            services.camera().orbit_main(f32(120), f32(10));
        });

    ASSERT_TRUE(wait_for_state(services.threads(), id, ThreadJobState::Completed));
    services.threads().drain_service_mailboxes();

    const CameraState after = services.camera().camera(view_id);
    EXPECT_FLOAT_EQ(after.yaw, before.yaw);
    EXPECT_FLOAT_EQ(after.pitch, before.pitch);

    const auto role_violations =
        services.diagnostics().active_with(ErrorCode::ThreadRoleViolation);
    ASSERT_EQ(role_violations.size(), 1u);
    EXPECT_NE(role_violations.front().message.find("CameraService::orbit_main"), std::string::npos);
}

TEST(DiagnosticsService, WorkerThreadDirectReportUsesThreadRoleViolation) {
    EngineServices services;
    std::atomic<u64> reported_id = u64(999);

    const ThreadJobId id = services.threads().submit(
        ThreadJobDescriptor{},
        [&services, &reported_id](ThreadJobContext&) {
            const DiagnosticId diagnostic = services.diagnostics().report(DiagnosticReport{
                .severity = DiagnosticSeverity::Error,
                .code = ErrorCode::InvalidParameter,
                .title = "Worker direct diagnostic"
            });
            reported_id.store(diagnostic.value);
        });

    ASSERT_TRUE(wait_for_state(services.threads(), id, ThreadJobState::Completed));
    services.threads().drain_service_mailboxes();

    EXPECT_EQ(reported_id.load(), u64(0));
    EXPECT_TRUE(services.diagnostics().active_with(ErrorCode::InvalidParameter).empty());

    const auto role_violations =
        services.diagnostics().active_with(ErrorCode::ThreadRoleViolation);
    ASSERT_EQ(role_violations.size(), 1u);
    EXPECT_NE(role_violations.front().message.find("DiagnosticsService::report"), std::string::npos);
}

TEST(EngineServices, OwnsThreadManagementServiceAndPassesItToSimulationHost) {
    EngineServices services;
    SimulationHost host = services.simulation_host();

    EXPECT_EQ(&host.threads(), &services.threads());
    EXPECT_TRUE(services.threads().initialised());
    EXPECT_TRUE(services.threads().is_main_thread());
}

} // namespace
