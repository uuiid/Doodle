//
// Created by td_main on 2023/8/9.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>

namespace doodle::render_farm::detail {

template <>
class http_method<boost::beast::http::verb::get> {
  using map_actin_type = std::map<std::string, std::function<void(const entt::handle&)>>;
  const map_actin_type map_action;

  using http_method_get_handle = doodle::detail::entt_handle<entt::tag<"http_method_get_handle"_hs>>;

 public:
  http_method(map_actin_type in_map_actin_type) : map_action{std::move(in_map_actin_type)} {};
  // http_method()
  //      : map_action{
  //            {"get_log"s, [this](const entt::handle& in_h, boost::urls::params_ref) { return get_log(in_h); }},
  //            {"get_err"s, [this](const entt::handle& in_h, boost::urls::params_ref) { return get_err(in_h); }},
  //            {"render_job"s, [this](const entt::handle&, boost::urls::params_ref) { return render_job(); }},
  //            {"computer"s, [this](const entt::handle&, boost::urls::params_ref) { return computer_reg(); }},
  //        } {}

  inline static auto make_server() {
    return http_method{
        {//
         {"get_log"s, [](const entt::handle& in_h) { return get_log(in_h); }},
         {"get_err"s, [](const entt::handle& in_h) { return get_err(in_h); }},
         {"render_job"s, [](const entt::handle& in_h) { return render_job(in_h); }},
         {"computer"s, [](const entt::handle& in_h) { return computer_reg(in_h); }}}  //
    };
  }

  void run(const entt::handle& in_session);

  [[nodiscard("")]] static boost::beast::http::message_generator get_log(const entt::handle& in_h);

  [[nodiscard("")]] static boost::beast::http::message_generator get_err(const entt::handle& in_h);
  [[nodiscard("")]] static boost::beast::http::message_generator render_job(const entt::handle& in_h);
  [[nodiscard("")]] static boost::beast::http::message_generator computer_reg(const entt::handle& in_h);

  static std::tuple<entt::handle, std::string> parser(
      const std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator>& in_segments
  );
};

}  // namespace doodle::render_farm::detail
