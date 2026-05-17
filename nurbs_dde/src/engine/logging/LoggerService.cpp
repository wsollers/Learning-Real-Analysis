#include "engine/logging/LoggerService.hpp"

#include <algorithm>

namespace ndde {

void LoggerService::init(LoggerConfig config) {
    shutdown();
    m_config = config;
    if (m_config.max_records == u64(0)) {
        m_config.max_records = u64(1);
    }
    m_store.reserve(static_cast<std::size_t>(std::min<u64>(m_config.max_records, u64(8192))));
    m_record_view.reserve(m_store.capacity());
    m_initialised = true;
}

void LoggerService::shutdown() noexcept {
    clear();
    m_config = {};
    m_next_id = u64(1);
    m_dropped_records = u64(0);
    m_dropped_string_bytes = u64(0);
    m_initialised = false;
}

LogRecordId LoggerService::write(LogSeverity severity,
                                 LogCategory category,
                                 LogSourceRef source,
                                 std::string_view message) {
    LogRecord record;
    record.severity = severity;
    record.category = category;
    record.source = source;
    return append(record, message);
}

LogRecordId LoggerService::write_event(EventRef event,
                                       LogSeverity severity,
                                       LogCategory category,
                                       std::string_view message) {
    LogRecord record;
    record.severity = severity;
    record.category = category;
    record.event = event;
    record.tick = event.tick;
    record.sim_time = event.sim_time;
    return append(record, message);
}

LogRecordId LoggerService::write_diagnostic(DiagnosticId diagnostic,
                                            LogSeverity severity,
                                            std::string_view message) {
    LogRecord record;
    record.severity = severity;
    record.category = LogCategory::Diagnostics;
    record.diagnostic = diagnostic;
    return append(record, message);
}

std::span<const LogRecord> LoggerService::records() const noexcept {
    return m_record_view;
}

std::string_view LoggerService::message(LogRecordId id) const noexcept {
    const auto found = std::find_if(m_store.begin(), m_store.end(),
        [id](const StoredRecord& stored) {
            return stored.record.id == id;
        });
    if (found == m_store.end()) {
        return {};
    }
    return found->message;
}

std::vector<LogRecord> LoggerService::records_at_or_above(LogSeverity severity) const {
    std::vector<LogRecord> result;
    for (const StoredRecord& stored : m_store) {
        if (severity_at_or_above(stored.record.severity, severity)) {
            result.push_back(stored.record);
        }
    }
    return result;
}

std::vector<LogRecord> LoggerService::records_in_category(LogCategory category) const {
    std::vector<LogRecord> result;
    for (const StoredRecord& stored : m_store) {
        if (stored.record.category == category) {
            result.push_back(stored.record);
        }
    }
    return result;
}

void LoggerService::clear() noexcept {
    m_store.clear();
    m_record_view.clear();
    m_string_bytes = u64(0);
}

void LoggerService::drain_sinks() {
    // File/async sinks are intentionally deferred. The in-memory service is the
    // first reliable contract; sinks can consume records here later.
}

LogRecordId LoggerService::append(LogRecord record, std::string_view message) {
    if (!m_initialised) {
        init();
    }

    record.id = LogRecordId{m_next_id++};
    record.message_size = static_cast<u32>(message.size());

    StoredRecord stored{
        .record = record,
        .message = std::string(message)
    };

    m_string_bytes += static_cast<u64>(stored.message.size());
    m_store.push_back(std::move(stored));
    enforce_capacity();
    rebuild_record_view();
    return record.id;
}

void LoggerService::enforce_capacity() {
    while (m_store.size() > static_cast<std::size_t>(m_config.max_records)) {
        evict_oldest();
    }

    while (m_config.max_string_bytes > u64(0) && m_string_bytes > m_config.max_string_bytes
           && m_store.size() > 1u) {
        evict_oldest();
    }
}

void LoggerService::evict_oldest() noexcept {
    if (m_store.empty()) {
        return;
    }

    const u64 bytes = static_cast<u64>(m_store.front().message.size());
    m_string_bytes = (bytes <= m_string_bytes) ? (m_string_bytes - bytes) : u64(0);
    ++m_dropped_records;
    m_dropped_string_bytes += bytes;
    m_store.erase(m_store.begin());
}

void LoggerService::rebuild_record_view() {
    m_record_view.clear();
    m_record_view.reserve(m_store.size());
    for (const StoredRecord& stored : m_store) {
        m_record_view.push_back(stored.record);
    }
}

bool LoggerService::severity_at_or_above(LogSeverity value, LogSeverity threshold) noexcept {
    return static_cast<u8>(value) >= static_cast<u8>(threshold);
}

} // namespace ndde
