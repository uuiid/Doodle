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

boost::asio::awaitable<std::shared_ptr<kitsu_client_factory::kitsu_client_guard>> kitsu_client_factory::create_client(

) {
  // todo 使用线程池
  auto l_this_exe = co_await boost::asio::this_coro::executor;

  // 转换到自己的线程
  co_await boost::asio::post(boost::asio::bind_executor(executor_, boost::asio::use_awaitable));

  auto l_value = std::make_shared<kitsu_client_guard>(std::make_shared<kitsu_client>(g_io_context(), ""));
  // 恢复线程
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
  co_return l_value;


}

}  // namespace doodle::kitsu