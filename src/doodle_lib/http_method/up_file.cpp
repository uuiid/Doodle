//
// Created by TD on 25-1-11.
//

#include "up_file.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "boost/beast/http/field.hpp"

#include <cpp-base64/base64.h>
namespace doodle::http {
namespace {

/// 从json中生成路径
FSys::path gen_path_from_json_ma(const nlohmann::json& in_json) {
  if (in_json["task_type"]["name"].get_ref<const std::string&>() != "角色")
    throw_exception(doodle_error{"未知的 task_type 类型"});

  auto l_data = in_json["entity"]["data"];

  std::int32_t l_gui_dang{};
  if (l_data["gui_dang"].is_number())
    l_gui_dang = l_data["gui_dang"].get<std::int32_t>();
  else if (l_data["gui_dang"].is_string() && !l_data["gui_dang"].get<std::string>().empty())
    l_gui_dang = std::stoi(l_data["gui_dang"].get<std::string>());
  std::int32_t l_kai_shi_ji_shu{};
  if (l_data["kai_shi_ji_shu"].is_number())
    l_kai_shi_ji_shu = l_data["kai_shi_ji_shu"].get<std::int32_t>();
  else if (l_data["kai_shi_ji_shu"].is_string() && !l_data["kai_shi_ji_shu"].get<std::string>().empty())
    l_kai_shi_ji_shu = std::stoi(l_data["kai_shi_ji_shu"].get<std::string>());

  auto l_str = fmt::format(
      "6-moxing/Ch/JD{:02d}_{:02d}/Ch{}", l_gui_dang, l_kai_shi_ji_shu, l_data["bian_hao"].get_ref<const std::string&>()
  );
  return l_str;
}

boost::asio::awaitable<boost::beast::http::message_generator> up_file_asset(
    std::shared_ptr<std::function<std::string(const nlohmann::json&)>> in_path, session_data_ptr in_handle
) {
  uuid l_task_id = from_uuid_str(in_handle->capture_->get("task_id"));
  if (in_handle->content_type_ != detail::content_type::application_nuknown)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  if (in_handle->req_header_.count(boost::beast::http::field::content_disposition) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "缺失必要的请求头信息");

  std::string l_name{in_handle->req_header_[boost::beast::http::field::content_disposition]};
  FSys::path l_d{l_name};
  try {
    static const std::regex g_regex_base64{R"(^([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)?$)"};
    if (std::regex_match(l_name, g_regex_base64))
      l_d = base64_decode(std::string_view{in_handle->req_header_[boost::beast::http::field::content_disposition]});
  } catch (...) {
    default_logger_raw()->error("base64 decode error {}", boost::current_exception_diagnostic_information());
  }

  nlohmann::json l_json;
  if (auto l_task = g_ctx().get<cache_manger>().get(l_task_id); l_task) {
    l_json = std::move(*l_task);

  } else {
    boost::beast::http::request<boost::beast::http::empty_body> l_req{in_handle->req_header_};
    l_req.target(fmt::format("/api/data/tasks/{}/full", l_task_id));
    l_req.method(boost::beast::http::verb::get);
    l_req.erase(boost::beast::http::field::content_disposition);
    l_req.erase(boost::beast::http::field::content_type);
    l_req.erase(boost::beast::http::field::content_length);
    l_req.prepare_payload();

    auto [l_ec, l_res] =
        co_await detail::read_and_write<boost::beast::http::string_body>(kitsu::create_kitsu_proxy(in_handle), l_req);
    if (l_ec) co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");

    l_json = nlohmann::json::parse(l_res.body());
    g_ctx().get<cache_manger>().set(l_task_id, l_json);
  }

  auto l_prj =
      g_ctx().get<sqlite_database>().get_by_uuid<project_helper::database_t>(l_json["project"]["id"].get<uuid>());
  l_d             = l_prj.path_ / gen_path_from_json_ma(l_json) / (*in_path)(l_json) / l_d;

  auto l_tmp_path = std::get<FSys::path>(in_handle->body_);
  if (!exists(l_d.parent_path())) create_directories(l_d.parent_path());
  FSys::rename(l_tmp_path, l_d);
  co_return in_handle->make_msg("{}");
}

}  // namespace
void up_file_reg(http_route& in_route) {
  in_route
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/data/asset/{task_id}/file/maya",
              std::bind_front(
                  up_file_asset, std::make_shared<std::function<std::string(const nlohmann::json&)>>(
                                     [](const nlohmann::json&) -> std::string { return {"Mod"}; }
                                 )
              )
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/data/asset/{task_id}/file/ue",
              std::bind_front(
                  up_file_asset,
                  std::make_shared<std::function<std::string(const nlohmann::json&)>>(
                      [](const nlohmann::json& in_json) -> std::string {
                        auto l_data = in_json["entity"]["data"];
                        return fmt::format("{}_UE5", l_data["pin_yin_ming_cheng"].get_ref<const std::string&>());
                      }
                  )
              )

          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/data/asset/{task_id}/file/image",
              std::bind_front(
                  up_file_asset, std::make_shared<std::function<std::string(const nlohmann::json&)>>(
                                     [](const nlohmann::json&) -> std::string { return ""; }
                                 )
              )

          )
      )

      ;
}
}  // namespace doodle::http