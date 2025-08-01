//
// Created by TD on 2024/2/21.
//

#include "http_route.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http {

class http_not_function : public http_function_template<http_not_function> {
 public:
  boost::asio::awaitable<boost::beast::http::message_generator> get(session_data_ptr in_handle) override {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
  }
  boost::asio::awaitable<boost::beast::http::message_generator> put(session_data_ptr in_handle) override {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
  }
  boost::asio::awaitable<boost::beast::http::message_generator> post(session_data_ptr in_handle) override {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
  }
  boost::asio::awaitable<boost::beast::http::message_generator> options(session_data_ptr in_handle) override {
    co_return in_handle->make_msg(std::string{});
  }
  boost::asio::awaitable<boost::beast::http::message_generator> delete_(session_data_ptr in_handle) override {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
  }
};

http_route::http_route() : default_function_(std::make_shared<http_not_function>()) {}

http_route& http_route::reg(url_route_component_t&& in_component, const http_function_ptr& in_function) {
  url_route_map_.emplace_back(std::make_shared<url_route_component_t>(std::move(in_component)), in_function);
  return *this;
}

http_function_ptr http_route::operator()(
    boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
) const {
  for (const auto& [i, l_data] : url_route_map_) {
    if (auto&& [l_m, l_cat] = i->set_match_url(in_segment, l_data); l_m) {
      return l_cat;
    }
  }
  return default_function_;
}

}  // namespace doodle::http