// DataVisitor.h
#ifndef DATA_VISITOR_H
#define DATA_VISITOR_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "data.h"

class DataVisitor : public std::enable_shared_from_this<DataVisitor> {
 public:
  using Callback = std::function<void(std::shared_ptr<Data>)>;

  // 构造函数，绑定回调函数
  explicit DataVisitor(Callback callback) : callback_(callback), stop_flag_(false) {
    worker_thread_ = std::thread(&DataVisitor::ProcessData, this);
  }

  // 析构函数，确保线程安全停止
  ~DataVisitor() {
    Stop();
    if (worker_thread_.joinable()) {
      worker_thread_.join();
    }
  }

  // 用于 DataDispatcher 发布数据时调用
  void Notify(std::shared_ptr<Data> data) {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      data_queue_.push(data);
    }
    cv_.notify_one();
  }

 private:
  // 数据处理线程函数
  void ProcessData() {
    while (!stop_flag_) {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      cv_.wait(lock, [this]() { return stop_flag_ || !data_queue_.empty(); });

      while (!data_queue_.empty()) {
        auto data = data_queue_.front();
        data_queue_.pop();
        lock.unlock();

        // 执行绑定的回调函数
        if (callback_) {
          try {
            callback_(data);
          } catch (const std::exception& e) {
            // 处理回调中的异常，防止线程终止
            // 可以记录日志或采取其他措施
            // 这里简单输出错误信息
            std::cerr << "Exception in callback: " << e.what() << std::endl;
          }
        }

        lock.lock();
      }
    }
  }

  // 停止线程
  void Stop() {
    stop_flag_ = true;
    cv_.notify_all();
  }

  Callback callback_;
  std::thread worker_thread_;
  std::mutex queue_mutex_;
  std::condition_variable cv_;
  std::queue<std::shared_ptr<Data>> data_queue_;
  std::atomic<bool> stop_flag_;
};

#endif  // DATA_VISITOR_H
