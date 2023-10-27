//
// Created by td_main on 2023/8/9.
//

#include "url_route_base.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_server/render_farm/http_session.h>
#include <doodle_server/render_farm/websocket.h>
namespace doodle::render_farm::detail {

void http_route::capture_url::set_cap_bit() {
  for (const auto& l_str : capture_vector_) {
    if (l_str.front() == '{' && l_str.back() == '}') {
      capture_bitset_.push_back(true);
    } else {
      capture_bitset_.push_back(false);
    }
  }
}
std::tuple<bool, std::map<std::string, std::string>> http_route::capture_url::match_url(
    boost::urls::segments_ref in_segments_ref
) const {
  auto l_it = in_segments_ref.begin();
  std::map<std::string, std::string> l_str{};
  if (in_segments_ref.size() != capture_vector_.size()) {
    return {false, l_str};
  }
  bool l_result{true};
  for (auto i = 0; l_it != in_segments_ref.end(); ++l_it, ++i) {
    if (capture_bitset_[i]) {
      l_str.emplace(capture_vector_[i].substr(1, capture_vector_[i].size() - 2), *l_it);
    } else {
      if (capture_vector_[i] != *l_it) {
        l_result = false;
        break;
      }
    }
  }
  return {l_result, l_str};
}

void http_route::upgrade_websocket(const entt::handle& in_handle) {
  struct upgrade_websocket_data {
    void operator()(
        boost::system::error_code ec, entt::handle in_handle,
        boost::beast::http::request<boost::beast::http::string_body> in_msg
    ) {
      auto l_logger = in_handle.get<socket_logger>().logger_;
      if (ec == boost::beast::http::error::end_of_stream) {
        session::do_close{in_handle}.run();
      }
      if (ec) {
        log_error(l_logger, fmt::format("on_read error: {} ", ec));
        session::do_write::send_error_code(in_handle, ec);
        return;
      }
      boost::beast::get_lowest_layer(*in_handle.get<http_session_data>().stream_).expires_never();
      in_handle.emplace<render_farm::websocket_data>(std::move(*in_handle.get<http_session_data>().stream_));
      in_handle.erase<http_session_data>();
      std::make_shared<render_farm::websocket>(in_handle)->run(std::move(in_msg));
    }
  };
  using do_read_msg_body = session::do_read_msg_body<
      boost::beast::http::string_body, upgrade_websocket_data, boost::asio::any_io_executor>;
  auto l_exe = in_handle.get<http_session_data>().stream_->get_executor();
  do_read_msg_body{in_handle, upgrade_websocket_data{}, l_exe}.run();
}

http_route::action_type http_route::capture_url::operator()(boost::urls::segments_ref in_segments_ref) const {
  auto [l_result, l_map] = match_url(in_segments_ref);
  if (l_result) {
    return [map_ = l_map, call = action_](const entt::handle& in_handle) {
      in_handle.emplace_or_replace<session::capture_url>(map_);
      call(in_handle);
    };
  } else
    return {};
}
void http_route::reg(
    boost::beast::http::verb in_verb, std::vector<std::string> in_vector,
    http_route::capture_url::action_type in_function
) {
  actions[in_verb].emplace_back(std::move(in_vector), std::move(in_function));
}

http_route::action_type http_route::operator()(boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment)
    const {
  auto l_it = actions.find(in_verb);
  if (l_it != actions.end()) {
    for (auto&& i : l_it->second) {
      auto l_action = i(in_segment);
      if (l_action) {
        return l_action;
      }
    }
  }
  return {};
}

}  // namespace doodle::render_farm::detail
