//
// Created by td_main on 2023/8/9.
//

#include "url_route_get.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
namespace doodle::render_farm::detail {

http_method<boost::beast::http::verb::get>::http_method()
    : map_action{
          {"get_log"s, [this](const entt::handle& in_h, boost::urls::params_ref) { return get_log(in_h); }},
          {"get_err"s, [this](const entt::handle& in_h, boost::urls::params_ref) { return get_err(in_h); }},
          {"render_job"s, [this](const entt::handle&, boost::urls::params_ref) { return render_job(); }},
          {"computer"s, [this](const entt::handle&, boost::urls::params_ref) { return computer_reg(); }},
      } {}
void http_method<boost::beast::http::verb::get>::run(const std::shared_ptr<working_machine_session>& in_session) {
  auto [l_handle, l_method] = parser(chick_url(in_session->url_.segments()));
  keep_alive_               = in_session->request_parser_.keep_alive();
  if (map_action.count(l_method) == 0) {
    boost::beast::http::response<boost::beast::http::empty_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.keep_alive(false);
    in_session->send_response(boost::beast::http::message_generator{std::move(l_response)});
  } else {
    in_session->send_response(map_action.at(l_method)(l_handle, in_session->url_.params()));
  }
}

std::tuple<entt::handle, std::string> http_method<boost::beast::http::verb::get>::parser(
    const std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator>& in_segments
) {
  auto [l_begin, l_end] = in_segments;
  entt::handle l_handle{*g_reg(), entt::null};
  if (std::regex_match(*l_begin, std::regex{R"(^\d+$)"})) {
    auto l_uuid = std::stoi(*l_begin);
    l_handle    = {*g_reg(), num_to_enum<entt::entity>(l_uuid)};
    if (!l_handle) {
      throw_exception(doodle_error{"url not found id"});
    }

    ++l_begin;
    if (l_begin != l_end) {
      throw_exception(doodle_error{"url not found method"});
    }

    auto l_method = *l_begin;
    if (l_method.empty()) {
      throw_exception(doodle_error{" url method is empty"});
    }
    return {l_handle, l_method};
  }
  return {l_handle, *l_begin};
}

boost::beast::http::message_generator http_method<boost::beast::http::verb::get>::render_job() const {
  auto l_view = g_reg()->view<render_farm::render_ue4_ptr>();
  auto l_ids  = l_view | ranges::to_vector;
  boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
  l_response.body() = l_ids;
  l_response.keep_alive(keep_alive_);
  l_response.insert(boost::beast::http::field::content_type, "application/json");
  return {std::move(l_response)};
}

namespace {
struct computer_tmp {
  std::string name;
  std::string status;
  explicit computer_tmp(const computer& in_computer)
      : name(in_computer.name()), status(magic_enum::enum_name(in_computer.status())) {}

  friend void to_json(nlohmann::json& j, const computer_tmp& in_tmp) {
    j = nlohmann::json{{"name", in_tmp.name}, {"status", in_tmp.status}};
  }
};
}  // namespace
boost::beast::http::message_generator http_method<boost::beast::http::verb::get>::computer_reg() const {
  auto l_view = g_reg()->view<render_farm::computer>().each();
  auto l_ids  = l_view | ranges::views::transform([](auto&& in_item) -> computer_tmp {
                 auto& l_computer = std::get<1>(in_item);
                 return computer_tmp{l_computer};
               }) |
               ranges::to_vector;
  boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, 11};
  l_response.body() = l_ids;
  l_response.keep_alive(keep_alive_);
  l_response.insert(boost::beast::http::field::content_type, "application/json");
  return {std::move(l_response)};
}

boost::beast::http::message_generator http_method<boost::beast::http::verb::get>::get_err(const entt::handle& in_h
) const {
  boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
  l_response.keep_alive(keep_alive_);
  l_response.body() = in_h.get<process_message>().err();
  return l_response;
}
boost::beast::http::message_generator http_method<boost::beast::http::verb::get>::get_log(const entt::handle& in_h
) const {
  boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
  l_response.keep_alive(keep_alive_);
  l_response.body() = in_h.get<process_message>().log();
  return l_response;
}

}  // namespace doodle::render_farm::detail