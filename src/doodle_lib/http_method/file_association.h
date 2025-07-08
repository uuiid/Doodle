#pragma once

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {

// /api/doodle/file_association/{uuid}
DOODLE_HTTP_FUN(doodle_file_association, get, ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "file_association" / make_cap(g_uuid_regex, &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/doodle/file"
DOODLE_HTTP_FUN(doodle_file, get, ucom_t{} / "api" / "doodle" / "file", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http