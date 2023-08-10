//
// Created by td_main on 2023/8/9.
//

#pragma once

#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::render_farm {
namespace detail {
class url_route_base {
  void chick_v1_render_farm();

 protected:
  boost::urls::segments_ref segments_ref_;

 public:
  explicit url_route_base(boost::urls::segments_ref in_ref) : segments_ref_(std::move(in_ref)) {
    chick_v1_render_farm();
  };
  virtual ~url_route_base() = default;
  virtual boost::beast::http::message_generator run(
      const std::shared_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>>& in_request_parser
  ) = 0;
};

}  // namespace detail
}  // namespace doodle::render_farm