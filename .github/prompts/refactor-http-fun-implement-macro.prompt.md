---
description: "将 HTTP 处理器实现函数签名 boost::asio::awaitable<boost::beast::http::message_generator> ClassName::method(session_data_ptr in_handle) 替换为宏 DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(ClassName, method)，重构范围为输入的文件"
name: refactor-http-fun-implement-macro
argument-hint: "选择要替换函数声明的 http_method 文件"
agent: "agent"
---

## 原模式
```cpp
boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_create_post::post(
    session_data_ptr in_handle
) {
```

## 目标模式
```cpp
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(dingding_attendance_create_post, post) {
```

## 规则
- 仅替换类成员实现函数的声明部分，函数体保持不变
- 方法名覆盖 `http_function` 的虚方法：get / put / post / options / head / patch / delete_
- 原签名可能跨行（返回类型、类名::方法名、参数列表可拆多行），需整体匹配后替换为目标宏
- 宏由 `src/doodle_lib/core/http/http_function.h` 提供，无需额外包含头文件
- 仅修改输入的文件，不触碰其它方法或文件
