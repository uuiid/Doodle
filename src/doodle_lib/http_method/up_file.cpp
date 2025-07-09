//
// Created by TD on 25-1-11.
//

#include "up_file.h"

#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/metadata/task_type.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "boost/beast/http/field.hpp"

#include <cpp-base64/base64.h>
namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> up_file_asset_base::callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) {
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
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_task   = l_sql.get_by_uuid<task>(in_arg->id_);
  auto l_extend = l_sql.get_entity_asset_extend(l_task.entity_id_);
  if (!l_extend) throw_exception(http_request_error{boost::beast::http::status::bad_request, "请求task没有附加元数据"});

  auto l_ptr         = check_data(*l_extend);

  auto l_entity      = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_entity_type = l_sql.get_by_uuid<asset_type>(l_entity.entity_type_id_);
  auto l_task_type   = l_sql.get_by_uuid<task_type>(l_task.task_type_id_);
  auto l_prj         = l_sql.get_by_uuid<project>(l_entity.project_id_);
  if (auto l_type = l_task_type.name_; !(l_type == "角色" || l_type == "地编模型" || l_type == "绑定"))
    throw_exception(doodle_error{"未知的 task_type 类型"});
  l_ptr->entity_type_ = l_entity_type.name_;
  l_ptr->root_path_   = l_prj.path_;
  l_ptr->file_path_   = l_d;
  move_file(in_handle, l_ptr);
  co_return in_handle->make_msg(nlohmann::json{});
}

std::shared_ptr<up_file_asset_base::task_info_t> up_file_asset_base::check_data(
    const entity_asset_extend& in_entity_asset_extend
) {
  auto l_task_info                 = std::make_shared<task_info_t>();
  l_task_info->gui_dang_           = in_entity_asset_extend.gui_dang_.value_or(0);
  l_task_info->kai_shi_ji_shu_     = in_entity_asset_extend.kai_shi_ji_shu_.value_or(0);
  l_task_info->bian_hao_           = in_entity_asset_extend.bian_hao_;
  l_task_info->pin_yin_ming_cheng_ = in_entity_asset_extend.pin_yin_ming_cheng_;
  l_task_info->version_            = in_entity_asset_extend.ban_ben_;
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