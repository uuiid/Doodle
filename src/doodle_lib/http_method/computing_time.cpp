#include "computing_time.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/platform/win/register_file_type.h"

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
namespace doodle::http {

class computing_time_post {
 public:
  computing_time_post()  = default;
  ~computing_time_post() = default;
  using executor_type    = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }
    auto l_req = in_handle->get_msg_body_parser<boost::beast::http::string_body>();
    auto l_str = l_req->request_parser_->get().body();
    if (!nlohmann::json::accept(l_str)) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", l_str);
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::system_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "错误的json格式");
      return;
    }
    auto l_json = nlohmann::json::parse(l_str);
    if (!l_json.contains("user_id") || !l_json.contains("time")) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", l_str);
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::system_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "缺失必须键 time 或 user_id");
      return;
    }
    auto& l_time = l_json["time"];
    if (!l_time.contains("start_time") || !l_time.contains("end_time")) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", l_str);
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::system_category()};
      in_handle->seed_error(
          boost::beast::http::status::bad_request, in_error_code, "缺失必须键 time.start_time 或 time.end_time"
      );
      return;
    }
    if (!l_json.contains("data")) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", l_str);
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::system_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "缺失必须键 data");
      return;
    }

    auto l_user_id = l_json["user_id"].get<std::string>();
    std::istringstream l_uuid_stream(l_user_id);
    boost::uuids::uuid l_uuid;
    l_uuid_stream >> l_uuid;
    if (!l_uuid_stream) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", l_str);
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::system_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "user_id 格式错误不是uuid格式");
      return;
    }
    auto l_start_time_str = l_time["start_time"].get<std::string>();
    auto l_end_time_str   = l_time["end_time"].get<std::string>();

    auto l_start_time     = parse_8601<std::chrono::system_clock::time_point>(l_end_time_str, in_error_code);
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "time parse error: {}", l_str);
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "end_time 格式错误不是时间格式");
      return;
    }
    auto l_end_time = parse_8601<std::chrono::system_clock::time_point>(l_start_time_str, in_error_code);

    if (!in_error_code) {
      l_logger->log(log_loc(), level::err, "time parse error: {}", l_str);
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "start_time 格式错误不是时间格式");
      return;
    }
    
    for (auto l_task : l_json["data"]) {
      


    }
  }
};
void reg_computing_time(http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(
      boost::beast::http::verb::post, "api/doodle/computing_time",
      session::make_http_reg_fun<boost::beast::http::string_body>(computing_time_post{})
  ));
}
}  // namespace doodle::http