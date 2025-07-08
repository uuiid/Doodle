//
// Created by TD on 25-5-13.
//
#pragma once
#include <doodle_lib/core/http/http_function.h>
namespace doodle::http::local {
// api/doodle/local_setting
DOODLE_HTTP_FUN(local_setting, get, ucom_t{} / "api" / "doodle" / "local_setting", http_function_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(local_setting, post, ucom_t{} / "api" / "doodle" / "local_setting", http_function_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/doodle/task"
DOODLE_HTTP_FUN_CONST(task, get, ucom_t{} / "api" / "doodle" / "task", http_function_template<void>) { init_ctx(); }
void init_ctx();
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(task, post, ucom_t{} / "api" / "doodle" / "task", http_function_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(task, patch, ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "task" / make_cap(g_uuid_regex, &capture_id_t::id_), http_function_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(task, delete_, ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "task" / make_cap(g_uuid_regex, &capture_id_t::id_), http_function_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/doodle/task/{id}"
DOODLE_HTTP_FUN(task_instance, get, ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "task" / make_cap(g_uuid_regex, &capture_id_t::id_), http_function_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
//"api/doodle/task/{id}/restart"
DOODLE_HTTP_FUN(
    task_instance_restart, post,
    ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "task" / make_cap(g_uuid_regex, &capture_id_t::id_) / "restart",
    http_function_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/doodle/task/{id}/log"
DOODLE_HTTP_FUN(
    task_instance_log, get,
    ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "task" / make_cap(g_uuid_regex, &capture_id_t::id_) / "log",
    http_function_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/doodle/video/thumbnail"
DOODLE_HTTP_FUN(video_thumbnail, post, ucom_t{} / "api" / "doodle" / "video" / "thumbnail", http_function_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN_CONST(video_thumbnail, get, ucom_t{} / "api" / "doodle" / "video" / "thumbnail", http_function_template<void>) {
  init_ctx();
}
void init_ctx();
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http::local
