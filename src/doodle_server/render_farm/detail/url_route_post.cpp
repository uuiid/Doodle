//
// Created by td_main on 2023/8/9.
//

#include "url_route_post.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_url.h>
#include <doodle_core/thread_pool/process_message.h>

#include "doodle_server_fwd.h"
#include <doodle_server/render_farm/detail/aync_read_body.h>
#include <doodle_server/render_farm/detail/computer.h>
#include <doodle_server/render_farm/detail/ue4_task.h>
#include <doodle_server/render_farm/work.h>
namespace doodle::render_farm {
namespace detail {

void render_job_type_post::operator()(
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
    return;
  }

  auto l_h = entt::handle{*g_reg(), g_reg()->create()};
  l_h.emplace<process_message>("解析json, 开始渲染");
  try {
    auto l_json = nlohmann::json::parse(in_request.body());
    l_h.emplace<ue4_task>(l_h, l_json.get<ue4_task::arg_t>());
  } catch (const nlohmann::json::exception& e) {
    log_error(l_logger, fmt::format("json parse error:{} ", e.what()));
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    session::do_write::send_error_code(in_handle, ec);
    return;
  }

  boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
  l_response.body() = {{"state", "ok"}, {"id", l_h.entity()}};
  l_response.keep_alive(in_request.keep_alive());
  l_response.prepare_payload();
  session::do_write{in_handle, std::move(l_response)}.run();
}

void get_log_type_post::operator()(
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
    return;
  }
  auto l_cap = in_handle.get<session::capture_url>().get<entt::id_type>("handle");
  if (!l_cap) {
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
    session::do_write::send_error_code(in_handle, ec);
    return;
  }
  auto l_h = entt::handle{*g_reg(), num_to_enum<entt::entity>(*l_cap)};
  if (!l_h || !l_h.all_of<process_message, ue4_task>()) {
    BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::bad_target);
    session::do_write::send_error_code(in_handle, ec);
    return;
  }
  auto& l_msg = l_h.get<process_message>();

  boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
  l_response.body() = {{"state", "ok"}, {"id", l_h.entity()}};
  l_response.keep_alive(in_request.keep_alive());
  l_response.prepare_payload();
  session::do_write{in_handle, std::move(l_response)}.run();
}
void get_err_type_post::operator()(
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
    return;
  }
  auto l_cap = in_handle.get<session::capture_url>().get<entt::id_type>("handle");
  if (!l_cap) {
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
    session::do_write::send_error_code(in_handle, ec);
    return;
  }
  auto l_h = entt::handle{*g_reg(), num_to_enum<entt::entity>(*l_cap)};
  if (!l_h || !l_h.all_of<process_message, ue4_task>()) {
    BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::bad_target);
    session::do_write::send_error_code(in_handle, ec);
    return;
  }
  auto& l_msg = l_h.get<process_message>();

  boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
  l_response.body() = {{"state", "ok"}, {"id", l_h.entity()}};
  l_response.keep_alive(in_request.keep_alive());
  l_response.prepare_payload();
  session::do_write{in_handle, std::move(l_response)}.run();
}
}  // namespace detail
}  // namespace doodle::render_farm