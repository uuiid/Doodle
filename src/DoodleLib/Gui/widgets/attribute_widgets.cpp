//
// Created by TD on 2021/9/16.
//

#include "attribute_widgets.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {

attribute_widgets::attribute_widgets()
    : p_root() {
}

void attribute_widgets::frame_render() {
  if (p_root)
    dear::Table{"attribute_widgets", 3} && [this]() {
      imgui::TableSetupColumn("名称");
      imgui::TableSetupColumn("路径");
      imgui::TableSetupColumn("字母名称");
      imgui::TableHeadersRow();
      for (const auto& i : p_root->child_item) {
        if (details::is_class<AssetsFile>(i)) {

        }
      }
    }
}

void attribute_widgets::set_metadata(const MetadataPtr& in_ptr) {
  p_root = in_ptr;
}

}  // namespace doodle
