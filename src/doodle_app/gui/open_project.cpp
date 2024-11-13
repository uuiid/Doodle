//
// Created by td_main on 2023/11/7.
//

#include "open_project.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/authorization.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <fmt/chrono.h>
namespace doodle::gui {

constexpr auto updata_log{R"(本次更新:
{0} 新增了启动界面,{1} 同时由于和退出弹窗有冲突,我们去除了退出弹窗
{0} 新增授权查看, 可以在界面中更新授权
{0} maya导出fbx已经完成, 可以进行使用了
{0} 新加入主项目, 为灯光自动项目, {1}不要在主项目中放无关自动灯光的文件连接
{0} 取消了保存, 改为自动保存, 每次修改都会自动保存
)"};

open_project::open_project() : auth_ptr_(std::make_shared<authorization>()) {
  const auto l_time = chrono::floor<chrono::days>(auth_ptr_->get_expire_time());
  //  expire_time_str_  = fmt::vformat("还有 {:%d} 天到期", fmt::make_format_args(l_time));
  expire_time_str_  = fmt::format("还有 {} 天到期", l_time);
  next_time_.data = next_time_backup_ = core_set::get_set().next_time_;
  cmd_path_                           = FSys::from_quotation_marks(app_base::Get().arg()[1]);
}

bool open_project::render() {
  ImGui::Text(expire_time_str_.c_str());
  if (ImGui::InputText(*auth_code_, &auth_code_)) {
    auth_ptr_->load_authorization_data(auth_code_);
    expire_time_str_ = fmt::format("还有 {} 天到期", chrono::floor<chrono::days>(auth_ptr_->get_expire_time()));
  }
  static auto l_str = fmt::format(updata_log, "*", "!!!");
  ImGui::TextUnformatted(l_str.c_str());
  ImGui::Dummy(ImVec2{30, 30});

  if (auth_ptr_->is_expire()) {
    if (ImGui::Button("临时项目(不会保存!!)", ImVec2{-FLT_MIN, 0})) {
      core_set::get_set().next_time_ = true;
      core_set_init{}.write_file();
      open = false;
    }
    if (!cmd_path_.empty()) {
      ImGui::Text("传入的项目");
      if (ImGui::Button(cmd_path_.generic_string().c_str(), ImVec2{-FLT_MIN, 0})) {
        g_ctx().get<database_n::file_translator_ptr>()->async_open(cmd_path_, false, false, g_reg(), [](auto&&) {});
        open = false;
      }
    }

    ImGui::Text("最近的项目");
    for (auto&& l_p : core_set::get_set().project_root) {
      if (!l_p.empty()) {
        if (ImGui::Button(l_p.generic_string().c_str(), ImVec2{-FLT_MIN, 0})) {
          g_ctx().get<database_n::file_translator_ptr>()->async_open(l_p, false, false, g_reg(), [](auto&&) {});
          open = false;
        }
      }
    }

    if (ImGui::Button("其他文件", ImVec2{-FLT_MIN, 0})) {
      g_windows_manage().create_windows_arg(
          windows_init_arg{}
              .create<file_dialog>(file_dialog::dialog_args{}.async_read([](const FSys::path& in) mutable {
                g_ctx().get<database_n::file_translator_ptr>()->async_open(in, false, false, g_reg(), [](auto&&) {});
              }))
              .set_title("打开项目")
              .set_render_type<dear::Popup>()
      );
      open = false;
    }
    if (ImGui::Checkbox(*next_time_, &next_time_)) {
      core_set::get_set().next_time_ = next_time_.data;
    }

    if (next_time_backup_) {
      if (auto l_path = FSys::from_quotation_marks(app_base::Get().arg()[1]);
          !l_path.empty() && l_path.extension() == doodle_config::doodle_db_name.data()) {
        g_ctx().get<database_n::file_translator_ptr>()->async_open(l_path, false, false, g_reg(), [](auto&&) {});
      } else if (l_path = core_set::get_set().project_root[0];
                 !l_path.empty() && l_path.extension() == doodle_config::doodle_db_name.data()) {
        g_ctx().get<database_n::file_translator_ptr>()->async_open(l_path, false, false, g_reg(), [](auto&&) {});
      }
      open = false;
      ImGui::CloseCurrentPopup();
    }
  } else {
    ImGui::Text("授权已过期");
  }
  return open;
}

}  // namespace doodle::gui