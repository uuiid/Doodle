//
// Created by td_main on 2023/8/9.
//
#pragma once
#include <doodle_lib/render_farm/detail/url_route_base.h>

#include <boost/url.hpp>
namespace doodle::render_farm {
namespace detail {

class url_rote_get : public url_route_base {
  void get_uuiid();
  // 方法
  std::string method_;
  entt::handle handle_;

 public:
  using call_back =
      std::function<boost::beast::http::message_generator(const boost::urls::params_ref&, const entt::handle&)>;
  std::map<std::string, call_back> call_back_map_;

 public:
  boost::beast::http::message_generator run(
      const std::shared_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>>& in_request_parser
  ) override;
};
}  // namespace detail

}  // namespace doodle::render_farm
