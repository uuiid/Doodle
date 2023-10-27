//
// Created by td_main on 2023/10/19.
//

#include "project.h"

#include <doodle_core/doodle_core_fwd.h>

#include <boost/beast.hpp>

#include "core/http_session.h"
#include <doodle_server/data/project_list.h>
#include <doodle_server/render_farm/render_farm_fwd.h>
namespace doodle::http::project {
void get_type::operator()(const entt::handle &in_handle) const {
  auto &l_list     = g_ctx().get<project_storage_list_type>().project_list_;

  auto l_name_list = l_list | ranges::views::keys | ranges::to<std::vector<std::string>>();
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok,
      in_handle.get<render_farm::session::request_parser_empty_body>()->get().version()};
  l_response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.keep_alive(in_handle.get<render_farm::session::request_parser_empty_body>()->keep_alive());
  l_response.body() = nlohmann::json(l_name_list).dump();
  l_response.prepare_payload();
  render_farm::session::do_write{in_handle, std::move(l_response)}.run();
}

void post_type::operator()(
    boost::system::error_code ec, const entt::handle &in_handle,
    const boost::beast::http::request<boost::beast::http::string_body> &in_request
) const {
  auto l_logger = in_handle.get<socket_logger>().logger_;
  if (ec == boost::beast::http::error::end_of_stream) {
    render_farm::session::do_close{in_handle}.run();
    return;
  }
  if (ec) {
    log_error(l_logger, fmt::format("on_read error: {} ", ec));
    render_farm::session::do_write::send_error_code(in_handle, ec);
    return;
  }

  auto l_body_str = in_request.body();
  if (!nlohmann::json::accept(l_body_str)) {
    log_error(l_logger, fmt::format("json parse error: {} ", ec));
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    render_farm::session::do_write::send_error_code(in_handle, ec);
    return;
  }

  auto l_json = nlohmann::json::parse(l_body_str);
  auto l_cap  = in_handle.get<render_farm::session::capture_url>().get<std::string>("name");
  if (!l_cap) {
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
    render_farm::session::do_write::send_error_code(in_handle, ec);
    return;
  }
  auto &l_list = g_ctx().get<project_storage_list_type>().project_list_;
  if (!l_list.contains(*l_cap)) {
    l_list.emplace(
        *l_cap, project_storage_type{(g_ctx().get<project_storage_list_type>().project_root / *l_cap)
                                         .replace_extension(doodle_config::doodle_db_name)}
    );
  }

  try {
    l_list.at(*l_cap).get_registry().ctx().get<project_config::base_config>() =
        l_json.get<project_config::base_config>();
  } catch (const nlohmann::json::exception &e) {
    log_error(l_logger, fmt::format("json parse error: {} ", e.what()));
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    render_farm::session::do_write::send_error_code(in_handle, ec);
    return;
  }

  boost::beast::http::response<boost::beast::http::empty_body> l_response{
      boost::beast::http::status::ok,
      in_handle.get<boost::beast::http::request<boost::beast::http::string_body>>().version()};
  l_response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_response.keep_alive(in_handle.get<render_farm::session::request_parser_empty_body>()->keep_alive());
  l_response.prepare_payload();
  render_farm::session::do_write{in_handle, std::move(l_response)}.run();
}

// void put_type::operator()(boost::system::error_code ec, const entt::handle &in_handle) const {}

void delete_type::operator()(boost::system::error_code ec, const entt::handle &in_handle) const {
  auto l_logger = in_handle.get<socket_logger>().logger_;
  if (ec == boost::beast::http::error::end_of_stream) {
    render_farm::session::do_close{in_handle}.run();
    return;
  }
  if (ec) {
    log_error(l_logger, fmt::format("on_read error: {} ", ec));
    render_farm::session::do_write::send_error_code(in_handle, ec);
    return;
  }

  auto l_cap = in_handle.get<render_farm::session::capture_url>().get<std::string>("name");
  if (!l_cap) {
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
    render_farm::session::do_write::send_error_code(in_handle, ec);
    return;
  }
  auto &l_list = g_ctx().get<project_storage_list_type>().project_list_;
  if (!l_list.contains(*l_cap)) {
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::project_not_exist);
    render_farm::session::do_write::send_error_code(in_handle, ec);
    return;
  }
  l_list.erase(*l_cap);

  auto l_path = g_ctx().get<project_storage_list_type>().project_root / *l_cap;
  l_path.replace_extension(doodle_config::doodle_db_name);
  if (FSys::exists(l_path)) {
    FSys::remove(l_path);
  }

  boost::beast::http::response<boost::beast::http::empty_body> l_response{
      boost::beast::http::status::ok,
      in_handle.get<boost::beast::http::request<boost::beast::http::string_body>>().version()};
  l_response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_response.keep_alive(in_handle.get<render_farm::session::request_parser_empty_body>()->keep_alive());
  l_response.prepare_payload();
  render_farm::session::do_write{in_handle, std::move(l_response)}.run();
}
}  // namespace doodle::http::project