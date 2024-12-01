// main.cpp
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "data_dispatcher.h"
#include "logging_visitor.h"
#include "processing_visitor.h"
#include "receiver.h"

int main() {
  // 创建并注册 DataVisitor
  auto logger = LoggingVisitor::Create();
  auto processor = ProcessingVisitor::Create();
  DataDispatcher::Instance().RegisterVisitor(logger);
  DataDispatcher::Instance().RegisterVisitor(processor);

  // 创建 Receiver，并绑定 DataDispatcher 的 Dispatch 方法
  Receiver receiver([](std::shared_ptr<Data> data) { DataDispatcher::Instance().Dispatch(data); });

  // 模拟接收消息
  std::cout << "=== 接收第1条消息 ===" << std::endl;
  receiver.ReceiveMessage(1, "Hello, CyberRT!");
  std::cout << "=== 接收第2条消息 ===" << std::endl;
  receiver.ReceiveMessage(2, "Another data packet.");

  // 等待一段时间以确保所有消息被处理
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // 注销一个 DataVisitor
  std::cout << "\n=== 移除 LoggingVisitor ===" << std::endl;
  DataDispatcher::Instance().UnregisterVisitor(logger);

  // 再次接收消息
  std::cout << "=== 接收第3条消息 ===" << std::endl;
  receiver.ReceiveMessage(3, "Data after removing logger.");

  // 等待一段时间以确保消息被处理
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::cout << "\n=== 程序结束 ===" << std::endl;

  return 0;
}
