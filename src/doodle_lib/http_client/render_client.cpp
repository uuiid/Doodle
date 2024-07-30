//
// Created by TD on 24-7-26.
//

#include "render_client.h"

#include <doodle_lib/core/http/json_body.h>

#include <boost/asio.hpp>

#include <wil/resource.h>
#include <wil/result.h>
namespace doodle::render_client {
namespace {
std::string get_user_name() {
  DWORD l_size = 0;
  auto l_err   = ::GetUserNameW(nullptr, &l_size);
  if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    LOG_IF_WIN32_ERROR(::GetLastError());
    return {"doodle"};
  }
  std::unique_ptr<wchar_t[]> l_user_name = std::make_unique<wchar_t[]>(l_size);
  l_err                                  = ::GetUserNameW(l_user_name.get(), &l_size);

  if (FALSE == l_err) {
    LOG_IF_WIN32_ERROR(::GetLastError());
    return {"doodle"};
  }
  l_size = l_size - 1;
  return boost::locale::conv::utf_to_utf<char>(l_user_name.get(), l_user_name.get() + l_size);
}
}  // namespace
boost::asio::awaitable<void> client::render(
    std::string in_name, FSys::path in_exe_path, std::vector<std::string> in_run_args
) {
  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::post, "v1/task", 11};
  l_req.body() = nlohmann::json{
      {"source_computer", boost::asio::ip::host_name()},
      {"submitter", get_user_name()},
      {"name", in_name},
      {"command", in_run_args},
      {"exe", in_exe_path.generic_string()}
  };
  l_req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<http::basic_json_body>(http_client_core_ptr_, std::move(l_req));
  if (l_ec) co_return;
}
boost::asio::awaitable<std::vector<computer>> client::get_computers() {
  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::get, "/v1/computer", 11};

  l_req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<http::basic_json_body>(http_client_core_ptr_, std::move(l_req));
  if (l_ec) co_return std::vector<computer>{};

  co_return l_res.body().get<std::vector<computer>>();
}
boost::asio::awaitable<std::tuple<std::vector<server_task_info>, std::size_t>> client::get_task(
    std::size_t in_begin, std::size_t in_count
) {
  boost::beast::http::request<boost::beast::http::string_body> l_req{
      boost::beast::http::verb::get, fmt::format("/v1/task?page={}&page_size={}", in_begin, in_count), 11
  };

  l_req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<http::basic_json_body>(http_client_core_ptr_, std::move(l_req));
  if (l_ec) co_return std::tuple<std::vector<server_task_info>, std::size_t>{};
  auto& l_body = l_res.body();
  co_return std::make_tuple(l_body["tasks"].get<std::vector<server_task_info>>(), l_body["size"].get<std::size_t>());
}
boost::asio::awaitable<std::string> client::get_logger(boost::uuids::uuid in_uuid, level::level_enum in_level) {
  boost::beast::http::request<boost::beast::http::string_body> l_req{
      boost::beast::http::verb::get, fmt::format("/v1/task/{}/log?level={}", in_uuid, magic_enum::enum_name(in_level)),
      11
  };

  l_req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<boost::beast::http::string_body>(http_client_core_ptr_, std::move(l_req));
  if (l_ec) co_return std::string{};

  co_return l_res.body();
}

boost::asio::awaitable<void> client::delete_task(boost::uuids::uuid in_uuid) {
  boost::beast::http::request<boost::beast::http::string_body> l_req{
      boost::beast::http::verb::delete_, fmt::format("/v1/task/{}", in_uuid), 11
  };
  l_req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.prepare_payload();
  auto [l_ec, l_res] =
      co_await http::detail::read_and_write<boost::beast::http::string_body>(http_client_core_ptr_, std::move(l_req));
  co_return;
}

}  // namespace doodle::render_client