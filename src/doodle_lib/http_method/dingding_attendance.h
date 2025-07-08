#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {

struct dingding_attendance_args {
  uuid user_id_;
  chrono::year_month year_month_{};
};
// "/api/doodle/attendance/{user_id}/{date}"
DOODLE_HTTP_FUN(dingding_attendance, get, ucom_t{}.ro<dingding_attendance_args>() / "api" / "doodle" / "attendance" / make_cap(g_uuid_regex, &dingding_attendance_args::user_id_) / make_cap(g_year_month_regex, &dingding_attendance_args::year_month_), http_jwt_fun_template<dingding_attendance_args>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<dingding_attendance_args>& in_arg
) override;
DOODLE_HTTP_FUN_END()
//"/api/doodle/attendance/{user_id}"
DOODLE_HTTP_FUN(dingding_attendance, post, ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "attendance" / make_cap(g_uuid_regex, &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/attendance/{user_id}/custom"
DOODLE_HTTP_FUN(
    dingding_attendance_custom, post,
    ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "attendance" / make_cap(g_uuid_regex, &capture_id_t::id_) /
        "custom",
    http_jwt_fun_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/attendance/custom/{id}"
DOODLE_HTTP_FUN(
    dingding_attendance_custom, put,
    ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "attendance" / "custom" /
        make_cap(g_uuid_regex, &capture_id_t::id_),
    http_jwt_fun_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/attendance/custom/{id}"
DOODLE_HTTP_FUN(
    dingding_attendance_custom, delete_,
    ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "attendance" / "custom" /
        make_cap(g_uuid_regex, &capture_id_t::id_),
    http_jwt_fun_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http