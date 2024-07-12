#include "dingding_client.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_beast.h>

namespace doodle::dingding {
boost::asio::awaitable<void> client::begin_refresh_token() {
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    co_return;
  }

  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, "/v1.0/oauth2/accessToken", 11
  };
  req.body() = nlohmann::json{{"appKey", app_key}, {"appSecret", app_secret}}.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
  auto [l_e, l_res] = co_await http::detail::read_and_write<http::basic_json_body>(
      http_client_core_ptr_, header_operator_req(std::move(req))
  );
  if (l_e) {
    http_client_core_ptr_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_e);
    co_return;
  }
  if (l_res.result() != boost::beast::http::status::ok) {
    http_client_core_ptr_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_res.result());
    co_return;
  }

  auto& l_json              = l_res.body();
  access_token_             = l_json["accessToken"].get<std::string>();
  chrono::seconds l_seconds = chrono::seconds(l_json["expireIn"].get<int>());

  boost::asio::co_spawn(
      http_client_core_ptr_->get_executor(), clear_token(l_seconds),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}

boost::asio::awaitable<void> client::clear_token(chrono::seconds in_seconds) {
  timer_ptr_->expires_after(in_seconds);
  // 防止析构
  auto l_     = shared_from_this();
  auto [l_ex] = co_await timer_ptr_->async_wait();
  access_token_.clear();
  if (l_ex) {
    http_client_core_ptr_->logger_->log(log_loc(), level::warn, "timer_ptr_ error: {}", l_ex);
    co_return;
  }
  co_return;
}

void client::access_token(const std::string& in_app_key, const std::string& in_app_secret) {
  app_key    = in_app_key;
  app_secret = in_app_secret;
}

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> client::get_user_by_mobile(
    const std::string& in_mobile
) {
  if (access_token_.empty()) co_await begin_refresh_token();

  std::string l_ret{};
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, fmt::format("/topapi/v2/user/getbymobile?access_token={}", access_token_), 11
  };
  req.body() = nlohmann::json{{"mobile", in_mobile}}.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::host, http_client_core_ptr_old_->server_ip_);
  auto [l_e, l_res] = co_await http::detail::read_and_write<http::basic_json_body>(
      http_client_core_ptr_old_, header_operator_req(std::move(req))
  );
  if (l_e) {
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_e);
    co_return std::make_tuple(l_e, l_ret);
  }
  if (l_res.result() != boost::beast::http::status::ok) {
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_res.result());
    co_return std::make_tuple(l_e, l_ret);
  }

  auto& l_json = l_res.body();
  try {
    l_ret = l_json["result"]["userid"].get<std::string>();
  } catch (const nlohmann::json::exception& e) {
    l_e = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "get_user_by_mobile error: {}", e.what());
    co_return std::make_tuple(l_e, l_ret);
  }
  co_return std::make_tuple(l_e, l_ret);
}

boost::asio::awaitable<std::tuple<boost::system::error_code, std::vector<client::attendance_update>>>
client::get_attendance_updatedata(
    const std::string& in_userid, const chrono::local_time_pos& in_work_date

) {
  if (access_token_.empty()) co_await begin_refresh_token();

  std::vector<attendance_update> l_ret{};
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, fmt::format("/topapi/attendance/getupdatedata?access_token={}", access_token_), 11
  };
  req.body() = nlohmann::json{{"userid", in_userid}, {"work_date", fmt::format("{:%Y-%m-%d}", in_work_date)}}.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::host, http_client_core_ptr_old_->server_ip_);

  auto [l_e, l_res] = co_await http::detail::read_and_write<http::basic_json_body>(
      http_client_core_ptr_old_, header_operator_req(std::move(req))
  );
  if (l_e) {
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_e);
    co_return std::make_tuple(l_e, l_ret);
  }
  if (l_res.result() != boost::beast::http::status::ok) {
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_res.result());
    co_return std::make_tuple(l_e, l_ret);
  }

  auto& l_json = l_res.body();

  try {
    l_ret = l_json["result"]["approve_list"].get<std::vector<attendance_update>>();
  } catch (const nlohmann::json::exception& e) {
    l_e = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "get_attendance_updatedata error: {}", e.what());
    co_return std::make_tuple(l_e, l_ret);
  }
  co_return std::make_tuple(l_e, l_ret);
}

std::shared_ptr<dingding_company::client_guard> dingding_company::get_client(const boost::uuids::uuid& in_id) {
  client_ptr l_ptr{};
  if (company_client_map_[in_id].empty()) {
    l_ptr = std::make_shared<client>(ssl_context_);
    l_ptr->access_token(company_info_map_[in_id].app_key, company_info_map_[in_id].app_secret);
  } else {
    l_ptr = std::move(company_client_map_[in_id].front());
    company_client_map_[in_id].pop_front();
  }

  return std::make_shared<client_guard>(l_ptr, this);
}

}  // namespace doodle::dingding