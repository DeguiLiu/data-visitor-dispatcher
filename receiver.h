// Receiver.h
#ifndef RECEIVER_H
#define RECEIVER_H

#include <functional>
#include <memory>
#include <string>

#include "data.h"
#include "data_dispatcher.h"

// Receiver，模拟消息接收器
class Receiver {
 public:
  using Callback = std::function<void(std::shared_ptr<Data>)>;

  // 构造函数，绑定回调函数
  explicit Receiver(Callback callback) : callback_(callback) {
  }

  // 模拟接收消息
  void ReceiveMessage(int id, const std::string& content) {
    auto data = std::make_shared<Data>();
    data->id = id;
    data->content = content;

    // 触发回调
    if (callback_) {
      callback_(data);
    }
  }

 private:
  Callback callback_;
};

#endif  // RECEIVER_H
