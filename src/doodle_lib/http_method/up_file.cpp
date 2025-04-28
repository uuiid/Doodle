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

boost::asio::awaitable<boost::beast::http::message_generator> http::up_file_asset::callback(
    session_data_ptr in_handle
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
  check_data(l_json);
  task_data_ = l_json;

  auto l_prj =
      g_ctx().get<sqlite_database>().get_by_uuid<project_helper::database_t>(l_json["project"]["id"].get<uuid>());
  l_d             = l_prj.path_ / gen_file_path() / l_d;

  auto l_tmp_path = std::get<FSys::path>(in_handle->body_);
  if (!exists(l_d.parent_path())) create_directories(l_d.parent_path());
  FSys::rename(l_tmp_path, l_d);
  co_return in_handle->make_msg("{}");
}

void up_file_asset::check_data(const nlohmann::json& in_data) {
  if (auto l_type = in_data["task_type"]["name"].get_ref<const std::string&>();
      !(l_type == "角色" || l_type == "地编模型" || l_type == "绑定"))
    throw_exception(doodle_error{"未知的 task_type 类型"});

  entity_type_       = in_data["entity_type"]["name"].get_ref<const std::string&>();

  const auto& l_data = in_data["entity"]["data"];
  std::int32_t l_gui_dang{};
  if (l_data["gui_dang"].is_number())
    gui_dang_ = l_data["gui_dang"].get<std::int32_t>();
  else if (l_data["gui_dang"].is_string() && !l_data["gui_dang"].get<std::string>().empty())
    gui_dang_ = std::stoi(l_data["gui_dang"].get<std::string>());
  std::int32_t l_kai_shi_ji_shu{};
  if (l_data["kai_shi_ji_shu"].is_number())
    kai_shi_ji_shu_ = l_data["kai_shi_ji_shu"].get<std::int32_t>();
  else if (l_data["kai_shi_ji_shu"].is_string() && !l_data["kai_shi_ji_shu"].get<std::string>().empty())
    kai_shi_ji_shu_ = std::stoi(l_data["kai_shi_ji_shu"].get<std::string>());

  if (l_data.contains("bian_hao")) bian_hao_ = l_data["bian_hao"].get_ref<const std::string&>();
  if (l_data.contains("pin_yin_ming_cheng"))
    pin_yin_ming_cheng_ = l_data["pin_yin_ming_cheng"].get_ref<const std::string&>();

  if (l_data.contains("ban_ben")) version_ = l_data["ban_ben"].get_ref<const std::string&>();
  if (!version_.empty()) version_.insert(version_.begin(), '_');
}

FSys::path up_file_asset_maya_post::gen_file_path() {
  if (entity_type_ == "角色")
    return fmt::format("6-moxing/Ch/JD{:02d}_{:02d}/Ch{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
  if (entity_type_ == "道具")
    return fmt::format("6-moxing/Ch/JD{:02d}_{:02d}/{}", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
  if (entity_type_ == "地编")
    return fmt::format("6-moxing/Ch/JD{:02d}_{:02d}/BG{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);

  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}

FSys::path up_file_asset_ue_post::gen_file_path() {
  if (entity_type_ == "角色")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/Ch{}/{}_UE5", gui_dang_, kai_shi_ji_shu_, bian_hao_, pin_yin_ming_cheng_
    );
  if (entity_type_ == "道具")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/JD{:02d}_{:02d}_UE", gui_dang_, kai_shi_ji_shu_, gui_dang_, kai_shi_ji_shu_
    );
  if (entity_type_ == "地编")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/BG{}/{}", gui_dang_, kai_shi_ji_shu_, bian_hao_, pin_yin_ming_cheng_
    );
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}

FSys::path up_file_asset_image_post::gen_file_path() {
  if (entity_type_ == "角色")
    return fmt::format("6-moxing/Ch/JD{:02d}_{:02d}/Ch{}", gui_dang_, kai_shi_ji_shu_, bian_hao_);
  if (entity_type_ == "道具")
    return fmt::format("6-moxing/Ch/JD{:02d}_{:02d}/{}", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
  if (entity_type_ == "地编")
    return fmt::format("6-moxing/Ch/JD{:02d}_{:02d}/BG{}", gui_dang_, kai_shi_ji_shu_, bian_hao_);
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}

}  // namespace doodle::http