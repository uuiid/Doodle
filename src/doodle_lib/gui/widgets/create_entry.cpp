//
// Created by td_main on 2023/3/30.
//

#include "create_entry.h"

#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/assets.h"
#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/project.h"
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/core/image_loader.h>

#include "assets_filter_widget.h"
#include "entt/entity/fwd.hpp"
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

void create_entry::create_ass_type_list() {
  for (auto &&[e, ass] : g_reg()->view<assets>().each()) {
    if (!ass.get_parent()) {
      ass_type_list_.emplace_back(entt::handle{*g_reg(), e}, ass.p_path);
    }
  }
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
  else {
    sources_file_type_ = sources_file_type::other_files;
    find_duplicate_file();
    create_ass_type_list();
  }
}

void create_entry::find_duplicate_file() {
  auto l_view = g_reg()->view<assets_file, database>().each();
  auto l_uuid_map =
      l_view | ranges::views::transform([](const decltype(*l_view.begin()) &in_pair) -> std::pair<uuid, entt::entity> {
        return {std::get<database &>(in_pair).uuid(), std::get<entt::entity>(in_pair)};
      }) |
      ranges::to<std::map>();
  auto l_path_map = args_->paths_ |
                    ranges::views::transform([](const FSys::path &in_path) -> std::pair<FSys::path, uuid> {
                      return {in_path, FSys::software_flag_file(in_path)};
                    }) |
                    ranges::to<std::map>();
  duplicate_paths_ = args_->paths_ | ranges::views::filter([&](const FSys::path &in_path) -> bool {
                       auto l_uuid = l_path_map.at(in_path);
                       return l_uuid_map.contains(l_uuid);
                     }) |
                     ranges::to_vector;
  args_->paths_ = args_->paths_ | ranges::views::filter([&](const FSys::path &in_path) -> bool {
                    auto l_uuid = l_path_map.at(in_path);
                    return !l_uuid_map.contains(l_uuid);
                  }) |
                  ranges::to_vector;
  duplicate_handles_ = duplicate_paths_ | ranges::views::transform([&](const FSys::path &in_path) -> entt::handle {
                         return {*g_reg(), l_uuid_map.at(l_path_map.at(in_path))};
                       }) |
                       ranges::to_vector;
}

void create_entry::render_other_files() {
  if (!duplicate_paths_.empty())
    dear::Text(fmt::format("库中具有重复类(重复的将不进行添加):\n {}", fmt::join(duplicate_paths_, "\n")));

  ImGui::Text("添加文件类别:");
  for (auto &&i : ass_type_list_) {
    if (ImGui::Selectable(i.asset_type_.c_str())) {
      if (auto *l_f = g_windows_manage().find_windows<assets_filter_widget>()) {
        l_f->init();
      }
      auto l_handle = args_->paths_ | ranges::views::transform([=, this](const FSys::path &in_path) -> entt::handle {
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
                        l_ent.emplace<database>(FSys::software_flag_file(in_path));
                        l_ent.emplace<assets_file>(in_path).assets_attr(i.handle_);

                        ue_main_map::find_ue_project_file(l_ent);
                        return l_ent;
                      }) |
                      ranges::to_vector;
      l_handle |= ranges::actions::push_back(duplicate_handles_);
      args_->fun_(l_handle);
      ImGui::CloseCurrentPopup();
    }
  }
}
void create_entry::render_project_open_files() {
  auto l_prj_path  = args_->paths_.front();
  auto l_text_size = ImGui::CalcTextSize("打开");
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + l_text_size.y * 2);
  ImGui::Dummy(l_text_size);
  ImGui::SameLine();
  dear::Text(fmt::format("打开库文件 {}", l_prj_path));
  auto l_main_text_size = ImGui::GetItemRectSize();
  ImGui::SameLine();
  ImGui::Dummy(l_text_size);

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + l_text_size.y);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_text_size.x);
  if (ImGui::Button("打开")) {
    ImGui::CloseCurrentPopup();
    g_ctx().get<database_n::file_translator_ptr>()->async_open(l_prj_path, false, false, g_reg(), [](auto &&) {});
  }
  ImGui::SameLine();

  {
    auto l_dummy_x = (l_main_text_size.x - l_text_size.x * 3 / 2) / 2;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_dummy_x);
    if (ImGui::Button("导入")) {
      ImGui::CloseCurrentPopup();
      g_ctx().get<database_n::file_translator_ptr>()->async_import(l_prj_path);
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_dummy_x);
  }
  if (ImGui::Button("取消")) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  ImGui::Dummy({l_text_size.x / 2, l_text_size.y});
  ImGui::Dummy(l_main_text_size);
}
void create_entry::render_project_import_files() {
  auto l_text_size = ImGui::CalcTextSize("打开");
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + l_text_size.y * 2);
  ImGui::Dummy(l_text_size);
  ImGui::SameLine();
  dear::Text(fmt::format("导入库文件:\n{}", fmt::join(args_->paths_, "\n")));
  auto l_main_text_size = ImGui::GetItemRectSize();
  ImGui::SameLine();
  ImGui::Dummy(l_text_size);

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + l_text_size.y);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_text_size.x);
  if (ImGui::Button("导入(暂时无法使用)")) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();

  auto l_dummy_x = l_main_text_size.x - l_text_size.x * 2;
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + l_dummy_x);

  if (ImGui::Button("取消")) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  ImGui::Dummy({l_text_size.x / 2, l_text_size.y});
  ImGui::Dummy(l_text_size);
}

}  // namespace doodle::gui