//
// Created by td_main on 2023/8/9.
//
#pragma once
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::render_farm {
namespace detail {

template <>
class http_method<boost::beast::http::verb::get> {
  using map_actin_type = std::map<
      std::string, std::function<boost::beast::http::message_generator(const entt::handle&, boost::urls::params_ref)>>;
  const map_actin_type map_action;

 public:
  http_method();

  void run(std::shared_ptr<working_machine_session> in_session);

  static boost::beast::http::message_generator get_log(const entt::handle& in_h);

  static boost::beast::http::message_generator get_err(const entt::handle& in_h);
  static boost::beast::http::message_generator render_job();

  static std::tuple<entt::handle, std::string> parser(
      const std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator>& in_segments
  );
};

}  // namespace detail

}  // namespace doodle::render_farm
