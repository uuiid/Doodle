#include "dingding_attendance.h"

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
namespace doodle::http {

class dingding_attendance_impl : public std::enable_shared_from_this<dingding_attendance_impl> {
  http_session_data_ptr handle_;

 public:
  explicit dingding_attendance_impl(http_session_data_ptr in_handle) : handle_(std::move(in_handle)) {}
  ~dingding_attendance_impl() = default;
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