#include "engine/SimulationHost.hpp"
#include "engine/threading/ThreadManagementService.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
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

TEST(EngineServices, OwnsThreadManagementServiceAndPassesItToSimulationHost) {
    EngineServices services;
    SimulationHost host = services.simulation_host();

    EXPECT_EQ(&host.threads(), &services.threads());
    EXPECT_TRUE(services.threads().initialised());
    EXPECT_TRUE(services.threads().is_main_thread());
}

} // namespace
