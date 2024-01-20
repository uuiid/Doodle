//
// Created by TD on 2024/1/5.
//

#include "down_auto_light_anim_file.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/core/thread_copy_io.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
namespace doodle {

void down_auto_light_anim_file::init() {
  data_->logger_ = msg_.get<process_message>().logger();

  if (!g_ctx().contains<thread_copy_io_service>()) g_ctx().emplace<thread_copy_io_service>();
}

void down_auto_light_anim_file::analysis_out_file(boost::system::error_code in_error_code) const {
  auto l_id_map_tmp = g_reg()->view<database, assets_file, file_association_ref>();
  auto l_id_map     = l_id_map_tmp | ranges::views::transform([](const entt::entity &in_entity) {
                    return std::make_pair(g_reg()->get<database>(in_entity).uuid(), in_entity);
                  }) |
                  ranges::to<std::unordered_map<uuid, entt::entity>>();

  auto l_refs =
      data_->out_maya_arg_.out_file_list |
      ranges::views::transform([](const maya_exe_ns::maya_out_arg::out_file_t &in_arg) { return in_arg.ref_file; }) |
      ranges::views::filter([](const FSys::path &in_arg) { return FSys::exists(in_arg); }) |
      ranges::views::transform([&](const FSys::path &in_arg) -> entt::handle {
        auto l_uuid = FSys::software_flag_file(in_arg);
        log_info(fmt::format("maya_to_exe_file get uuid :{}", l_uuid));

        if (l_id_map.contains(l_uuid)) {
          data_->logger_->log(log_loc(), level::level_enum::warn, "找到了文件{}的引用", in_arg);
          return entt::handle{*g_reg(), l_id_map.at(l_uuid)};
        }
        data_->logger_->log(log_loc(), level::level_enum::err, "文件 {} 找不到引用, 继续输出将变为不正确排屏", in_arg);
        return entt::handle{};
      }) |
      ranges::to<std::vector<entt::handle>>();
  l_refs |= ranges::actions::unique;

  if (!ranges::all_of(l_refs, [&](const entt::handle &in_handle) -> bool {
        if (!in_handle) {
          return false;
        }
        const auto &l_file = in_handle.get<assets_file>().path_attr();
        if (!in_handle.get<file_association_ref>()) {
          data_->logger_->log(log_loc(), level::level_enum::err, "未查找到文件 {} 的引用", l_file);
          return false;
        }
        if (!in_handle.get<file_association_ref>().get<file_association>().ue_file) {
          data_->logger_->log(log_loc(), level::level_enum::err, "未查找到文件 {} 的 ue 引用", l_file);
          return false;
        }
        if (!in_handle.get<file_association_ref>().get<file_association>().ue_file.all_of<assets_file>()) {
          data_->logger_->log(log_loc(), level::level_enum::err, "文件 {} 的 ue 引用无效", l_file);
          return false;
        }
        return true;
      })) {
    in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
    data_->logger_->log(log_loc(), level::level_enum::err, "maya结束进程后 输出文件引用查找有误");
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();

    return;
  }
  if (!ranges::any_of(l_refs, [&](const entt::handle &in_handle) {
        return in_handle.get<file_association_ref>().get<file_association>().ue_file.all_of<scene_id>();
      })) {
    data_->logger_->log(log_loc(), level::level_enum::err, "未查找到主项目文件, 既场景文件没有在库中提交");
    in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }

  // sort scene_id
  l_refs |= ranges::actions::sort([](const entt::handle &in_r, const entt::handle &in_l) {
    return in_r.get<file_association_ref>().get<file_association>().ue_file.all_of<scene_id>() &&
           !in_l.get<file_association_ref>().get<file_association>().ue_file.all_of<scene_id>();
  });

  static auto g_root{FSys::path{"D:/doodle/cache/ue"}};
  std::vector<std::pair<FSys::path, FSys::path>> l_copy_path{};
  FSys::path l_down_path_file_name{};
  for (auto &&h : l_refs) {
    auto l_is_se     = h.get<file_association_ref>().get<file_association>().ue_file.all_of<scene_id>();
    auto l_uproject  = h.get<file_association_ref>().get<file_association>().ue_file.get<ue_main_map>().project_path_;
    auto l_down_path = l_uproject.parent_path();
    if (l_is_se) {
      l_down_path_file_name = l_down_path.filename();
      auto l_root           = l_uproject.parent_path() / doodle_config::ue4_content;

      auto l_ass_map_file =
          h.get<file_association_ref>().get<file_association>().ue_file.get<assets_file>().path_attr();
      auto l_original = l_ass_map_file.lexically_relative(l_root);
      data_->down_info_.scene_file_ =
          fmt::format("/Game/{}/{}", l_original.parent_path().generic_string(), l_original.stem());
    }
    if (!msg_.all_of<project>()) {
      data_->logger_->log(log_loc(), level::err, "没有项目组件, 失败");
      in_error_code.assign(error_enum::component_missing_error, doodle_category::get());
      BOOST_ASIO_ERROR_LOCATION(in_error_code);
      wait_op_->ec_ = in_error_code;
      wait_op_->complete();
      return;
    }
    if (l_down_path_file_name.empty()) {
      data_->logger_->log(log_loc(), level::err, "没有项目组件, 失败");
      in_error_code.assign(error_enum::component_missing_error, doodle_category::get());
      BOOST_ASIO_ERROR_LOCATION(in_error_code);
      wait_op_->ec_ = in_error_code;
      wait_op_->complete();
      return;
    }
    auto l_local_path = g_root / msg_.get<project>().p_shor_str / l_down_path_file_name;
    // 内容文件夹复制
    l_copy_path.emplace_back(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content);
    if (l_is_se) {
      // 配置文件夹复制
      l_copy_path.emplace_back(l_down_path / doodle_config::ue4_config, l_local_path / doodle_config::ue4_config);
      // 复制项目文件
      l_copy_path.emplace_back(l_uproject, l_local_path / l_uproject.filename());
      data_->down_info_.render_project_ = l_copy_path.back().second;
    }
  }
  g_ctx().get<thread_copy_io_service>().async_copy(
      l_copy_path, FSys::copy_options::recursive, boost::asio::bind_executor(g_io_context(), *this)
  );
}

void down_auto_light_anim_file::operator()(
    boost::system::error_code in_error_code, const maya_exe_ns::maya_out_arg &in_vector
) const {
  if (!data_->logger_) {
    default_logger_raw()->log(log_loc(), level::level_enum::err, "缺失组建错误 缺失日志组件");
    in_error_code.assign(error_enum::component_missing_error, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  if (in_error_code) {
    data_->logger_->log(log_loc(), level::level_enum::err, "maya_to_exe_file error:{}", in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  data_->out_maya_arg_ = in_vector;
  analysis_out_file(in_error_code);
}
void down_auto_light_anim_file::operator()(boost::system::error_code in_error_code) const {
  if (in_error_code) {
    data_->logger_->log(log_loc(), level::level_enum::err, "maya_to_exe_file error:{}", in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  set_info_(data_->down_info_);
  wait_op_->ec_ = in_error_code;
  wait_op_->complete();
}

}  // namespace doodle