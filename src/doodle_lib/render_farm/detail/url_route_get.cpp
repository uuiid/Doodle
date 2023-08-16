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

void get_log_type_get::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  if (in_cap.count("handle") == 0) {
    throw_exception(doodle_error("not found handle"));
  }
  auto l_h        = entt::handle{*g_reg(), num_to_enum<entt::entity>(std::stoi(in_cap.at("handle")))};
  auto& l_session = in_handle.get<working_machine_session>();
  if (l_h && l_h.all_of<process_message>()) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = l_h.get<process_message>().log();
    l_response.keep_alive(l_session.request_parser().keep_alive());
    l_session.send_response(std::move(l_response));
  } else {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.keep_alive(l_session.request_parser().keep_alive());
    l_response.body() = "not found";
    l_session.send_response(std::move(l_response));
  }
}
void get_err_type_get::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  if (in_cap.count("handle") == 0) {
    throw_exception(doodle_error("not found handle"));
  }
  auto l_h        = entt::handle{*g_reg(), num_to_enum<entt::entity>(std::stoi(in_cap.at("handle")))};
  auto& l_session = in_handle.get<working_machine_session>();

  if (l_h && l_h.all_of<process_message>()) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = l_h.get<process_message>().err();
    l_response.keep_alive(l_session.request_parser().keep_alive());
    l_session.send_response(std::move(l_response));
  } else {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.keep_alive(l_session.request_parser().keep_alive());
    l_response.body() = "not found";
    l_session.send_response(std::move(l_response));
  }
}
void render_job_type_get::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  auto l_view     = g_reg()->view<render_farm::ue4_task_ptr>();
  auto l_ids      = l_view | ranges::to_vector;
  auto& l_session = in_handle.get<working_machine_session>();
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

void computer_reg_type_get::operator()(const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap)
    const {
  auto l_view = g_reg()->view<render_farm::computer>().each();
  auto l_ids  = l_view | ranges::views::transform([](auto&& in_item) -> computer_tmp {
                 auto& l_computer = std::get<1>(in_item);
                 return computer_tmp{l_computer, std::get<0>(in_item)};
               }) |
               ranges::to_vector;
  boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
  auto& l_session   = in_handle.get<working_machine_session>();
  l_response.body() = l_ids;
  l_response.keep_alive(l_session.request_parser().keep_alive());
  l_response.insert(boost::beast::http::field::content_type, "application/json");

  l_session.send_response(std::move(l_response));
}
}  // namespace doodle::render_farm::detail