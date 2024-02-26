//
// Created by TD on 2024/2/21.
//

#include "http_function.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/http_websocket_data.h>
namespace doodle::http {
std::tuple<bool, http_function::capture_t> http_function::set_match_url(boost::urls::segments_ref in_segments_ref
) const {
  std::map<std::string, std::string> l_str{};
  if (in_segments_ref.size() != capture_vector_.size()) {
    return {false, {}};
  }
  for (const auto& [l_cap, l_seg] : ranges::zip_view(capture_vector_, in_segments_ref)) {
    if (!l_cap.is_capture && l_cap.name != l_seg) {
      return {false, {}};
    }
    l_str.emplace(l_cap.name, l_seg);
  }
  return {true, capture_t{l_str}};
}
namespace detail {
void http_method_web_socket::upgrade_websocket(const entt::handle& in_handle) const {
  auto& l_read = in_handle.emplace<session::async_read_body<boost::beast::http::string_body>>(in_handle);
  l_read.async_end([](const boost::system::error_code& ec, const entt::handle& in_handle_1) {
    auto l_logger = in_handle_1.get<socket_logger>().logger_;
    if (ec == boost::beast::http::error::end_of_stream) {
      in_handle_1.get<http_session_data>().do_close();
      return;
    }
    if (ec) {
      l_logger->log(log_loc(), level::err, "on_read error: {}", ec);
      in_handle_1.get<http_session_data>().seed_error(
          boost::beast::http::status::internal_server_error,
          boost::system::errc::make_error_code(boost::system::errc::bad_message)
      );
      return;
    }
    boost::beast::get_lowest_layer(*in_handle_1.get<http_session_data>().stream_).expires_never();
    in_handle_1.emplace<http_websocket_data>(std::move(*in_handle_1.get<http_session_data>().stream_)).run();
    in_handle_1.erase<http_session_data>();
  });
}

void http_method_web_socket::operator()(const entt::handle& in_handle) const {
  if (boost::beast::websocket::is_upgrade(in_handle.get<http_session_data>().request_parser_->get())) {
    upgrade_websocket(in_handle);
    return;
  } else {
    operator_call(in_handle);
  }
}
}  // namespace detail
}  // namespace doodle::http
