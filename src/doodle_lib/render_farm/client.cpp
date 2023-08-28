//
// Created by td_main on 2023/8/18.
//

#include "client.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/core/bind_front_handler.h>

#include <boost/asio.hpp>
namespace doodle {


boost::beast::http::message_generator client::task_list_t::operator()() {
  boost::beast::http::request<render_farm::detail::basic_json_body> l_request{
      boost::beast::http::verb::get, "/v1/render_farm/render_job", 11};
  l_request.keep_alive(true);
  l_request.set(boost::beast::http::field::host, fmt::format("{}:50021", ptr_->server_ip()));
  l_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  return {std::move(l_request)};
}
client::task_list_t::result_type client::task_list_t::operator()(const detail::client_core::response_type& in_response
) {
  return in_response.body().get<std::vector<task_t>>();
}

boost::beast::http::message_generator client::computer_list_t::operator()() {
  boost::beast::http::request<render_farm::detail::basic_json_body> l_request{
      boost::beast::http::verb::get, "/v1/render_farm/computer", 11};
  l_request.keep_alive(true);
  l_request.set(boost::beast::http::field::host, fmt::format("{}:50021", ptr_->server_ip()));
  l_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  return {std::move(l_request)};
}
client::computer_list_t::result_type client::computer_list_t::operator()(
    const detail::client_core::response_type& in_response
) {
  return in_response.body().get<std::vector<computer>>();
}

}  // namespace doodle