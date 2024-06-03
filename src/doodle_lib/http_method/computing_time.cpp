#include "computing_time.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/platform/win/register_file_type.h"

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
namespace doodle::http {

struct computing_time_data {
  struct task_data {
    boost::uuids::uuid task_id;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    // form json
    friend void from_json(const nlohmann::json& j, task_data& p) {
      p.task_id    = j.at("task_id").get<boost::uuids::uuid>();
      p.start_time = parse_8601<std::chrono::system_clock::time_point>(j.at("start_time").get<std::string>());
      p.end_time   = parse_8601<std::chrono::system_clock::time_point>(j.at("end_time").get<std::string>());
    }
  };
  chrono::year_month year_month_;
  boost::uuids::uuid user_id;
  std::vector<task_data> data;

  // form json
  friend void from_json(const nlohmann::json& j, computing_time_data& p) {
    std::istringstream l_year_month_stream(j.at("year_month").get<std::string>());
    l_year_month_stream >> chrono::parse("%Y-%m", p.year_month_);
    if (!l_year_month_stream) {
      throw nlohmann::json::parse_error::create(101, 0, "year_month 格式错误不是时间格式", j.at("year_month"));
    }
    p.user_id = j.at("user_id").get<boost::uuids::uuid>();
    p.data    = j.at("data").get<std::vector<task_data>>();
  }
};

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
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "不是json字符串");
      return;
    }
    auto l_json = nlohmann::json::parse(l_str);

    computing_time_data l_data{};
    try {
      l_data = l_json.get<computing_time_data>();
    } catch (const nlohmann::json::exception& e) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", e.what());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::system_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, e.what());
      return;
    } catch (const std::exception& e) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", e.what());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::system_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, e.what());
      return;
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