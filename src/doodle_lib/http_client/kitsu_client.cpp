#include "kitsu_client.h"

namespace doodle::kitsu {
boost::asio::awaitable<std::tuple<boost::system::error_code, kitsu_client::task>> kitsu_client::get_task(
  const boost::uuids::uuid& in_uuid
) {
  boost::beast::http::request<boost::beast::http::empty_body> req{
      boost::beast::http::verb::get, fmt::format("/api/data/tasks/{}/full", in_uuid), 11
  };

  auto [l_e, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(
    http_client_core_ptr_, header_operator_req(std::move(req))
  );
  if (l_e) {
    co_return std::make_tuple(l_e, task{});
  }
  header_operator_resp(l_res);
  task l_task{};
  try {
    l_task = nlohmann::json::parse(l_res.body()).get<task>();
  } catch (const nlohmann::json::exception& e) {
    http_client_core_ptr_->logger_->log(log_loc(), level::err, "get task failed: {}", e.what());
    co_return std::make_tuple(boost::system::errc::make_error_code(boost::system::errc::invalid_argument), l_task);
  }
  co_return std::make_tuple(l_e, l_task);
}

boost::asio::awaitable<std::tuple<boost::system::error_code, kitsu_client::user_t>> kitsu_client::get_user(
  const boost::uuids::uuid& in_uuid
) {
  boost::beast::http::request<boost::beast::http::empty_body> req{
      boost::beast::http::verb::get, fmt::format("/api/data/persons/{}", in_uuid), 11
  };

  user_t l_user;
  auto [l_e, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(
    http_client_core_ptr_, header_operator_req(std::move(req))
  );
  if (l_e) {
    co_return std::make_tuple(l_e, l_user);
  }
  header_operator_resp(l_res);
  try {
    auto l_user = nlohmann::json::parse(l_res.body()).get<user_t>();
  } catch (const nlohmann::json::exception& e) {
    http_client_core_ptr_->logger_->log(log_loc(), level::err, "get user failed: {}", e.what());
    co_return std::make_tuple(boost::system::errc::make_error_code(boost::system::errc::invalid_argument), l_user);
  }
  co_return std::make_tuple(l_e, l_user);
}
} // namespace doodle::kitsu