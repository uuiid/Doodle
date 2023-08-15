//
// Created by td_main on 2023/8/9.
//

#include "url_route_get.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/detail/ue4_task.h>
namespace doodle::render_farm::detail {

void http_method<boost::beast::http::verb::get>::run(const entt::handle& in_handle) {
  auto& l_session           = in_handle.get<working_machine_session>();
  auto [l_handle, l_method] = parser(chick_url(l_session.url().segments()));
  if (map_action.count(l_method) == 0) {
    boost::beast::http::response<boost::beast::http::empty_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.keep_alive(false);
    l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
  } else {
    in_handle.emplace<http_method_get_handle>(l_handle);
    map_action.at(l_method)(in_handle);
  }
}

std::tuple<entt::handle, std::string> http_method<boost::beast::http::verb::get>::parser(
    const std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator>& in_segments
) {
  auto [l_begin, l_end] = in_segments;
  entt::handle l_handle{*g_reg(), entt::null};
  auto l_method = *l_begin;
  if (l_method.empty()) {
    throw_exception(doodle_error{" url method is empty"});
  }
  ++l_begin;
  if (l_begin == l_end) {
    return {l_handle, l_method};
  }
  if (std::regex_match(*l_begin, std::regex{R"(^\d+$)"})) {
    auto l_uuid = std::stoi(*l_begin);
    l_handle    = {*g_reg(), num_to_enum<entt::entity>(l_uuid)};
    if (!l_handle) {
      throw_exception(doodle_error{"url not found id"});
    }
    return {l_handle, l_method};
  } else {
    throw_exception(doodle_error{"url id error"});
  }
  return {l_handle, l_method};
}

void http_method<boost::beast::http::verb::get>::render_job(const entt::handle& in_h) {
  auto l_view     = g_reg()->view<render_farm::ue4_task_ptr>();
  auto l_ids      = l_view | ranges::to_vector;
  auto& l_session = in_h.get<working_machine_session>();
  boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
  l_response.body() = l_ids;
  l_response.keep_alive(l_session.request_parser().keep_alive());
  l_response.insert(boost::beast::http::field::content_type, "application/json");
  l_session.send_response(std::move(l_response));
}

namespace {
struct computer_tmp {
  std::string name;
  std::string status;
  entt::entity id;
  explicit computer_tmp(const computer& in_computer, entt::entity in_id)
      : name(in_computer.name()), status(magic_enum::enum_name(in_computer.status())), id(in_id) {}

  friend void to_json(nlohmann::json& j, const computer_tmp& in_tmp) {
    j["name"]   = in_tmp.name;
    j["status"] = in_tmp.status;
    j["id"]     = in_tmp.id;
  }
};
}  // namespace
void http_method<boost::beast::http::verb::get>::computer_reg(const entt::handle& in_h) {
  auto l_view = g_reg()->view<render_farm::computer>().each();
  auto l_ids  = l_view | ranges::views::transform([](auto&& in_item) -> computer_tmp {
                 auto& l_computer = std::get<1>(in_item);
                 return computer_tmp{l_computer, std::get<0>(in_item)};
               }) |
               ranges::to_vector;
  boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
  auto& l_session   = in_h.get<working_machine_session>();
  l_response.body() = l_ids;
  l_response.keep_alive(l_session.request_parser().keep_alive());
  l_response.insert(boost::beast::http::field::content_type, "application/json");

  l_session.send_response(std::move(l_response));
}

void http_method<boost::beast::http::verb::get>::get_err(const entt::handle& in_h) {
  auto& l_session = in_h.get<working_machine_session>();
  auto l_h        = in_h.get<http_method_get_handle>().handle_;
  if (l_h && l_h.all_of<process_message>()) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = in_h.get<http_method_get_handle>().handle_.get<process_message>().err();
    l_response.keep_alive(l_session.request_parser().keep_alive());
    l_session.send_response(std::move(l_response));
  } else {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.keep_alive(l_session.request_parser().keep_alive());
    l_response.body() = "not found";
    l_session.send_response(std::move(l_response));
  }
}
void http_method<boost::beast::http::verb::get>::get_log(const entt::handle& in_h) {
  auto& l_session = in_h.get<working_machine_session>();
  auto l_h        = in_h.get<http_method_get_handle>().handle_;
  if (l_h && l_h.all_of<process_message>()) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = in_h.get<http_method_get_handle>().handle_.get<process_message>().log();
    l_response.keep_alive(l_session.request_parser().keep_alive());
    l_session.send_response(std::move(l_response));
  } else {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.keep_alive(l_session.request_parser().keep_alive());
    l_response.body() = "not found";
    l_session.send_response(std::move(l_response));
  }
}

}  // namespace doodle::render_farm::detail