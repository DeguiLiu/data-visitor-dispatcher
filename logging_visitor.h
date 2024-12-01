// LoggingVisitor.h
#ifndef LOGGING_VISITOR_H
#define LOGGING_VISITOR_H

#include <iostream>
#include <memory>
#include "data_visitor.h"

// LoggingVisitor，负责记录数据
class LoggingVisitor {
 public:
  // 创建一个 LoggingVisitor 的 DataVisitor 实例
  static std::shared_ptr<DataVisitor> Create() {
    return std::make_shared<DataVisitor>([](std::shared_ptr<Data> data) {
      std::cout << "[LoggingVisitor] Received data: ID=" << data->id << ", Content=\"" << data->content << "\""
                << std::endl;
    });
  }
};

#endif  // LOGGING_VISITOR_H
