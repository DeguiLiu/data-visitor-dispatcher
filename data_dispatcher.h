// DataDispatcher.h
#ifndef DATA_DISPATCHER_H
#define DATA_DISPATCHER_H

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

#include "data.h"
#include "data_visitor.h"

// 单例的 DataDispatcher
class DataDispatcher {
 public:
  // 获取单例实例
  static DataDispatcher& Instance() {
    static DataDispatcher instance;
    return instance;
  }

  // 禁止拷贝和赋值
  DataDispatcher(const DataDispatcher&) = delete;
  DataDispatcher& operator=(const DataDispatcher&) = delete;

  // 注册一个 DataVisitor
  void RegisterVisitor(const std::shared_ptr<DataVisitor>& visitor) {
    std::lock_guard<std::mutex> lock(mutex_);
    visitors_.emplace_back(visitor);
  }

  // 注销一个 DataVisitor
  void UnregisterVisitor(const std::shared_ptr<DataVisitor>& visitor) {
    std::lock_guard<std::mutex> lock(mutex_);
    visitors_.erase(std::remove(visitors_.begin(), visitors_.end(), visitor), visitors_.end());
  }

  // 分发数据到所有注册的 DataVisitor
  void Dispatch(const std::shared_ptr<Data>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& visitor : visitors_) {
      if (visitor) {
        visitor->Notify(data);
      }
    }
  }

 private:
  // 私有构造函数
  DataDispatcher() = default;
  ~DataDispatcher() = default;

  std::vector<std::shared_ptr<DataVisitor>> visitors_;
  std::mutex mutex_;
};

#endif  // DATA_DISPATCHER_H
