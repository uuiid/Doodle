//
// Created by TD on 2022/5/7.
//

#include "name_filter_factory.h"
#include <gui/widgets/assets_filter_widgets/name_filter.h>
#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui {
class name_filter_factory::impl {
 public:
  gui_cache<std::string> name_{"制作人"s, ""s};
};

name_filter_factory::name_filter_factory() : ptr(std::make_unique<impl>()){};

bool name_filter_factory::render() {
  bool result{false};
  if (ImGui::InputText(*ptr->name_.gui_name, &ptr->name_.data, ImGuiInputTextFlags_EnterReturnsTrue)) {
    this->is_edit = true;
    result        = true;
  }
  dear::HelpMarker{"使用 enter 建开始搜素"};
  return result;
}
std::unique_ptr<filter_base> name_filter_factory::make_filter_() {
  return std::make_unique<name_filter>(ptr->name_.data);
}
void name_filter_factory::refresh_() {
}
void name_filter_factory::init() {
  ptr->name_.data.clear();
}
name_filter_factory::~name_filter_factory() = default;

}  // namespace doodle::gui
