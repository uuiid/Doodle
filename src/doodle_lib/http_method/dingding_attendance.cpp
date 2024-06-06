#include "dingding_attendance.h"

#include <doodle_core/metadata/user.h>

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>

namespace doodle::http {

class dingding_attendance_impl : public std::enable_shared_from_this<dingding_attendance_impl> {
  http_session_data_ptr handle_;
  user user_;
  entt::entity user_entity_{entt::null};
  chrono::year_month_day date_;
  void find_user(const boost::uuids::uuid& in_user_id) {
    auto l_logger = handle_->logger_;
    auto l_user   = std::as_const(*g_reg()).view<const user>();
    for (auto&& [e, l_u] : l_user.each()) {
      if (l_u.id_ == in_user_id) {
        user_        = l_u;
        user_entity_ = e;
        break;
      }
    }
    if (user_.id_ == boost::uuids::nil_uuid() || user_.mobile_.empty()) {
      user_.id_           = in_user_id;
      auto l_kitsu_client = g_ctx().get<kitsu::kitsu_client_ptr>();
      l_kitsu_client->get_user(
          user_.id_,
          boost::asio::bind_executor(
              g_io_context(),
              boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_mobile, shared_from_this())
          )
      );
    } else {
      boost::asio::post(boost::asio::bind_executor(
          g_io_context(),
          boost::beast::bind_front_handler(&dingding_attendance_impl::feach_dingding, shared_from_this())
      ));
    }
  }
  void do_feach_mobile(boost::system::error_code ec, nlohmann::json l_json) {
    auto l_logger = handle_->logger_;
    if (ec) {
      l_logger->log(log_loc(), level::err, "get user failed: {}", ec.message());
      handle_->seed_error(boost::beast::http::status::internal_server_error, ec);
      return;
    }
    try {
      user_.mobile_ = l_json["phone"].get<std::string>();
    } catch (const nlohmann::json::exception& e) {
      l_logger->log(
          log_loc(), level::err, "user {} json parse error: {}", l_json["email"].get<std::string>(), e.what()
      );
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} {}", l_json["email"].get<std::string>(), e.what())
      );
      return;
    } catch (const std::exception& e) {
      l_logger->log(
          log_loc(), level::err, "user {} json parse error: {}", l_json["email"].get<std::string>(), e.what()
      );
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} {}", l_json["email"].get<std::string>(), e.what())
      );
      return;
    }
    if (user_.mobile_.empty()) {
      l_logger->log(log_loc(), level::err, "user {} mobile is empty", l_json["email"].get<std::string>());
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} mobile is empty", l_json["email"].get<std::string>())
      );
      return;
    }
    entt::handle l_handle{};
    // 创建用户
    if (user_entity_ == entt::null)
      l_handle = {*g_reg(), g_reg()->create()};
    else  // 存在用户则修改
      l_handle = {*g_reg(), user_entity_};
    l_handle.emplace_or_replace<user>(user_);
    user_entity_ = l_handle.entity();
    boost::asio::post(boost::asio::bind_executor(
        g_io_context(), boost::beast::bind_front_handler(&dingding_attendance_impl::feach_dingding, shared_from_this())
    ));
  }

  void feach_dingding() {
    auto l_kitsu_client = g_ctx().get<dingding::client_ptr>();
    if (user_.dingding_id_.empty()) {
      l_kitsu_client->get_user_by_mobile(
          user_.mobile_,
          boost::asio::bind_executor(
              g_io_context(),
              boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_dingding, shared_from_this())
          )
      );
    } else {
      feach_attendance();
    }
  }
  void do_feach_dingding(boost::system::error_code in_err, nlohmann::json in_json) {
    if (in_err) {
      handle_->logger_->log(log_loc(), level::err, "get user by mobile failed: {}", in_err.message());
      handle_->seed_error(
          boost::beast::http::status::internal_server_error, in_err, "无法从手机号码中获取钉钉用户信息"
      );
      return;
    }
    if (in_json.contains("result") && in_json["result"].contains("userid")) {
      user_.dingding_id_ = in_json["result"]["userid"].get<std::string>();
    } else {
      handle_->logger_->log(log_loc(), level::err, "get user by mobile failed: {}", in_json.dump());
      handle_->seed_error(boost::beast::http::status::internal_server_error, in_err, "返回用户信息错误");
      return;
    }
  }

  void feach_attendance() {
    auto l_kitsu_client = g_ctx().get<dingding::client_ptr>();
    l_kitsu_client->get_attendance_updatedata(
        user_.dingding_id_, chrono::local_days{date_},
        boost::asio::bind_executor(
            g_io_context(),
            boost::beast::bind_front_handler(&dingding_attendance_impl::do_feach_attendance, shared_from_this())
        )
    );
  }
  void do_feach_attendance(boost::system::error_code in_err, nlohmann::json in_json) {
    if (in_err) {
      handle_->logger_->log(log_loc(), level::err, "get attendance failed: {}", in_err.message());
      handle_->seed_error(boost::beast::http::status::internal_server_error, in_err, "获取考勤信息失败");
      return;
    }
    
  }

 public:
  explicit dingding_attendance_impl(http_session_data_ptr in_handle) : handle_(std::move(in_handle)) {}
  ~dingding_attendance_impl() = default;

  void run_post(const boost::uuids::uuid& in_user_id, const chrono::year_month_day& in_date) {
    find_user(in_user_id);
    date_ = in_date;
  }
};

class dingding_attendance_get {
 public:
  dingding_attendance_get() : executor_(g_thread().get_executor()) {}
  ~dingding_attendance_get() = default;
  using executor_type        = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {}
};

class dingding_attendance_post {
 public:
  dingding_attendance_post() : executor_(g_thread().get_executor()) {}
  ~dingding_attendance_post() = default;
  using executor_type         = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }
    auto l_req                   = in_handle->get_msg_body_parser<boost::beast::http::string_body>();

    auto l_computing_time_id_str = in_handle->capture_->get("user_id");
    auto l_date_str              = in_handle->capture_->get("date");
    boost::uuids::uuid l_computing_time_id{};
    chrono::year_month_day l_date{};
    try {
      l_computing_time_id = boost::lexical_cast<boost::uuids::uuid>(l_computing_time_id_str);
      std::istringstream l_date_stream{l_date_str};
      l_date_stream >> date::parse("%Y-%m-%d", l_date);
    } catch (const std::exception& e) {
      l_logger->log(log_loc(), level::err, "url parse error: {}", e.what());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, e.what());
      return;
    }
    auto l_impl = std::make_shared<dingding_attendance_impl>(in_handle);
    l_impl->run_post(l_computing_time_id, l_date);
  }
};

void reg_dingding_attendance(http_route& in_route) {
  in_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/doodle/attendance/{user_id}/{date}",
          session::make_http_reg_fun<boost::beast::http::string_body>(dingding_attendance_post{})
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/attendance/{user_id}/{date}",
          session::make_http_reg_fun<boost::beast::http::string_body>(dingding_attendance_get{})
      ))

      ;
}
}  // namespace doodle::http