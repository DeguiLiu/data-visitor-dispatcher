// ProcessingVisitor.h
#ifndef PROCESSING_VISITOR_H
#define PROCESSING_VISITOR_H

#include <iostream>
#include <memory>
#include "data_visitor.h"

// ProcessingVisitor，负责处理数据
class ProcessingVisitor {
 public:
  // 创建一个 ProcessingVisitor 的 DataVisitor 实例
  static std::shared_ptr<DataVisitor> Create() {
    return std::make_shared<DataVisitor>([](std::shared_ptr<Data> data) {
      // 简单示例：打印数据长度
      std::cout << "[ProcessingVisitor] Processed data ID=" << data->id << ", Length=" << data->content.length()
                << std::endl;
    });
  }
};

#endif  // PROCESSING_VISITOR_H
