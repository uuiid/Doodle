//
// Created by TD on 2021/9/16.
//

#include "assets_file_widgets.h"

#include <doodle_lib/Gui/factory/attribute_factory_interface.h>
#include <doodle_lib/Metadata/metadata_cpp.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
namespace doodle {
namespace details {
void table_column::frame_render(const assets_file_ptr& in_ptr) {
  imgui::TableNextColumn();
  p_render(in_ptr);
}
}  // namespace details

bool assets_file_widgets::add_colum_render() {
  auto k_col      = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "id";
  k_col->p_width  = 6;
  k_col->p_render = [this](const assets_file_ptr& in_) -> bool {
    if (dear::Selectable(in_->get_id_str(),
                         in_ == p_current_select,
                         ImGuiSelectableFlags_SpanAllColumns)) {
      p_current_select = in_;
      p_current_select->attribute_widget(p_factory);
      select_change(p_current_select);
    }
    return true;
  };

  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "版本";
  k_col->p_width  = 6;
  k_col->p_render = [this](const assets_file_ptr& in_) -> bool {
    dear::Text(in_->get_version_str());
    return true;
  };

  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "评论";
  k_col->p_render = [this](const assets_file_ptr& in_) -> bool {
    auto com = in_->get_comment();

    if (com)
      dear::Text(com->get().empty() ? std::string{} : com->get().front()->get_comment());
    else
      dear::Text(std::string{});
    return true;
  };

  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "路径";
  k_col->p_width  = 13;
  k_col->p_render = [](const assets_file_ptr& in_) -> bool {
    auto k_path = in_->get_path_file();
    string k_all_str{};
    string k_line_str{};

    if (k_path && !k_path->get().empty()) {
      k_line_str = k_path->get().front()->get_server_path().generic_string();
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
  k_col->p_render = [this](const assets_file_ptr& in_) -> bool {
    dear::Text(in_->get_time()->show_str());
    return true;
  };
  k_col           = p_colum_list.emplace_back(new_object<details::table_column>());
  k_col->p_name   = "制作人";
  k_col->p_width  = 6;
  k_col->p_render = [this](const assets_file_ptr& in_) -> bool {
    dear::Text(in_->get_user());
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
        if (p_root) {
          for (const auto& i : p_root->get_child()) {
            if (details::is_class<assets_file>(i)) {
              auto k = std::dynamic_pointer_cast<assets_file>(i);
              imgui::TableNextRow();
              ///添加每行渲染
              for (auto& i : p_colum_list)
                i->frame_render(k);
            }
          }
        }
      };
}

void assets_file_widgets::set_metadata(const metadata_ptr& in_ptr) {
  p_root = in_ptr;
  p_root->select_indb();
}

}  // namespace doodle
