//
// Created by TD on 24-7-26.
//

#include "render_client.h"

#include <boost/asio.hpp>

#include "core/http/json_body.h"
namespace doodle::render_client {
boost::asio::awaitable<void> client::render(
    std::string in_name, FSys::path in_exe_path, std::vector<std::string> in_run_args
) {
  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::post, "v1/task", 11};
  l_req.body() = nlohmann::json{
      {"source_computer", boost::asio::ip::host_name()},
      {"submitter", boost::asio::ip::host_name()},
      {"name", in_name},
      {"command", in_run_args},
      {"exe", in_exe_path.generic_string()}
  };
  l_req.set(boost::beast::http::field::host, "192.168.40.181");
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<http::basic_json_body>(http_client_core_ptr_, std::move(l_req));
  if (l_ec) co_return;
}
boost::asio::awaitable<std::vector<computer>> client::get_computers() {
  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::get, "v1/computer", 11};

  l_req.set(boost::beast::http::field::host, "192.168.40.181");
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<http::basic_json_body>(http_client_core_ptr_, std::move(l_req));
  if (l_ec) co_return std::vector<computer>{};

  co_return l_res.body().get<std::vector<computer>>();
}
boost::asio::awaitable<std::vector<server_task_info>> client::get_task() {
  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::get, "v1/task", 11};

  l_req.set(boost::beast::http::field::host, "192.168.40.181");
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<http::basic_json_body>(http_client_core_ptr_, std::move(l_req));
  if (l_ec) co_return std::vector<server_task_info>{};

  co_return l_res.body().get<std::vector<server_task_info>>();
}
boost::asio::awaitable<std::string> client::get_logger(boost::uuids::uuid in_uuid, level::level_enum in_level) {
  boost::beast::http::request<boost::beast::http::string_body> l_req{
      boost::beast::http::verb::get, fmt::format("v1/task/{}/log?level={}", in_uuid, magic_enum::enum_name(in_level)),
      11
  };

  l_req.set(boost::beast::http::field::host, "192.168.40.181");
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<boost::beast::http::string_body>(http_client_core_ptr_, std::move(l_req));
  if (l_ec) co_return std::string{};

  co_return l_res.body();
}
}  // namespace doodle::render_client