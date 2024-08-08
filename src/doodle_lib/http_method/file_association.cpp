#include "file_association.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> file_association_get(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  auto l_id_str = in_handle->capture_->get("uuid");
  boost::uuids::uuid l_id;
  try {
    l_id = boost::lexical_cast<boost::uuids::uuid>(l_id_str);
  } catch (const std::exception& e) {
    l_logger->log(log_loc(), level::err, "error: {}", e.what());
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, e.what());
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::current_exception_diagnostic_information()
    );
  }

  auto& l_map = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data();
  if (l_map.contains(l_id)) {
    auto l_data = l_map.at(l_id);
    nlohmann::json l_json;
    l_json["maya_file"]   = l_data->rig_file_.path_;
    l_json["ue_file"]     = l_data->ue_file_.path_;
    l_json["solve_file_"] = l_data->solve_file_.path_;
    l_json["type"]        = l_data->assets_type_;
    co_return in_handle->make_msg(l_json.dump());
  }
  l_logger->log(log_loc(), level::info, "file not found");
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "file not found");
}

void reg_file_association_http(http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(
      boost::beast::http::verb::get, "api/doodle/file_association/{uuid}", file_association_get
  ));
}
}  // namespace doodle::http