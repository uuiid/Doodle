#include "file_association.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/http_websocket_data.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>

namespace doodle::http {
class file_association_get {
 public:
  file_association_get() : executor_(g_thread().get_executor()) {}
  ~file_association_get() = default;
  using executor_type     = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }
    auto& l_req   = in_handle->request_parser_->get();

    auto l_id_str = in_handle->capture_->get("uuid");
    boost::uuids::uuid l_id;
    try {
      l_id = boost::lexical_cast<boost::uuids::uuid>(l_id_str);
    } catch (const std::exception& e) {
      l_logger->log(log_loc(), level::err, "error: {}", e.what());
      in_error_code =
          boost::system::error_code{boost::system::errc::invalid_argument, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "错误的 uuid 格式");
      return;
    } catch (...) {
      l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(
          boost::beast::http::status::bad_request, in_error_code, boost::current_exception_diagnostic_information()
      );
      return;
    }

    auto l_map = g_ctx().get<scan_win_service_t>().get_scan_data();
    if (l_map.contains(l_id)) {
      auto l_data = l_map.at(l_id);

      nlohmann::json l_json;
      l_json["maya_file"]   = l_data->rig_file_.path_;
      l_json["ue_file"]     = l_data->ue_file_.path_;
      l_json["solve_file_"] = l_data->solve_file_.path_;

      boost::beast::http::response<boost::beast::http::string_body> l_res{
          boost::beast::http::status::ok, l_req.version()
      };
      l_res.set(boost::beast::http::field::content_type, "application/json");
      l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
      l_res.body() = l_json.dump();
      l_res.prepare_payload();

      in_handle->seed(std::move(l_res));
    } else {
      in_error_code =
          boost::system::error_code{boost::system::errc::no_such_file_or_directory, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::not_found, in_error_code, "未找到文件关联");
    }
  }
};

void reg_file_association_http(http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(
      boost::beast::http::verb::get, "api/doodle/file_association/{uuid}",
      session::make_http_reg_fun(file_association_get{})
  ));
}
}  // namespace doodle::http