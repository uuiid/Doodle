---
description: "Use when writing or refactoring HTTP handler 实现/声明 (http_method, DOODLE_HTTP_FUN, DOODLE_HTTP_FUN_C, handler 实现签名). 统一规范：类声明用 DOODLE_HTTP_FUN / DOODLE_HTTP_JWT_FUN + DOODLE_HTTP_FUN_OVERRIDE，cpp 实现一律用 DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(类名, 方法名)，禁止手写 boost::asio::awaitable<...> ...::method(session_data_ptr) 签名。"
applyTo: "src/doodle_lib/http_method/**"
---
# HTTP 处理器声明与实现宏规范

`src/doodle_lib/http_method/**` 下的 HTTP 处理器，声明与实现签名必须使用
`core/http/http_function.h`（及 `http_method/http_jwt_fun.h`）提供的宏，禁止手写完整签名。

## 声明（.h）：类 + 虚方法 override

每个处理器类用 `DOODLE_HTTP_FUN`（无鉴权）或 `DOODLE_HTTP_JWT_FUN`（JWT 鉴权）
声明，内部按需用 `DOODLE_HTTP_FUN_OVERRIDE(method)` 逐个声明要覆盖的方法，
成员变量写在中间，末尾用 `DOODLE_HTTP_FUN_END()` 收尾。

```cpp
// "/api/doodle/attendance/{user_id}"
DOODLE_HTTP_JWT_FUN(dingding_attendance_create_post)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};                                  // 成员变量
DOODLE_HTTP_FUN_END()

// 无鉴权处理器
DOODLE_HTTP_FUN(auth_login)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
```

## 实现（.cpp）：一律用 DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT

```cpp
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(dingding_attendance_create_post, post) {
  auto l_json = in_handle->get_json();
  // ...函数体，开放成员与方法签名由宏展开提供
}
```

## 禁止

```cpp
// 禁止手写完整签名（含跨行拆写）
boost::asio::awaitable<boost::beast::http::message_generator> dingding_attendance_create_post::post(
    session_data_ptr in_handle
) {
```

## 宏关系

```cpp
// http_function.h
#define DOODLE_HTTP_FUN_C(fun_name, base_fun)   // 声明类 + clone，展开 public:
#define DOODLE_HTTP_FUN(fun_name)  DOODLE_HTTP_FUN_C(fun_name, ::doodle::http::http_function)
#define DOODLE_HTTP_FUN_OVERRIDE(method)          // 声明虚方法 override
#define DOODLE_HTTP_FUN_END()                     // }; 收尾
#define DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(clazz, method) \
  boost::asio::awaitable<boost::beast::http::message_generator> clazz::method(session_data_ptr in_handle)

// http_jwt_fun.h
#define DOODLE_HTTP_JWT_FUN(fun_name) DOODLE_HTTP_FUN_C(fun_name, ::doodle::http::http_jwt_fun)
```

## 规则

- 方法名只覆盖返回 `message_generator` 的虚方法：`get / put / post / options / head / patch / delete_`
  （`delete_` 带下划线，声明与实现两处必须一致）
- 新增自定义基类时，仿照 `DOODLE_HTTP_JWT_FUN` 用 `DOODLE_HTTP_FUN_C(fun_name, 基类)` 派生，
  不要改动 `DOODLE_HTTP_FUN_C` 本身
- `websocket_init` / `websocket_callback` 签名不同，不适用本宏，保持手写 override
- 头文件需包含 `<doodle_lib/core/http/http_function.h>`（使用 JWT 基类时含
  `<doodle_lib/http_method/http_jwt_fun.h>`），宏展开所需的 `session_data_ptr` 类型由同一路径提供
- 替换时整体匹配可能跨行的手写签名，函数体不动
