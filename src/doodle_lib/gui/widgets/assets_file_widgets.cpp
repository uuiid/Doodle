//
// Created by TD on 2021/9/16.
//

#include "assets_file_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>

#include <entt/entt.hpp>

namespace doodle {
namespace details {
void table_column::frame_render(const entt::handle& in_ptr) {
  imgui::TableNextColumn();
  p_render(in_ptr);
}
}  // namespace details

bool assets_file_widgets::add_colum_render() {
  auto k_col      = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "id";
  k_col->p_width  = 6;
  k_col->p_render = [this](const entt::handle& in_) -> bool {
    if (dear::Selectable(in_.get<database>().get_id_str(),
                         in_.entity() == p_current_select,
                         ImGuiSelectableFlags_SpanAllColumns)) {
      p_current_select = in_;
      auto comm        = command_list<comm_ass_file_attr,
                               comm_files_select>{};
                               
      comm.set_data(in_);
      g_reg()->set<widget_>(comm);
      select_change(p_current_select);
    }
    return true;
  };

  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "版本";
  k_col->p_width  = 6;
  k_col->p_render = [this](const entt::handle& in_) -> bool {
    dear::Text(in_.get<assets_file>().get_version_str());
    return true;
  };

  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "评论";
  k_col->p_render = [this](const entt::handle& in_) -> bool {
    auto com = in_.try_get<comment_vector>();

    if (com)
      dear::Text(com->get().empty() ? std::string{} : com->get().front().get_comment());
    else
      dear::Text(std::string{});
    return true;
  };

  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "路径";
  k_col->p_width  = 13;
  k_col->p_render = [](const entt::handle& in_) -> bool {
    auto k_path = in_.try_get<assets_path_vector>();
    string k_all_str{};
    string k_line_str{};

    if (k_path && !k_path->get().empty()) {
      k_line_str = k_path->get().front().get_server_path().generic_string();
      k_all_str  = fmt::format("{}", *k_path);
    }
    dear::Text(k_line_str.c_str());
    if (!k_all_str.empty()) {
      imgui::SameLine();
      dear::HelpMarker{"(...)", k_all_str.c_str()};
    }
    return true;
  };

  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "时间";
  k_col->p_width  = 6;
  k_col->p_render = [this](const entt::handle& in_) -> bool {
    dear::Text(in_.get<time_point_wrap>().show_str());
    return true;
  };
  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "制作人";
  k_col->p_width  = 6;
  k_col->p_render = [this](const entt::handle& in_) -> bool {
    dear::Text(in_.get<assets_file>().get_user());
    return true;
  };

  return false;
}
assets_file_widgets::assets_file_widgets()
    : p_root(),
      p_current_select(),
      p_colum_list() {
  p_class_name = "文件列表";
  p_factory    = new_object<attr_assets_file>();
  add_colum_render();
}

void assets_file_widgets::frame_render() {
  static auto flags{ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
                    ImGuiTableFlags_::ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersV |
                    ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody |
                    ImGuiTableFlags_::ImGuiTableFlags_ScrollX |
                    ImGuiTableFlags_::ImGuiTableFlags_ScrollY};
  dear::Table{"attribute_widgets",
              boost::numeric_cast<std::int32_t>(p_colum_list.size()),
              flags} &&
      [this]() {
        /// 添加表头
        for (auto& i : p_colum_list) {
          if (i->p_width != 0)
            imgui::TableSetupColumn(i->p_name.c_str(), 0, imgui::GetFontSize() * i->p_width);
          else
            imgui::TableSetupColumn(i->p_name.c_str());
        }

        imgui::TableHeadersRow();
        list_data l_data{};

        if (p_root) {
          auto& l_database = p_root.get<database>();
          auto& l_tree     = p_root.get<tree_relationship>();
          for (const auto& i : l_tree.get_child()) {
            auto l_h = make_handle(i);
            if (l_h.all_of<database, tree_relationship, assets_file>()) {
              imgui::TableNextRow();
              for (auto& l_i : p_colum_list)
                l_i->frame_render(l_h);
            }
          }
        }
      };
}

void assets_file_widgets::set_metadata(const entt::entity& in_ptr) {
  p_root = make_handle(in_ptr);
}

}  // namespace doodle
