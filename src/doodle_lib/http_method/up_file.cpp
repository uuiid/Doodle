//
// Created by TD on 25-1-11.
//

#include "up_file.h"

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "boost/beast/http/field.hpp"
namespace doodle::http {
namespace {

/// 从json中生成路径
FSys::path gen_path_from_json(const nlohmann::json& in_json) {}

boost::asio::awaitable<boost::beast::http::message_generator> up_file_asset(session_data_ptr in_handle) {
  uuid l_task_id = from_uuid_str(in_handle->capture_->get("task_id"));
  if (in_handle->content_type_ != detail::content_type::application_nuknown)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  if (in_handle->req_header_.count(boost::beast::http::field::content_disposition) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "缺失必要的请求头信息");

  FSys::path l_d{std::string{in_handle->req_header_[boost::beast::http::field::content_disposition]}};
  if (auto l_task = g_ctx().get<cache_manger>().get(l_task_id); l_task) {
    l_d = gen_path_from_json(*l_task) / l_d;
  } else {
    boost::beast::http::request<boost::beast::http::empty_body> l_req{in_handle->req_header_};
    l_req.erase(boost::beast::http::field::content_disposition);
    l_req.erase(boost::beast::http::field::content_type);
    l_req.prepare_payload();

    auto [l_ec, l_res] =
        co_await detail::read_and_write<boost::beast::http::empty_body>(kitsu::create_kitsu_proxy(in_handle), l_req);
    if (l_ec) co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");

    auto l_json = nlohmann::json::parse(l_res.body());
    l_d         = gen_path_from_json(l_json) / l_d;
    g_ctx().get<cache_manger>().set(l_task_id, l_json);
  }
  auto l_tmp_path = std::get<FSys::path>(in_handle->body_);
  if (!exists(l_d.parent_path())) create_directories(l_d.parent_path());
  FSys::rename(l_tmp_path, l_d);
  co_return in_handle->make_msg("{}");
}
}  // namespace
void up_file_reg(http_route& in_route) {
  in_route.reg(
      std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/doodle/data/asset/{task_id}/file/maya", up_file_asset
      )
  );
}
}  // namespace doodle::http