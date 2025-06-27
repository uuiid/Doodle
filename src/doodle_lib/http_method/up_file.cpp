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

boost::asio::awaitable<boost::beast::http::message_generator> up_file_asset_base::callback(session_data_ptr in_handle) {
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
  auto l_ptr = check_data(l_json);
  l_ptr->root_path_ =
      g_ctx().get<sqlite_database>().get_by_uuid<project>(l_json["project"]["id"].get<uuid>()).path_;
  l_ptr->file_path_ = l_d;
  move_file(in_handle, l_ptr);
  co_return in_handle->make_msg(nlohmann::json{});
}

std::shared_ptr<up_file_asset::task_info_t> up_file_asset_base::check_data(const nlohmann::json& in_data) {
  if (auto l_type = in_data["task_type"]["name"].get_ref<const std::string&>();
      !(l_type == "角色" || l_type == "地编模型" || l_type == "绑定"))
    throw_exception(doodle_error{"未知的 task_type 类型"});
  auto l_task_info          = std::make_shared<task_info_t>();

  l_task_info->entity_type_ = in_data["entity_type"]["name"].get_ref<const std::string&>();

  const auto& l_data        = in_data["entity"]["data"];
  std::int32_t l_gui_dang{};
  if (l_data["gui_dang"].is_number())
    l_task_info->gui_dang_ = l_data["gui_dang"].get<std::int32_t>();
  else if (l_data["gui_dang"].is_string() && !l_data["gui_dang"].get<std::string>().empty())
    l_task_info->gui_dang_ = std::stoi(l_data["gui_dang"].get<std::string>());
  std::int32_t l_kai_shi_ji_shu{};
  if (l_data["kai_shi_ji_shu"].is_number())
    l_task_info->kai_shi_ji_shu_ = l_data["kai_shi_ji_shu"].get<std::int32_t>();
  else if (l_data["kai_shi_ji_shu"].is_string() && !l_data["kai_shi_ji_shu"].get<std::string>().empty())
    l_task_info->kai_shi_ji_shu_ = std::stoi(l_data["kai_shi_ji_shu"].get<std::string>());

  if (l_data.contains("bian_hao")) l_task_info->bian_hao_ = l_data["bian_hao"].get_ref<const std::string&>();
  if (l_data.contains("pin_yin_ming_cheng"))
    l_task_info->pin_yin_ming_cheng_ = l_data["pin_yin_ming_cheng"].get_ref<const std::string&>();

  if (l_data.contains("ban_ben")) l_task_info->version_ = l_data["ban_ben"].get_ref<const std::string&>();
  if (!l_task_info->version_.empty()) l_task_info->version_.insert(l_task_info->version_.begin(), '_');
  return l_task_info;
}
void up_file_asset::move_file(session_data_ptr in_handle, const std::shared_ptr<task_info_t>& in_data) {
  auto l_d        = in_data->root_path_ / gen_file_path(in_data) / in_data->file_path_;
  auto l_tmp_path = std::get<FSys::path>(in_handle->body_);
  if (!exists(l_d.parent_path())) create_directories(l_d.parent_path());
  if (exists(l_d)) FSys::backup_file(l_d);
  FSys::rename(l_tmp_path, l_d);
}

FSys::path up_file_asset_maya_post::gen_file_path(const std::shared_ptr<task_info_t>& in_data) {
  if (in_data->entity_type_ == "角色")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/Ch{}/Mod", in_data->gui_dang_, in_data->kai_shi_ji_shu_, in_data->bian_hao_
    );
  if (in_data->entity_type_ == "道具")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/{}", in_data->gui_dang_, in_data->kai_shi_ji_shu_, in_data->pin_yin_ming_cheng_
    );
  if (in_data->entity_type_ == "场景")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/BG{}/Mod", in_data->gui_dang_, in_data->kai_shi_ji_shu_, in_data->bian_hao_
    );

  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}

FSys::path up_file_asset_ue_post::gen_file_path(const std::shared_ptr<task_info_t>& in_data) {
  if (in_data->entity_type_ == "角色")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/Ch{}/{}_UE5", in_data->gui_dang_, in_data->kai_shi_ji_shu_, in_data->bian_hao_,
        in_data->pin_yin_ming_cheng_
    );
  if (in_data->entity_type_ == "道具")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/JD{:02d}_{:02d}_UE", in_data->gui_dang_, in_data->kai_shi_ji_shu_,
        in_data->gui_dang_, in_data->kai_shi_ji_shu_
    );
  if (in_data->entity_type_ == "场景")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/BG{}/{}", in_data->gui_dang_, in_data->kai_shi_ji_shu_, in_data->bian_hao_,
        in_data->pin_yin_ming_cheng_
    );
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}
void up_file_asset_ue_post::move_file(session_data_ptr in_handle, const std::shared_ptr<task_info_t>& in_data) {
  auto l_UE_folder = in_data->root_path_ / gen_file_path(in_data);
  auto l_d         = l_UE_folder / in_data->file_path_;
  auto l_tmp_path  = std::get<FSys::path>(in_handle->body_);
  if (!exists(l_d.parent_path())) create_directories(l_d.parent_path());
  if (exists(l_d)) {
    auto l_backup_path = l_UE_folder / "backup" / FSys::add_time_stamp(in_data->file_path_.filename());
    if (!exists(l_backup_path.parent_path())) create_directories(l_backup_path.parent_path());
    FSys::rename(l_d, l_backup_path);
  }
  FSys::rename(l_tmp_path, l_d);
}

FSys::path up_file_asset_image_post::gen_file_path(const std::shared_ptr<task_info_t>& in_data) {
  if (in_data->entity_type_ == "角色")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/Ch{}", in_data->gui_dang_, in_data->kai_shi_ji_shu_, in_data->bian_hao_
    );
  if (in_data->entity_type_ == "道具")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/{}", in_data->gui_dang_, in_data->kai_shi_ji_shu_, in_data->pin_yin_ming_cheng_
    );
  if (in_data->entity_type_ == "场景")
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/BG{}", in_data->gui_dang_, in_data->kai_shi_ji_shu_, in_data->bian_hao_
    );
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}

}  // namespace doodle::http