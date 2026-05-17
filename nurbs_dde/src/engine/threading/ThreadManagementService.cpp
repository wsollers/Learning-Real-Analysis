#include "engine/threading/ThreadManagementService.hpp"

#include <algorithm>
#include <exception>
#include <format>

namespace ndde {

namespace {
thread_local ThreadRole t_thread_role = ThreadRole::Unknown;

bool has_capacity(u64 size, u64 capacity) noexcept {
    return capacity == u64(0) || size < capacity;
}
} // namespace

ThreadJobContext::ThreadJobContext(ThreadManagementService& service,
                                   ThreadJobId job_id,
                                   std::stop_token stop_token) noexcept
    : m_service(&service)
    , m_job_id(job_id)
    , m_stop_token(stop_token)
{}

void ThreadJobContext::publish_worker_record(events::EventRecord record) const {
    if (!m_service) return;
    record.id_a = record.id_a == u64(0) ? m_job_id.value : record.id_a;
    m_service->enqueue_event(record);
}

void ThreadJobContext::report_diagnostic(DiagnosticReport report) const {
    if (!m_service) return;
    if (!report.source.component.has_value()) {
        report.source.component = ids::unknown_component;
    }
    m_service->enqueue_diagnostic(std::move(report));
}

void ThreadJobContext::log(LogSeverity severity,
                           LogCategory category,
                           std::string_view message) const {
    if (!m_service) return;
    m_service->enqueue_log(ThreadManagementService::QueuedLog{
        .severity = severity,
        .category = category,
        .source = {},
        .message = std::string{message}
    });
}

void ThreadJobContext::complete(ThreadJobResult result) const {
    if (!m_service) return;
    m_service->push_result(std::move(result));
}

ThreadManagementService::~ThreadManagementService() {
    shutdown();
}

void ThreadManagementService::init(ThreadPoolConfig config, ThreadServiceBindings bindings) {
    shutdown();

    m_config = config;
    m_bindings = bindings;
    m_main_thread_id = std::this_thread::get_id();
    t_thread_role = ThreadRole::Main;
    m_accepting_jobs = true;
    m_shutting_down = false;
    m_initialised = true;

    u32 worker_count = u32(0);
    if (m_config.enable_background_workers) {
        worker_count = m_config.worker_count == u32(0)
            ? default_worker_count()
            : m_config.worker_count;
    }

    m_workers.reserve(worker_count);
    for (u32 index = u32(0); index < worker_count; ++index) {
        m_workers.emplace_back([this, worker_index = u64(index + u32(1))]
                               (std::stop_token stop) noexcept {
            worker_loop(stop, worker_index);
        });
    }

    if (m_config.enable_logger_thread && m_bindings.logger) {
        m_logger_thread.emplace([this](std::stop_token stop) noexcept {
            logger_loop(stop);
        });
    }
}

void ThreadManagementService::shutdown() noexcept {
    {
        std::scoped_lock lock(m_mutex);
        if (!m_initialised && m_workers.empty()) return;
        m_accepting_jobs = false;
        m_shutting_down = true;
        for (JobRecord& job : m_jobs) {
            if (job.status.cancellable &&
                (job.status.state == ThreadJobState::Queued ||
                 job.status.state == ThreadJobState::Running ||
                 job.status.state == ThreadJobState::CancelRequested)) {
                job.stop_source.request_stop();
                job.status.state = ThreadJobState::CancelRequested;
            }
        }
    }

    m_work_available.notify_all();
    m_log_available.notify_all();
    if (m_logger_thread) {
        m_logger_thread->request_stop();
        m_log_available.notify_all();
        m_logger_thread.reset();
    }
    m_workers.clear();

    {
        std::scoped_lock lock(m_mutex);
        for (JobRecord& job : m_jobs) {
            if (job.status.state == ThreadJobState::Queued ||
                job.status.state == ThreadJobState::CancelRequested) {
                job.status.state = ThreadJobState::Cancelled;
            }
        }
        m_pending.clear();
        m_shutting_down = false;
        m_initialised = false;
    }
}

ThreadJobId ThreadManagementService::submit(ThreadJobDescriptor descriptor, ThreadJobFn job) {
    if (!job) return {};

    std::scoped_lock lock(m_mutex);
    if (!m_initialised || !m_accepting_jobs) return {};
    if (!has_capacity(static_cast<u64>(m_pending.size()), m_config.max_queued_jobs)) {
        ++m_dropped_events;
        return {};
    }

    const ThreadJobId id = descriptor.id.value == u64(0)
        ? ThreadJobId{m_next_job_id++}
        : descriptor.id;

    JobRecord record;
    record.status = ThreadJobStatus{
        .id = id,
        .owner = descriptor.owner,
        .node = descriptor.node,
        .priority = descriptor.priority,
        .state = ThreadJobState::Queued,
        .cancellable = descriptor.cancellable,
        .queued_order = m_next_queue_order++
    };

    m_jobs.push_back(std::move(record));
    m_pending.push_back(PendingJob{.id = id, .fn = std::move(job)});
    m_work_available.notify_one();
    return id;
}

void ThreadManagementService::request_cancel(ThreadJobId id) noexcept {
    std::scoped_lock lock(m_mutex);
    JobRecord* job = find_job_locked(id);
    if (!job || !job->status.cancellable) return;
    if (job->status.state == ThreadJobState::Completed ||
        job->status.state == ThreadJobState::Cancelled ||
        job->status.state == ThreadJobState::Failed) {
        return;
    }
    job->stop_source.request_stop();
    job->status.state = ThreadJobState::CancelRequested;
}

void ThreadManagementService::request_cancel_all() noexcept {
    std::scoped_lock lock(m_mutex);
    for (JobRecord& job : m_jobs) {
        if (!job.status.cancellable) continue;
        if (job.status.state == ThreadJobState::Completed ||
            job.status.state == ThreadJobState::Cancelled ||
            job.status.state == ThreadJobState::Failed) {
            continue;
        }
        job.stop_source.request_stop();
        job.status.state = ThreadJobState::CancelRequested;
    }
}

std::optional<ThreadJobStatus> ThreadManagementService::status(ThreadJobId id) const {
    std::scoped_lock lock(m_mutex);
    const JobRecord* job = find_job_locked(id);
    if (!job) return std::nullopt;
    return job->status;
}

std::span<const ThreadJobStatus> ThreadManagementService::jobs() const {
    std::scoped_lock lock(m_mutex);
    m_job_status_view.clear();
    m_job_status_view.reserve(m_jobs.size());
    for (const JobRecord& job : m_jobs) {
        m_job_status_view.push_back(job.status);
    }
    return m_job_status_view;
}

std::vector<ThreadJobResult> ThreadManagementService::consume_completed_results() {
    std::scoped_lock lock(m_mutex);
    std::vector<ThreadJobResult> out;
    out.swap(m_completed_results);
    return out;
}

void ThreadManagementService::drain_service_mailboxes() {
    std::vector<QueuedLog> logs;
    std::vector<DiagnosticReport> diagnostics;
    std::vector<events::EventRecord> events;
    {
        std::scoped_lock lock(m_mutex);
        logs.swap(m_log_mailbox);
        diagnostics.swap(m_diagnostic_mailbox);
        events.swap(m_event_mailbox);
    }

    if (m_bindings.logger && !m_logger_thread) {
        for (const QueuedLog& log : logs) {
            (void)m_bindings.logger->write(log.severity, log.category, log.source, log.message);
        }
    }

    if (m_bindings.diagnostics) {
        for (DiagnosticReport& report : diagnostics) {
            (void)m_bindings.diagnostics->report(std::move(report));
        }
    }

    if (m_bindings.events) {
        for (events::EventRecord& record : events) {
            (void)m_bindings.events->enqueue_worker_record(record);
        }
        (void)m_bindings.events->drain_worker_mailbox();
    }
}

ThreadStats ThreadManagementService::stats() const {
    std::scoped_lock lock(m_mutex);
    return ThreadStats{
        .worker_count = static_cast<u32>(m_workers.size()),
        .queued_jobs = static_cast<u64>(m_pending.size()),
        .completed_results = static_cast<u64>(m_completed_results.size()),
        .dropped_results = m_dropped_results,
        .dropped_logs = m_dropped_logs,
        .dropped_diagnostics = m_dropped_diagnostics,
        .dropped_events = m_dropped_events
    };
}

bool ThreadManagementService::is_main_thread() const noexcept {
    return std::this_thread::get_id() == m_main_thread_id;
}

ThreadRole ThreadManagementService::current_thread_role() const noexcept {
    return t_thread_role;
}

void ThreadManagementService::worker_loop(std::stop_token service_stop, u64 worker_index) noexcept {
    t_thread_role = ThreadRole::Worker;
    while (!service_stop.stop_requested()) {
        PendingJob pending = wait_for_job(service_stop);
        if (!pending.fn) return;

        JobRecord* job_record = nullptr;
        std::stop_token job_stop;
        {
            std::scoped_lock lock(m_mutex);
            job_record = find_job_locked(pending.id);
            if (!job_record) continue;
            if (job_record->status.state == ThreadJobState::CancelRequested ||
                job_record->stop_source.stop_requested()) {
                job_record->status.state = ThreadJobState::Cancelled;
                continue;
            }
            job_record->status.state = ThreadJobState::Running;
            job_record->status.worker_index = worker_index;
            job_stop = job_record->stop_source.get_token();
        }

        ThreadJobContext context{*this, pending.id, job_stop};
        try {
            pending.fn(context);
            std::scoped_lock lock(m_mutex);
            JobRecord* finished = find_job_locked(pending.id);
            if (finished) {
                finished->status.state = finished->stop_source.stop_requested()
                    ? ThreadJobState::Cancelled
                    : ThreadJobState::Completed;
            }
        } catch (const std::exception& ex) {
            set_state(pending.id, ThreadJobState::Failed, worker_index);
            report_thread_fault(pending.id, ex.what());
        } catch (...) {
            set_state(pending.id, ThreadJobState::Failed, worker_index);
            report_thread_fault(pending.id, "worker job threw an unknown exception");
        }
    }
}

void ThreadManagementService::logger_loop(std::stop_token service_stop) noexcept {
    t_thread_role = ThreadRole::Logger;
    while (!service_stop.stop_requested()) {
        std::vector<QueuedLog> logs = wait_for_logs(service_stop);
        if (logs.empty()) {
            continue;
        }
        if (!m_bindings.logger) {
            continue;
        }
        for (const QueuedLog& log : logs) {
            (void)m_bindings.logger->write(log.severity, log.category, log.source, log.message);
        }
        m_bindings.logger->drain_sinks();
    }

    for (;;) {
        std::vector<QueuedLog> logs;
        {
            std::scoped_lock lock(m_mutex);
            if (m_log_mailbox.empty()) {
                break;
            }
            logs.swap(m_log_mailbox);
        }
        if (!m_bindings.logger) {
            continue;
        }
        for (const QueuedLog& log : logs) {
            (void)m_bindings.logger->write(log.severity, log.category, log.source, log.message);
        }
        m_bindings.logger->drain_sinks();
    }
}

ThreadManagementService::PendingJob
ThreadManagementService::wait_for_job(std::stop_token service_stop) {
    std::unique_lock lock(m_mutex);
    m_work_available.wait(lock, [this, service_stop] {
        return service_stop.stop_requested() || m_shutting_down || !m_pending.empty();
    });
    if (service_stop.stop_requested() || m_shutting_down || m_pending.empty()) {
        return {};
    }

    auto best = m_pending.begin();
    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        const JobRecord* lhs = find_job_locked(it->id);
        const JobRecord* rhs = find_job_locked(best->id);
        if (!lhs || !rhs) continue;
        if (static_cast<u8>(lhs->status.priority) > static_cast<u8>(rhs->status.priority)) {
            best = it;
        }
    }

    PendingJob job = std::move(*best);
    m_pending.erase(best);
    return job;
}

std::vector<ThreadManagementService::QueuedLog>
ThreadManagementService::wait_for_logs(std::stop_token service_stop) {
    std::unique_lock lock(m_mutex);
    m_log_available.wait(lock, [this, service_stop] {
        return service_stop.stop_requested() || !m_log_mailbox.empty();
    });

    std::vector<QueuedLog> logs;
    logs.swap(m_log_mailbox);
    return logs;
}

ThreadManagementService::JobRecord*
ThreadManagementService::find_job_locked(ThreadJobId id) noexcept {
    auto it = std::find_if(m_jobs.begin(), m_jobs.end(), [id](const JobRecord& job) {
        return job.status.id == id;
    });
    return it == m_jobs.end() ? nullptr : &*it;
}

const ThreadManagementService::JobRecord*
ThreadManagementService::find_job_locked(ThreadJobId id) const noexcept {
    auto it = std::find_if(m_jobs.begin(), m_jobs.end(), [id](const JobRecord& job) {
        return job.status.id == id;
    });
    return it == m_jobs.end() ? nullptr : &*it;
}

u32 ThreadManagementService::default_worker_count() noexcept {
    const u32 hardware = static_cast<u32>(std::max(1u, std::thread::hardware_concurrency()));
    if (hardware <= u32(2)) return u32(1);
    return std::min<u32>(hardware - u32(1), u32(8));
}

void ThreadManagementService::set_state(ThreadJobId id,
                                        ThreadJobState state,
                                        u64 worker_index) noexcept {
    std::scoped_lock lock(m_mutex);
    JobRecord* job = find_job_locked(id);
    if (!job) return;
    job->status.state = state;
    if (worker_index != u64(0)) {
        job->status.worker_index = worker_index;
    }
}

void ThreadManagementService::push_result(ThreadJobResult result) {
    std::scoped_lock lock(m_mutex);
    if (!has_capacity(static_cast<u64>(m_completed_results.size()), m_config.max_completed_results)) {
        ++m_dropped_results;
        return;
    }
    m_completed_results.push_back(std::move(result));
}

void ThreadManagementService::enqueue_log(QueuedLog log) {
    std::scoped_lock lock(m_mutex);
    if (!has_capacity(static_cast<u64>(m_log_mailbox.size()), m_config.max_mailbox_records)) {
        ++m_dropped_logs;
        return;
    }
    m_log_mailbox.push_back(std::move(log));
    m_log_available.notify_one();
}

void ThreadManagementService::enqueue_diagnostic(DiagnosticReport report) {
    std::scoped_lock lock(m_mutex);
    if (!has_capacity(static_cast<u64>(m_diagnostic_mailbox.size()), m_config.max_mailbox_records)) {
        ++m_dropped_diagnostics;
        return;
    }
    m_diagnostic_mailbox.push_back(std::move(report));
}

void ThreadManagementService::enqueue_event(events::EventRecord record) {
    std::scoped_lock lock(m_mutex);
    if (!has_capacity(static_cast<u64>(m_event_mailbox.size()), m_config.max_mailbox_records)) {
        ++m_dropped_events;
        return;
    }
    m_event_mailbox.push_back(record);
}

void ThreadManagementService::report_thread_fault(ThreadJobId id, std::string message) noexcept {
    DiagnosticReport report;
    report.severity = DiagnosticSeverity::Error;
    report.lifetime = DiagnosticLifetime::UntilResolved;
    report.code = ErrorCode::ThreadFault;
    report.source.subsystem = DiagnosticSubsystem::Threading;
    report.title = "Worker job failed";
    report.message = std::format("Worker job {} failed: {}", id.value, message);
    report.suggested_fix = "Inspect the job owner and make the worker path cancellable and exception-safe.";
    enqueue_diagnostic(std::move(report));
}

} // namespace ndde
