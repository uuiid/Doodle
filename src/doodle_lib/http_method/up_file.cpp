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
boost::asio::awaitable<boost::beast::http::message_generator> up_file_asset_base::post(session_data_ptr in_handle) {
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
  auto l_task   = l_sql.get_by_uuid<task>(id_);
  auto l_extend = l_sql.get_entity_asset_extend(l_task.entity_id_);
  if (!l_extend) throw_exception(http_request_error{boost::beast::http::status::bad_request, "请求task没有附加元数据"});

  task_info_.gui_dang_           = l_extend->gui_dang_.value_or(0);
  task_info_.kai_shi_ji_shu_     = l_extend->kai_shi_ji_shu_.value_or(0);
  task_info_.bian_hao_           = l_extend->bian_hao_;
  task_info_.pin_yin_ming_cheng_ = l_extend->pin_yin_ming_cheng_;
  task_info_.version_            = l_extend->ban_ben_;

  auto l_entity                  = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_prj                     = l_sql.get_by_uuid<project>(l_entity.project_id_);
  if (!(l_task.task_type_id_ == task_type::get_character_id() ||
        l_task.task_type_id_ == task_type::get_ground_model_id() ||
        l_task.task_type_id_ == task_type::get_binding_id()))
    throw_exception(doodle_error{"未知的 task_type 类型"});

  task_info_.task_type_id_   = l_task.task_type_id_;
  task_info_.entity_type_id_ = l_entity.entity_type_id_;

  task_info_.root_path_      = l_prj.path_;
  task_info_.file_path_      = l_d;
  move_file(in_handle);
  co_return in_handle->make_msg(nlohmann::json{});
}

void up_file_asset_base::move_file(session_data_ptr in_handle) {
  auto l_d        = task_info_.root_path_ / gen_file_path() / task_info_.file_path_;
  auto l_tmp_path = std::get<FSys::path>(in_handle->body_);
  if (!exists(l_d.parent_path())) create_directories(l_d.parent_path());
  if (exists(l_d)) FSys::backup_file(l_d);
  FSys::rename(l_tmp_path, l_d);
}

//         | 角色 | 地编模型 | 绑定
// 角色    |
// 场景    |
// 道具    |
// 地编资产 |
// 特效    |
// 其他    |

FSys::path doodle_data_asset_file_maya::gen_file_path() {
  if (task_info_.entity_type_id_ == asset_type::get_character_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/Ch{}/Mod", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_, task_info_.bian_hao_
    );
  if (task_info_.entity_type_id_ == asset_type::get_prop_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/{}", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_,
        task_info_.pin_yin_ming_cheng_
    );
  if (task_info_.entity_type_id_ == asset_type::get_ground_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/BG{}/Mod", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_, task_info_.bian_hao_
    );

  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}

FSys::path doodle_data_asset_file_ue::gen_file_path() {
  if (task_info_.entity_type_id_ == asset_type::get_character_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/Ch{}/{}_UE5", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_,
        task_info_.bian_hao_, task_info_.pin_yin_ming_cheng_
    );
  if (task_info_.entity_type_id_ == asset_type::get_prop_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/JD{:02d}_{:02d}_UE", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_,
        task_info_.gui_dang_, task_info_.kai_shi_ji_shu_
    );
  if (task_info_.entity_type_id_ == asset_type::get_ground_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/BG{}/{}", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_, task_info_.bian_hao_,
        task_info_.pin_yin_ming_cheng_
    );
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}
void doodle_data_asset_file_ue::move_file(session_data_ptr in_handle) {
  auto l_UE_folder = task_info_.root_path_ / gen_file_path();
  auto l_d         = l_UE_folder / task_info_.file_path_;
  auto l_tmp_path  = std::get<FSys::path>(in_handle->body_);
  if (!exists(l_d.parent_path())) create_directories(l_d.parent_path());
  if (exists(l_d)) {
    auto l_backup_path = l_UE_folder / "backup" / FSys::add_time_stamp(task_info_.file_path_.filename());
    if (!exists(l_backup_path.parent_path())) create_directories(l_backup_path.parent_path());
    FSys::rename(l_d, l_backup_path);
  }
  FSys::rename(l_tmp_path, l_d);
}

FSys::path doodle_data_asset_file_image::gen_file_path() {
  if (task_info_.entity_type_id_ == asset_type::get_character_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/Ch{}", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_, task_info_.bian_hao_
    );
  if (task_info_.entity_type_id_ == asset_type::get_prop_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/{}", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_,
        task_info_.pin_yin_ming_cheng_
    );
  if (task_info_.entity_type_id_ == asset_type::get_ground_id())
    return fmt::format(
        "6-moxing/Ch/JD{:02d}_{:02d}/BG{}", task_info_.gui_dang_, task_info_.kai_shi_ji_shu_, task_info_.bian_hao_
    );
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "未知的 entity_type 类型"});
}

}  // namespace doodle::http