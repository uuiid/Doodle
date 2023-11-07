//
// Created by td_main on 2023/11/7.
//

#include "open_project.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/database_task/sqlite_client.h>

#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle::gui {
#ifdef DNDEBUG
constexpr auto main_project{""};
#else
constexpr auto main_project{"E:/cache/doodle_main.doodle_db"};
#endif

bool open_project::render() {
  if (ImGui::Button("主项目")) {
    g_ctx().get<database_n::file_translator_ptr>()->async_open(main_project);
    open = false;
  }

  ImGui::Text("最近的项目");
  for (auto&& l_p : core_set::get_set().project_root) {
    if (!l_p.empty()) {
      if (ImGui::Button(l_p.generic_string().c_str())) {
        g_ctx().get<database_n::file_translator_ptr>()->async_open(main_project);
        open = false;
      }
    }
  }

  if (ImGui::Button("其他文件")) {
    open = false;
  }
  return open;
}

}  // namespace doodle::gui