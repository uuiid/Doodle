---
description: "审查Git提交/差异中的C++代码，检查逻辑错误、协程、空指针、线程安全等问题，输出结构化检查清单"
agent: C++ Expert
tools: [vscode, execute, read, agent, ms-azuretools.vscode-containers, ms-vscode.cpp-devtools, ms-vscode.cpptools, edit, search, web, browser, todo]
---

# git审查

审查提供的 Git diff / 代码变更，按以下结构化清单逐项检查。**忽略接口版本兼容性问题。**

## 检查清单

### 1. 逻辑正确性
- [ ] 条件分支逻辑是否正确（边界条件、短路求值）
- [ ] 循环是否正确（边界、迭代器失效、无限循环）
- [ ] 类型转换是否安全（隐式转换、`static_cast`/`reinterpret_cast` 合理性）
- [ ] 算术运算是否可能溢出或被零除
- [ ] 枚举/switch 是否覆盖所有分支（缺省处理）
- [ ] 比较操作语义是否正确（浮点数比较、无符号数减法）

### 2. C++20 协程 (boost::asio::awaitable)
- [ ] `co_await` 是否在正确的作用域内（不在非协程函数中调用）
- [ ] 协程生命周期管理：`co_await` 期间对象是否仍然存活
- [ ] 协程是否通过 `boost::asio::use_awaitable` 或 `boost::asio::awaitable&lt;T&gt;` 正确连接
- [ ] 协程取消/超时处理是否完备
- [ ] 协程中捕获的引用/指针在挂起点后是否仍有效
- [ ] `co_return` 是否正确返回预期值
- [ ] 协程异常传播路径是否清晰（try-catch 包裹 `co_await` 表达式）
- [ ] `DOODLE_TO_EXECUTOR`/`DOODLE_TO_MAIN_THREAD`/`DOODLE_TO_SELF` 宏使用是否正确
- [ ] 是否存在在错误 executor 上恢复执行的风险

### 3. 空指针与智能指针
- [ ] `std::shared_ptr`/`std::unique_ptr` 的 `.get()` 是否被裸持并超过智能指针生命周期
- [ ] 解引用前是否检查指针非空
- [ ] `std::weak_ptr::lock()` 调用后是否检查结果
- [ ] 智能指针循环引用风险（是否该用 `weak_ptr` 打破环）
- [ ] `std::unique_ptr` 是否被意外拷贝（通过引用或 `std::move` 语义正确性）
- [ ] 原始指针参数是否标注 `nullptr` 为合法值（文档或断言）
- [ ] `std::make_shared`/`std::make_unique` 优先于 `new` 的直接使用

### 4. 线程安全与并发
- [ ] 共享数据是否通过互斥体、`strand` 或原子操作保护
- [ ] `boost::asio::strand` 是否一致地用于序列化协程访问
- [ ] 是否存在跨 `strand` 的不安全数据共享
- [ ] `std::mutex` 加锁范围是否足够小（避免持锁调用协程）
- [ ] 是否在协程挂起期间持有锁（禁止：持有 mutex 时 `co_await`）
- [ ] 原子变量使用 `std::memory_order` 是否正确（默认 `seq_cst` 是否必要或有性能问题）
- [ ] 静态/全局变量的并发初始化是否有锁保护或为 `constinit`/`constexpr`

### 5. 竞态条件
- [ ] 读写分离是否存在 TOCTOU 问题
- [ ] 延迟初始化是否存在双重检查锁定（是否使用 `std::call_once` 或 `static local`）
- [ ] 事件/回调注册与注销的顺序是否可能导致野回调
- [ ] 异步操作中是否存在"先释放后使用"（对象在回调执行前被销毁）
- [ ] 协程的取消信号 (`cancellation_signal`) 是否与操作生命周期匹配

### 6. 死锁
- [ ] 多个锁的加锁顺序是否一致（是否可能死锁）
- [ ] 是否在持有锁的代码路径中间接请求同一把锁（重入）
- [ ] `boost::asio::strand` 的嵌套分发是否导致上下文死等
- [ ] 协程 A 等待协程 B，协程 B 又等待协程 A（协程死锁）
- [ ] 同步原语（`std::promise`/`std::future`）与协程混合使用时是否阻塞了 io_context 线程

### 7. 资源管理
- [ ] RAII 对象生命周期是否与作用域匹配
- [ ] 异常路径上资源是否泄漏
- [ ] 文件 handle / socket / 数据库连接等是否一定被关闭
- [ ] 移动语义后对象状态是否有效（移后源不被使用）

### 8. 性能隐患（仅明显路径）
- [ ] 容器是否按值传递大对象（应传 const 引用或智能指针）
- [ ] 不必要的拷贝（如循环中 `auto` 而非 `const auto&`）
- [ ] 锁竞争激烈度是否可能导致性能退化

## 输出格式

按上述清单逐项报告，每项检出的问题格式如下：

```
- ❌ [类别-子项] 描述问题 | 文件:行号 | 建议修复
```

对于未发现问题的项目，合并报告为一行：
```
- ✅ [类别] 未发现问题
```

最终给出总体评价：**通过(Pass)** / **需修改(Needs Fix)** / **不通过(Fail)**
