//
// Created by td_main on 2023/8/17.
//
#include "url_route_put.h"

#include <doodle_core/thread_pool/process_message.h>
namespace doodle::render_farm {
namespace detail {
void render_job_type_put::operator()(
    boost::system::error_code ec, const entt::handle& in_handle,
    boost::beast::http::request<boost::beast::http::string_body> in_request
) const {
  auto l_logger = in_handle.get<socket_logger>().logger_;
  if (ec == boost::beast::http::error::end_of_stream) {
    session::do_close{in_handle}.run();
    return;
  }
  if (ec) {
    log_error(l_logger, fmt::format("on_read error: {} ", ec));
    session::do_write::send_error_code(in_handle, ec);
  }

  auto l_cap = in_handle.get<session::capture_url>().get<entt::id_type>("handle");
  if (!l_cap) {
    log_error(l_logger, fmt::format("on_read error: {} ", ec));
    session::do_write::send_error_code(in_handle, ec);
    return;
  }
  auto l_handle = entt::handle{*g_reg(), num_to_enum<entt::entity>(*l_cap)};
  if (!l_handle || !l_handle.all_of<process_message>()) {
    BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::bad_target);
    session::do_write::send_error_code(in_handle, ec);
    return;
  }
  auto l_body_str = in_request.body();
  if (!nlohmann::json ::accept(l_body_str)) {
    log_error(l_logger, fmt::format("json parse error: {} ", ec));
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    session::do_write::send_error_code(in_handle, ec);
    return;
  }

  auto l_json = nlohmann::json::parse(l_body_str);
  if (l_json.contains("state")) {
    auto l_state = l_json["state"].get<process_message::state>();
    l_handle.get<process_message>().set_state(l_state);

    auto l_response   = boost::beast::http::response<detail::basic_json_body>{boost::beast::http::status::ok, 11};
    l_response.body() = {{"state", "ok"}};
    l_response.keep_alive(in_request.keep_alive());
    l_response.prepare_payload();
    session::do_write{in_handle, std::move(l_response)}.run();
  } else {
    log_error(l_logger, fmt::format("json parse error: state is not valid"));
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    session::do_write::send_error_code(in_handle, ec);
  }
}
}  // namespace detail
}  // namespace doodle::render_farm