#pragma once

#include "doodle_core/metadata/user.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
boost::asio::awaitable<void> recomputing_time(const uuid& in_person_id, const chrono::year_month& in_year_month);
struct computing_time_args {
  uuid user_id_{};
  chrono::year_month year_month_{};
};

// "/api/doodle/computing_time/{user_id}/{year_month}"
DOODLE_HTTP_FUN(computing_time, post, ucom_t{} / "api" / "doodle" / "computing_time" / make_cap(g_uuid_regex, &computing_time_args::user_id_) / make_cap(g_year_month_regex, &computing_time_args::year_month_), http_jwt_fun_template<computing_time_args>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<computing_time_args> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{user_id}/{year_month}/add"
DOODLE_HTTP_FUN(computing_time_add, post, ucom_t{} / "api" / "doodle" / "computing_time" / make_cap(g_uuid_regex, &computing_time_args::user_id_) / make_cap(g_year_month_regex, &computing_time_args::year_month_) / "add", http_jwt_fun_template<computing_time_args>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<computing_time_args> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{user_id}/{year_month}/custom"
DOODLE_HTTP_FUN(
    computing_time_custom, post,
    ucom_t{} / "api" / "doodle" / "computing_time" /
        make_cap(g_uuid_regex, &computing_time_args::user_id_) /
        make_cap(g_year_month_regex, &computing_time_args::year_month_) / "custom",
    http_jwt_fun_template<computing_time_args>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<computing_time_args> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{user_id}/{year_month}/sort"
DOODLE_HTTP_FUN(
    computing_time_sort, post,
    ucom_t{} / "api" / "doodle" / "computing_time" /
        make_cap(g_uuid_regex, &computing_time_args::user_id_) /
        make_cap(g_year_month_regex, &computing_time_args::year_month_) / "sort",
    http_jwt_fun_template<computing_time_args>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<computing_time_args> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{user_id}/{year_month}/average"
DOODLE_HTTP_FUN(
    computing_time_average, post,
    ucom_t{} / "api" / "doodle" / "computing_time" /
        make_cap(g_uuid_regex, &computing_time_args::user_id_) /
        make_cap(g_year_month_regex, &computing_time_args::year_month_) / "average",
    http_jwt_fun_template<computing_time_args>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<computing_time_args> in_arg
) override;
DOODLE_HTTP_FUN_END()
//"/api/doodle/computing_time/{user_id}/{year_month}"
DOODLE_HTTP_FUN(computing_time, get, ucom_t{} / "api" / "doodle" / "computing_time" / make_cap(g_uuid_regex, &computing_time_args::user_id_) / make_cap(g_year_month_regex, &computing_time_args::year_month_), http_jwt_fun_template<computing_time_args>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<computing_time_args> in_arg
) override;
DOODLE_HTTP_FUN_END()
struct computing_time_args2 {
  uuid user_id_{};
  chrono::year_month year_month_{};
  uuid task_id_{};
};
// "/api/doodle/computing_time/{user_id}/{year_month}/{task_id}"
DOODLE_HTTP_FUN(computing_time, patch, ucom_t{} / "api" / "doodle" / "computing_time" / make_cap(g_uuid_regex, &computing_time_args2::user_id_) / make_cap(g_year_month_regex, &computing_time_args2::year_month_) / make_cap(g_uuid_regex, &computing_time_args2::task_id_), http_jwt_fun_template<computing_time_args2>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<computing_time_args2> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{computing_time_id}"
DOODLE_HTTP_FUN(computing_time, delete_, ucom_t{} / "api" / "doodle" / "computing_time" / make_cap(g_uuid_regex, &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http