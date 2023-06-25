//
// Created by td_main on 2023/3/30.
//

#include "create_entry.h"

#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/metadata.h"
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/core/image_loader.h>

#include "entt/entity/fwd.hpp"
#include "fmt/compile.h"
#include "fmt/format.h"
#include "imgui.h"
#include "range/v3/view/transform.hpp"

namespace doodle::gui {
bool create_entry::render() {
  switch (sources_file_type_) {
    case sources_file_type::other_files:
      render_other_files();
      break;
    case sources_file_type::project_open:
      render_project_open_files();
      break;
    case sources_file_type::project_import:
      render_project_import_files();
      break;
  }
  return true;
}
void create_entry::find_icon(const entt::handle &in_handle, const FSys::path &in_path) const {
  image_loader l_image_load{};

  auto &l_config = g_reg()->ctx().get<project_config::base_config>();
  std::regex l_regex{l_config.find_icon_regex};
  FSys::path l_path{in_path};
  if (FSys::is_regular_file(l_path) && l_config.match_icon_extensions(l_path)) {
    l_image_load.save(in_handle, l_path);
    return;
  } else if (FSys::is_regular_file(l_path))
    l_path = in_path.parent_path();
  else
    l_path = in_path;

  if (FSys::is_directory(l_path)) {
    auto k_imghe_path = ranges::find_if(
        ranges::make_subrange(FSys::directory_iterator{l_path}, FSys::directory_iterator{}),
        [&](const FSys::path &in_file) {
          return l_config.match_icon_extensions(in_file) &&
                 std::regex_search(in_file.filename().generic_string(), l_regex);
        }
    );
    if (k_imghe_path != FSys::directory_iterator{}) {
      l_image_load.save(in_handle, k_imghe_path->path());
    }
  }
}

void create_entry::switch_sources_file() {
  if (ranges::all_of(args_->paths_, [](const FSys::path &in_path) {
        return in_path.extension() == doodle_config::doodle_db_name;
      }))
    sources_file_type_ =
        args_->paths_.size() == 1 ? sources_file_type::project_open : sources_file_type::project_import;
  else
    sources_file_type_ = sources_file_type::other_files;
}
void create_entry::render_other_files() {
  args_->fun_(
      args_->paths_ | ranges::views::transform([=](const FSys::path &in_path) -> entt::handle {
        auto l_ent = entt::handle{*g_reg(), g_reg()->create()};
        //        auto l_prj_path = g_reg()->ctx().get<project>().p_path;
        /// \brief 这里使用 lexically_proximate 防止相对路径失败
        //        auto l_path     = in_path.lexically_proximate(l_prj_path);

        season::analysis_static(l_ent, in_path);
        episodes::analysis_static(l_ent, in_path);
        shot::analysis_static(l_ent, in_path);
        episodes::conjecture_season(l_ent);

        if (FSys::exists(in_path)) {
          l_ent.emplace_or_replace<time_point_wrap>(FSys::last_write_time_point(in_path));
        }
        find_icon(l_ent, in_path);
        l_ent.emplace<database>();
        l_ent.emplace<assets_file>(in_path);
        return l_ent;
      }) |
      ranges::to_vector
  );
  ImGui::CloseCurrentPopup();
}
void create_entry::render_project_open_files() {
  auto l_prj_path  = args_->paths_.front();
  auto l_show_str  = fmt::format("打开库文件 {}", l_prj_path);
  auto l_text_size = ImGui::CalcTextSize("打开");
  //  auto l_text_size = ImGui::GetTextLineHeightWithSpacing();
  //  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_text_size.x);
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + l_text_size.y * 2);
  //  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  ImGui::Dummy(l_text_size);
  ImGui::SameLine();
  dear::Text(l_show_str);
  auto l_main_text_size = ImGui::GetItemRectSize();
  ImGui::SameLine();
  ImGui::Dummy(l_text_size);

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + l_text_size.y);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_text_size.x);
  if (ImGui::Button("打开")) {
    ImGui::CloseCurrentPopup();
    doodle_lib::Get().ctx().get<database_n::file_translator_ptr>()->async_open(l_prj_path, [l_prj_path](auto) {
      DOODLE_LOG_INFO("打开项目 {}", l_prj_path);
    });
  }
  ImGui::SameLine();
  ImGui::Dummy({l_main_text_size.x - l_text_size.x / 2, l_text_size.y});
  ImGui::SameLine();
  if (ImGui::Button("取消")) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  ImGui::Dummy({l_text_size.x / 2, l_text_size.y});
  ImGui::Dummy(l_main_text_size);
}
void create_entry::render_project_import_files() {
  auto l_show_str  = fmt::format("打开库文件 {}", fmt::join(args_->paths_, "\n"));
  auto l_text_size = ImGui::CalcTextSize("打开库文件");
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_text_size.x);
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + l_text_size.y * 2);
  //  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  dear::Text(l_show_str);
  ImGui::SameLine();
  ImGui::Dummy(l_text_size);

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + l_text_size.y);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_text_size.x / 2);
  if (ImGui::Button("打开")) {
    //    ImGui::CloseCurrentPopup();
    //    doodle_lib::Get().ctx().get<database_n::file_translator_ptr>()->async_open(l_prj_path, [l_prj_path](auto) {
    //      DOODLE_LOG_INFO("打开项目 {}", l_prj_path);
    //    });
  }
  ImGui::SameLine();
  ImGui::Dummy({l_text_size.x, l_text_size.y});
  ImGui::SameLine();
  if (ImGui::Button("取消")) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::Dummy(l_text_size);
  ImGui::Dummy(l_text_size);
}

}  // namespace doodle::gui