//
// Created by TD on 2022/9/21.
//

#include "maya_tool.h"

#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/organization.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_lib/exe_warp/maya_exe.h>


#include <doodle_lib/gui/gui_ref/ref_base.h>

#include <utility>

namespace doodle {
namespace gui {
maya_tool::maya_tool()
    : p_cloth_path(),
      p_text(),
      p_sim_path(),
      p_only_sim(false),
      p_use_all_ref(false),
      p_upload_files(false),
      p_sim_export_fbx(true),
      p_sim_only_export() {
  title_name_ = std::string{name};
}
void maya_tool::init() {
  g_reg()->ctx().at<core_sig>().project_end_open.connect(
      [this]() {
        p_text = g_reg()->ctx().at<project_config::base_config>().vfx_cloth_sim_path.generic_string();
      }
  );
  g_reg()->ctx().at<core_sig>().select_handles.connect(
      [this](const std::vector<entt::handle>& in_list) {
        p_sim_path = in_list |
                     ranges::views::filter([](const entt::handle& in_handle) -> bool {
                       return in_handle &&
                              in_handle.any_of<assets_file>();
                     }) |
                     ranges::views::filter([](const entt::handle& in_handle) -> bool {
                       auto l_ex = in_handle.get<assets_file>().path_attr().extension();
                       return l_ex == ".ma" ||
                              l_ex == ".mb";
                     }) |
                     ranges::views::transform([](const entt::handle& in_handle) -> FSys::path {
                       return in_handle.get<assets_file>().get_path_normal();
                     }) |
                     ranges::to_vector;
      }
  );

  p_text = g_reg()->ctx().at<project_config::base_config>().vfx_cloth_sim_path.generic_string();
  g_reg()->ctx().emplace<maya_tool&>(*this);
}

void maya_tool::render() {
  dear::ListBox{"file_list"} && [this]() {
    for (const auto& f : p_sim_path) {
      dear::Selectable(f.generic_string());
    }
  };

  dear::Text(fmt::format("解算资产: {}", p_text));

  imgui::Checkbox("自动上传", &p_upload_files);
  dear::TreeNode{"解算设置"} && [this]() {
    imgui::Checkbox("只解算不替换引用", &p_only_sim);
    imgui::Checkbox("导出为fbx格式", &p_sim_export_fbx);
    imgui::Checkbox("只导出", &p_sim_only_export);
  };
  dear::TreeNode{"fbx导出设置"} && [&]() {
    imgui::Checkbox("直接加载所有引用", &p_use_all_ref);
  };

  if (imgui::Button("解算")) {
    std::for_each(p_sim_path.begin(), p_sim_path.end(), [this](const FSys::path& in_path) {
      auto k_arg             = maya_exe_ns::qcloth_arg{};
      k_arg.file_path        = in_path;
      k_arg.only_sim         = p_only_sim;
      k_arg.upload_file      = p_upload_files;
      k_arg.export_fbx       = p_sim_export_fbx;
      k_arg.only_export      = p_sim_only_export;
      k_arg.project_         = g_reg()->ctx().at<database_info>().path_;
      k_arg.t_post           = g_reg()->ctx().at<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().at<project_config::base_config>().export_anim_time;
      g_bounded_pool().attach<maya_exe>(
          make_handle(),
          k_arg
      );
    });
  }
  ImGui::SameLine();
  if (imgui::Button("fbx导出")) {
    std::for_each(p_sim_path.begin(), p_sim_path.end(), [this](const FSys::path& i) {
      auto k_arg             = maya_exe_ns::export_fbx_arg{};
      k_arg.file_path        = i;
      k_arg.use_all_ref      = this->p_use_all_ref;
      k_arg.upload_file      = p_upload_files;
      k_arg.t_post           = g_reg()->ctx().at<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().at<project_config::base_config>().export_anim_time;

      k_arg.project_         = g_reg()->ctx().at<database_info>().path_;
      g_bounded_pool().attach<maya_exe>(
          make_handle(),
          k_arg
      );
    });
  }
  ImGui::SameLine();
  if (imgui::Button("引用文件替换")) {
    std::for_each(p_sim_path.begin(), p_sim_path.end(), [this](const FSys::path& i) {
      auto k_arg             = maya_exe_ns::replace_file_arg{};
      k_arg.file_path        = i;
      k_arg.replace_file_all = true;
      k_arg.project_         = g_reg()->ctx().at<database_info>().path_;
      k_arg.t_post           = g_reg()->ctx().at<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().at<project_config::base_config>().export_anim_time;

      g_bounded_pool().attach<maya_exe>(
          make_handle(),
          k_arg
      );
    });
  }
}
const std::string& maya_tool::title() const {
  return title_name_;
}

}  // namespace gui
}  // namespace doodle
