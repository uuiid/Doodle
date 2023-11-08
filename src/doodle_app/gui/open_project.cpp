//
// Created by td_main on 2023/11/7.
//

#include "open_project.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/database_task/sqlite_client.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/authorization.h>
#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <fmt/chrono.h>
namespace doodle::gui {
#ifdef DNDEBUG
constexpr auto main_project{"//192.168.10.250/public/Prism_projects/db_file/doodle_main.doodle_db"};
#else
constexpr auto main_project{"E:/cache/doodle_main.doodle_db"};
#endif

open_project::open_project() : auth_ptr_(std::make_shared<authorization>()) {
  const auto l_time = chrono::floor<chrono::days>(auth_ptr_->get_expire_time());
  //  expire_time_str_  = fmt::vformat("还有 {:%d} 天到期", fmt::make_format_args(l_time));
  expire_time_str_  = fmt::format("还有 {} 天到期", l_time);
}

bool open_project::render() {
  ImGui::Text(expire_time_str_.c_str());
  if (ImGui::InputText(*auth_code_, &auth_code_)) {
    auth_ptr_->load_authorization_data(auth_code_);
    expire_time_str_ = fmt::format("还有 {} 天到期", chrono::floor<chrono::days>(auth_ptr_->get_expire_time()));
  }
  if (ImGui::Button("主项目")) {
    g_ctx().get<database_n::file_translator_ptr>()->async_open(main_project);
    open = false;
  }

  ImGui::Text("最近的项目");
  for (auto&& l_p : core_set::get_set().project_root) {
    if (!l_p.empty()) {
      if (ImGui::Button(l_p.generic_string().c_str())) {
        g_ctx().get<database_n::file_translator_ptr>()->async_open(l_p);
        open = false;
      }
    }
  }

  if (ImGui::Button("其他文件")) {
    g_windows_manage().create_windows_arg(
        windows_init_arg{}
            .create<file_dialog>(file_dialog::dialog_args{}.async_read([](const FSys::path& in) mutable {
              g_ctx().get<database_n::file_translator_ptr>()->async_open(in);
            }))
            .set_title("打开项目")
            .set_render_type<dear::Popup>()
    );
    open = false;
  }
  return open;
}

}  // namespace doodle::gui