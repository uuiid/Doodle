//
// Created by td_main on 2023/8/9.
//

#pragma once
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <nlohmann/json.hpp>
#include <utility>
namespace doodle::render_farm {
namespace detail {

template <>
class http_method<boost::beast::http::verb::post> {
  using map_action_type = std::map<std::string, std::function<void(const entt::handle &)>>;
  const map_action_type map_action;

 public:
  http_method(map_action_type in_map_action) : map_action{std::move(in_map_action)} {}
  void run(const entt::handle &in_session);

  inline static auto make_client() {
    return http_method{
        {{"client_submit_job"s, [](const entt::handle &in_handle) { return client_submit_job(in_handle); }}}};
  }
  inline static auto make_server() {
    return http_method{
        {{"render_job"s, [](const entt::handle &in_handle) { return render_job(in_handle); }},
         {"computer_reg"s, [](const entt::handle &in_handle) { return computer_reg(in_handle); }}}};
  }
  inline static auto make_work() {
    return http_method{
        {{"computer_reg"s, [](const entt::handle &in_handle) { return computer_reg(in_handle); }},
         {"run_job", [](const entt::handle &in_handle) { return run_job(in_handle); }}}};
  }

  inline static std::string parser(
      const std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator> &in_segments
  ) {
    auto [l_begin, l_end] = in_segments;
    return *l_begin;
  }

  static void render_job(const entt::handle &in_handle);

  static void client_submit_job(const entt::handle &in_handle);

  // 计算机注册
  static void computer_reg(const entt::handle &in_handle);
  /**
   * @brief 运行任务
   * @param in_handle 会话
   *
   * 传入的json
   * json["id'] -> entt::entity
   * json["arg"] -> ue_render::arg
   *
   */

  static void run_job(const entt::handle &in_handle);
};
}  // namespace detail
}  // namespace doodle::render_farm
