## Data Visitor Dispatcher

基于无锁 MPSC 消息总线的观察者模式示例，演示嵌入式场景下的数据分发架构。

### 特性

- **无锁并发**: lock-free CAS (MPSC) 多生产者无锁并发发布
- **零堆分配**: Ring Buffer 嵌入消息存储，`FixedString<N>` 栈缓冲替代 `std::string`
- **单 worker 线程**: `ProcessBatch()` 一次遍历处理所有消息，线程数 O(1)
- **编译期类型路由**: `std::variant` + `Subscribe<T>` 类型安全订阅
- **内置背压**: 定长环形缓冲，生产者快于消费者时自动丢弃最旧消息

### 两种实现

| 版本 | 文件 | 订阅方式 | 分发机制 | 适用场景 |
|------|------|----------|----------|----------|
| Component 版 | `examples/component_demo.cpp` | 运行时动态 | `FixedFunction` SBO 回调 | 需要动态增删订阅者 |
| StaticComponent 版 | `examples/static_component_demo.cpp` | 编译期固定 | CRTP `Handle()` 内联 | 订阅者集合固定，追求零开销 |

### 技术对比

| 维度 | Component 版 | StaticComponent 版 |
|------|-------------|-------------------|
| 代码量 | ~110 行 / 1 文件 | ~95 行 / 1 文件 |
| 堆分配 (每条消息) | 0 次 | 0 次 |
| 线程数 | 2 (worker + main) | 2 (worker + main) |
| 动态增删订阅者 | 支持 | 不支持 |
| 间接调用 | `FixedFunction` (SBO，非堆) | 无 (可内联) |
| 订阅者存储 | `shared_ptr` 堆分配 | 栈分配 |

### 构建与运行

```bash
# 依赖: mccc-bus 源码
git clone git@gitee.com:liudegui/mccc-bus.git /tmp/mccc-bus

# 构建
mkdir build && cd build
cmake .. -DMCCC_DIR=/tmp/mccc-bus
make -j$(nproc)

# 运行
./examples/component_demo          # Component 版 (动态订阅)
./examples/static_component_demo   # StaticComponent 版 (零开销)
```

### 目录结构

```
data-visitor-dispatcher/
├── CMakeLists.txt
├── LICENSE
├── README.md
└── examples/
    ├── CMakeLists.txt
    ├── component_demo.cpp
    └── static_component_demo.cpp
```

### 相关项目

- [mccc-bus](https://gitee.com/liudegui/mccc-bus): C++17 header-only 无锁消息总线
- [newosp](https://github.com/DeguiLiu/newosp): 工业级嵌入式 C++17 基础设施库 (基于 mccc-bus)
- 技术文章: [嵌入式 C++ 技术专栏](https://deguiliu.github.io/embedded-cpp-articles/)

### License

MIT
