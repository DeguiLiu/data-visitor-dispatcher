/**
 * @file static_component_demo.cpp
 * @brief Zero-overhead CRTP compile-time dispatch demo.
 *
 * Uses StaticComponent for compile-time handler binding:
 * - No virtual destructor, no shared_ptr / weak_ptr
 * - No FixedFunction / callback table lookup
 * - Handler calls are inlineable by the compiler
 *
 * Trade-off: handlers are fixed at compile time (no dynamic subscribe/unsubscribe).
 */

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif
#include "log_macro.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <variant>

#include <mccc/static_component.hpp>

// ---------------------------------------------------------------------------
// Message types
// ---------------------------------------------------------------------------

struct SensorData {
  int32_t id;
  mccc::FixedString<64> content;

  SensorData() noexcept : id(0) {}
  SensorData(int32_t id_, const char* msg) noexcept
      : id(id_), content(mccc::TruncateToCapacity, msg) {}
};

// ---------------------------------------------------------------------------
// Bus type aliases
// ---------------------------------------------------------------------------

using DemoPayload = std::variant<SensorData>;
using DemoBus = mccc::AsyncBus<DemoPayload>;

// ---------------------------------------------------------------------------
// LoggingVisitor (CRTP: Handle method detected at compile time)
// ---------------------------------------------------------------------------

class LoggingVisitor
    : public mccc::StaticComponent<LoggingVisitor, DemoPayload> {
 public:
  void Handle(const SensorData& data) noexcept {
    LOG_INFO("[LoggingVisitor] id=%d content=\"%s\"",
             data.id, data.content.c_str());
  }
};

// ---------------------------------------------------------------------------
// ProcessingVisitor (CRTP: Handle method detected at compile time)
// ---------------------------------------------------------------------------

class ProcessingVisitor
    : public mccc::StaticComponent<ProcessingVisitor, DemoPayload> {
 public:
  void Handle(const SensorData& data) noexcept {
    LOG_INFO("[ProcessingVisitor] id=%d length=%u",
             data.id, data.content.size());
  }
};

// ---------------------------------------------------------------------------
// CombinedVisitor: dispatch to multiple handlers in one pass
// ---------------------------------------------------------------------------

template <typename... Visitors>
class CombinedVisitor {
 public:
  explicit CombinedVisitor(Visitors&... visitors) noexcept
      : visitors_(visitors...) {}

  template <typename T>
  void operator()(const T& data) noexcept {
    DispatchAll<T>(data, std::index_sequence_for<Visitors...>{});
  }

 private:
  template <typename T, size_t... Is>
  void DispatchAll(const T& data, std::index_sequence<Is...>) noexcept {
    (std::get<Is>(visitors_).get().Handle(data), ...);
  }

  std::tuple<std::reference_wrapper<Visitors>...> visitors_;
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
  LOG_INFO("========================================");
  LOG_INFO("   StaticComponent Demo");
  LOG_INFO("   (Zero-overhead compile-time dispatch)");
  LOG_INFO("========================================");

  // Stack-allocated visitors (no shared_ptr, no heap)
  LoggingVisitor logger;
  ProcessingVisitor processor;

  // Combined visitor: processes both in a single Ring Buffer pass
  CombinedVisitor combined(logger, processor);

  std::atomic<bool> stop_worker{false};

  // Single consumer thread with compile-time dispatch
  std::thread worker([&stop_worker, &combined]() noexcept {
    while (!stop_worker.load(std::memory_order_acquire)) {
      uint32_t processed = DemoBus::Instance().ProcessBatchWith(combined);
      if (processed == 0U) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    }
    while (DemoBus::Instance().ProcessBatchWith(combined) > 0U) {}
  });

  // Publish messages
  LOG_INFO("");
  LOG_INFO("=== Publishing messages ===");

  DemoBus::Instance().Publish(SensorData{1, "Hello, World!"}, /*sender_id=*/1U);
  DemoBus::Instance().Publish(SensorData{2, "Another data packet."}, /*sender_id=*/1U);
  DemoBus::Instance().Publish(SensorData{3, "Zero-overhead dispatch."}, /*sender_id=*/1U);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Statistics
  auto stats = DemoBus::Instance().GetStatistics();
  LOG_INFO("");
  LOG_INFO("Statistics:");
  LOG_INFO("  Published: %lu", stats.messages_published);
  LOG_INFO("  Processed: %lu", stats.messages_processed);
  LOG_INFO("  Dropped:   %lu", stats.messages_dropped);

  LOG_INFO("");
  LOG_INFO("=== Demo completed ===");
  stop_worker.store(true, std::memory_order_release);
  worker.join();

  return 0;
}
