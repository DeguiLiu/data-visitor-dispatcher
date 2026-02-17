## CyberRT DataVisitor/DataDispatcher: 原始实现 vs MCCC 重写

CyberRT 风格的 DataVisitor/DataDispatcher 数据分发模式，演示如何用 [mccc-bus](https://gitee.com/liudegui/mccc-bus) 无锁消息总线替代传统的 mutex + thread 方案。

### 文件说明

| 文件 | 说明 |
|------|------|
| `main.cpp` | 原始实现 (mutex + per-visitor thread + std::function) |
| `data.h` / `data_visitor.h` / `data_dispatcher.h` / `receiver.h` | 原始实现的头文件 |
| `logging_visitor.h` / `processing_visitor.h` | 原始实现的具体 visitor |
| `mccc_demo.cpp` | **MCCC 重写: Component 动态订阅版** |
| `mccc_static_demo.cpp` | **MCCC 重写: StaticComponent 零开销版** |
| `CMakeLists.txt` | CMake 构建配置 |

### 问题: 原始实现的架构缺陷

原始实现用 ~200 行代码模拟 CyberRT 的 DataVisitor/DataDispatcher 模式，但存在以下工程问题:

| 问题 | 严重度 | 说明 |
|------|--------|------|
| 全局 mutex 瓶颈 | 性能 | DataDispatcher 每次 Dispatch 都锁 mutex，N 个生产者串行化 |
| 每 visitor 一个线程 | 资源浪费 | 10 个 visitor = 10 个线程，CyberRT 原版用协程池 |
| `std::function` 堆分配 | 性能 | 每个回调都可能触发 heap allocation |
| `shared_ptr<Data>` 满天飞 | 性能 | 每条消息一次 `make_shared`，引用计数原子操作 |
| `std::queue` 无界 | 安全隐患 | 生产者快于消费者时无限增长，无背压机制 |
| 无 topic 路由 | 功能缺失 | 所有 visitor 收到所有消息，无法按类型订阅 |
| 无生命周期管理 | 正确性 | visitor 析构顺序不当可能导致线程 join 死锁 |

### 解决方案: MCCC 重写

MCCC (Message-driven Component Communication for C++) 是一个 C++17 header-only 无锁消息总线:

- lock-free MPSC Ring Buffer，零 mutex
- `FixedFunction` SBO 回调，零堆分配
- 定长环形缓冲，内置背压
- 编译期类型安全的 `std::variant` 消息路由
- Component 生命周期自动管理

提供两种重写版本:

#### 1. Component 版 (`mccc_demo.cpp`) -- 动态订阅

- 支持运行时动态注册/注销 visitor
- `shared_ptr` release 自动取消订阅
- 完整复现原始 demo 的"移除 LoggingVisitor"行为

#### 2. StaticComponent 版 (`mccc_static_demo.cpp`) -- 零开销

- CRTP 编译期分发，无间接调用
- 栈分配 visitor，零 shared_ptr
- Handler 可被编译器内联

### 技术对比

| 维度 | Original | MCCC Component | MCCC StaticComponent |
|------|----------|----------------|----------------------|
| 并发同步 | `std::mutex` | lock-free CAS | lock-free CAS |
| 线程模型 | 每 visitor 独立线程 | 单 worker 线程 | 单 worker 线程 |
| 回调存储 | `std::function` (堆分配) | `FixedFunction` SBO (零堆) | 编译期内联 |
| 消息队列 | `std::queue` (无界, OOM) | Ring Buffer (定长, 背压) | Ring Buffer (定长, 背压) |
| 消息传递 | `shared_ptr<Data>` (堆分配) | Ring Buffer 嵌入 (零拷贝) | Ring Buffer 嵌入 (零拷贝) |
| 字符串类型 | `std::string` (堆分配) | `FixedString<N>` (栈内联) | `FixedString<N>` (栈内联) |
| 生命周期 | 手动 register/unregister | `weak_ptr` 自动管理 | 栈 RAII |
| 动态订阅 | 支持 | 支持 | 不支持 (编译期固定) |
| 统计信息 | 无 | 内置 (发布/处理/丢弃) | 内置 |
| 代码量 | ~200 行 / 7 文件 | ~110 行 / 1 文件 | ~95 行 / 1 文件 |

### 构建与运行

```bash
# 依赖: mccc-bus 源码
git clone git@gitee.com:liudegui/mccc-bus.git /tmp/mccc-bus

# 构建
mkdir build && cd build
cmake .. -DMCCC_DIR=/tmp/mccc-bus
make -j$(nproc)

# 运行
./mccc_demo            # Component 版 (动态订阅)
./mccc_static_demo     # StaticComponent 版 (零开销)
```

### 相关项目

- [mccc-bus](https://gitee.com/liudegui/mccc-bus): C++17 header-only 无锁消息总线
- [newosp](https://github.com/DeguiLiu/newosp): 工业级嵌入式 C++17 基础设施库 (基于 mccc-bus)
- 技术文章: [嵌入式 C++ 技术专栏](https://deguiliu.github.io/embedded-cpp-articles/)
