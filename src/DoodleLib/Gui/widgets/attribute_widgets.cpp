//
// Created by TD on 2021/9/16.
//

#include "attribute_widgets.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {

attribute_widgets::attribute_widgets()
    : p_root(),
      p_current_select() {
}

void attribute_widgets::frame_render() {
  dear::Table{"attribute_widgets", 5} && [this]() {
    imgui::TableSetupColumn("id");
    imgui::TableSetupColumn("版本");
    imgui::TableSetupColumn("评论");
    imgui::TableSetupColumn("时间");
    imgui::TableSetupColumn("制作人");
    //      imgui::TableSetupColumn("存在文件");
    imgui::TableHeadersRow();
    if (p_root)
      for (const auto& i : p_root->child_item) {
        if (details::is_class<AssetsFile>(i)) {
          auto k = std::dynamic_pointer_cast<AssetsFile>(i);
          imgui::TableNextRow();
          imgui::TableNextColumn();
          if (dear::Selectable(k->getIdStr(),
                               k == p_current_select,
                               ImGuiSelectableFlags_SpanAllColumns)) {
            p_current_select = k;
          }

          imgui::TableNextColumn();
          dear::Text(k->getVersionStr());

          imgui::TableNextColumn();
          auto& com = k->getComment();
          dear::Text(com.empty() ? std::string{} : com.front()->getComment());

          imgui::TableNextColumn();
          dear::Text(k->getTime()->showStr());

          imgui::TableNextColumn();
          dear::Text(k->getUser());

          //          imgui::TableNextColumn();
          //          dear::Text()
        }
      }
  };
}

void attribute_widgets::set_metadata(const MetadataPtr& in_ptr) {
  p_root = in_ptr;
}

}  // namespace doodle
