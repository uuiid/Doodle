//
// Created by TD on 2021/9/16.
//

#include "assets_file_widgets.h"

#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <DoodleLib/Metadata/metadata_cpp.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {

assets_file_widgets::assets_file_widgets()
    : p_root(),
      p_current_select() {
  p_class_name = "文件列表";
  p_factory    = new_object<attr_assets_file>();
}

void assets_file_widgets::frame_render() {
  static auto flags{ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
                    ImGuiTableFlags_::ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersV |
                    ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody};
  dear::Table{"attribute_widgets", 5, flags} && [this]() {
    imgui::TableSetupColumn("id");
    imgui::TableSetupColumn("版本");
    imgui::TableSetupColumn("评论", ImGuiTableColumnFlags_WidthStretch);
    imgui::TableSetupColumn("时间");
    imgui::TableSetupColumn("制作人");
    //      imgui::TableSetupColumn("存在文件");
    imgui::TableHeadersRow();
    if (p_root) {
      for (const auto& i : p_root->get_child()) {
        if (details::is_class<assets_file>(i)) {
          auto k = std::dynamic_pointer_cast<assets_file>(i);
          imgui::TableNextRow();
          imgui::TableNextColumn();
          if (dear::Selectable(k->get_id_str(),
                               k == p_current_select,
                               ImGuiSelectableFlags_SpanAllColumns)) {
            p_current_select = k;
            p_current_select->attribute_widget(p_factory);
            select_change(p_current_select);
          }

          imgui::TableNextColumn();
          dear::Text(k->get_version_str());

          imgui::TableNextColumn();
          auto com = k->get_comment();
          if (com)
            dear::Text(com->get().empty() ? std::string{} : com->get().front()->get_comment());
          else
            dear::Text(std::string{});

          imgui::TableNextColumn();
          dear::Text(k->get_time()->show_str());

          imgui::TableNextColumn();
          dear::Text(k->get_user());

          //          imgui::TableNextColumn();
          //          dear::Text()
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
