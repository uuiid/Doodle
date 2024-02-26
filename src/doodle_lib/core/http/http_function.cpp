//
// Created by TD on 2024/2/21.
//

#include "http_function.h"

#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>
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
  if (in_handle && in_handle.any_of<http_session_data, session::request_parser_empty_body>()) {
  } else {
    default_logger_raw()->log(log_loc(), level::err, "upgrade_websocket 无效的句柄");
    boost::system::error_code l_error_code{};
    BOOST_BEAST_ASSIGN_EC(l_error_code, error_enum::invalid_handle);
  }
}

void http_method_web_socket::operator()(const entt::handle& in_handle) const {
  if (boost::beast::websocket::is_upgrade(in_handle.get<session::request_parser_empty_body>()->get())) {
    upgrade_websocket(in_handle);
    return;
  } else {
    operator_call(in_handle);
  }
}
}  // namespace detail
}  // namespace doodle::http
