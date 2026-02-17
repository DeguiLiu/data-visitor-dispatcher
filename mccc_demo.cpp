/**
 * @file mccc_demo.cpp
 * @brief CyberRT DataVisitor/DataDispatcher rewrite using MCCC.
 *
 * Original: mutex + vector + per-visitor thread + std::function + shared_ptr
 * MCCC:     lock-free MPSC bus + Component lifecycle + FixedFunction SBO
 *
 * Demonstrates:
 * - Zero heap allocation message passing (Ring Buffer embedded)
 * - Component-based subscribe with automatic lifecycle management
 * - Dynamic register/unregister of visitors at runtime
 * - Single consumer thread (ProcessBatch) replaces per-visitor threads
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

#include <mccc/component.hpp>

// ---------------------------------------------------------------------------
// Message types (replaces Data struct)
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
using DemoComponent = mccc::Component<DemoPayload>;
using DemoEnvelope = mccc::MessageEnvelope<DemoPayload>;

// ---------------------------------------------------------------------------
// LoggingVisitor (replaces original LoggingVisitor + DataVisitor thread)
// ---------------------------------------------------------------------------

class LoggingVisitor : public DemoComponent {
 public:
  static std::shared_ptr<LoggingVisitor> Create() noexcept {
    std::shared_ptr<LoggingVisitor> ptr(new LoggingVisitor());
    ptr->Init();
    return ptr;
  }

 private:
  LoggingVisitor() = default;

  void Init() noexcept {
    InitializeComponent();
    SubscribeSimple<SensorData>(
        [](const SensorData& data, const mccc::MessageHeader& hdr) noexcept {
          LOG_INFO("[LoggingVisitor] msg_id=%lu id=%d content=\"%s\"",
                   hdr.msg_id, data.id, data.content.c_str());
        });
  }
};

// ---------------------------------------------------------------------------
// ProcessingVisitor (replaces original ProcessingVisitor + DataVisitor thread)
// ---------------------------------------------------------------------------

class ProcessingVisitor : public DemoComponent {
 public:
  static std::shared_ptr<ProcessingVisitor> Create() noexcept {
    std::shared_ptr<ProcessingVisitor> ptr(new ProcessingVisitor());
    ptr->Init();
    return ptr;
  }

 private:
  ProcessingVisitor() = default;

  void Init() noexcept {
    InitializeComponent();
    SubscribeSimple<SensorData>(
        [](const SensorData& data, const mccc::MessageHeader& hdr) noexcept {
          LOG_INFO("[ProcessingVisitor] msg_id=%lu id=%d length=%u",
                   hdr.msg_id, data.id, data.content.size());
        });
  }
};

// ---------------------------------------------------------------------------
// Receiver (message source, publishes to the bus)
// ---------------------------------------------------------------------------

class Receiver {
 public:
  explicit Receiver(uint32_t sender_id) noexcept : sender_id_(sender_id) {}

  void ReceiveMessage(int32_t id, const char* content) noexcept {
    SensorData data(id, content);
    DemoBus::Instance().Publish(std::move(data), sender_id_);
  }

 private:
  uint32_t sender_id_;
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
  LOG_INFO("========================================");
  LOG_INFO("   MCCC DataVisitor/Dispatcher Demo");
  LOG_INFO("========================================");

  std::atomic<bool> stop_worker{false};

  // Single consumer thread (replaces N per-visitor threads)
  std::thread worker([&stop_worker]() noexcept {
    while (!stop_worker.load(std::memory_order_acquire)) {
      uint32_t processed = DemoBus::Instance().ProcessBatch();
      if (processed == 0U) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    }
    while (DemoBus::Instance().ProcessBatch() > 0U) {}
  });

  // Create and register visitors
  auto logger = LoggingVisitor::Create();
  auto processor = ProcessingVisitor::Create();

  // Create receiver
  Receiver receiver(/*sender_id=*/1U);

  // Simulate receiving messages
  LOG_INFO("");
  LOG_INFO("=== Receiving message #1 ===");
  receiver.ReceiveMessage(1, "Hello, CyberRT!");
  LOG_INFO("=== Receiving message #2 ===");
  receiver.ReceiveMessage(2, "Another data packet.");

  // Wait for processing
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Unregister LoggingVisitor (shared_ptr release triggers Unsubscribe)
  LOG_INFO("");
  LOG_INFO("=== Removing LoggingVisitor ===");
  logger.reset();

  // Send another message (only ProcessingVisitor receives it)
  LOG_INFO("=== Receiving message #3 ===");
  receiver.ReceiveMessage(3, "Data after removing logger.");

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Show statistics
  auto stats = DemoBus::Instance().GetStatistics();
  LOG_INFO("");
  LOG_INFO("Statistics:");
  LOG_INFO("  Published: %lu", stats.messages_published);
  LOG_INFO("  Processed: %lu", stats.messages_processed);
  LOG_INFO("  Dropped:   %lu", stats.messages_dropped);

  // Cleanup
  LOG_INFO("");
  LOG_INFO("=== Demo completed ===");
  stop_worker.store(true, std::memory_order_release);
  worker.join();

  return 0;
}
