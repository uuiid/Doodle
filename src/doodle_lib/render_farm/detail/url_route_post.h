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
  using map_action_type =
      std::map<std::string, std::function<void(std::shared_ptr<working_machine_session>, boost::urls::params_ref)>>;
  const map_action_type map_action;

 public:
  http_method()
      : map_action{
            {"render_job"s, [](std::shared_ptr<working_machine_session> in_session,
                               boost::urls::params_ref) { return render_job(std::move(in_session)); }},

            {"computer"s, [](std::shared_ptr<working_machine_session> in_session, boost::urls::params_ref) {
               return computer_reg(std::move(in_session));
             }}} {}
  void run(std::shared_ptr<working_machine_session> in_session);

  inline static std::string parser(
      const std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator>& in_segments
  ) {
    auto [l_begin, l_end] = in_segments;
    return *l_begin;
  }

  static void render_job(std::shared_ptr<working_machine_session> in_session);
  // 计算机注册
  static void computer_reg(std::shared_ptr<working_machine_session> in_session);
};
}  // namespace detail
}  // namespace doodle::render_farm
