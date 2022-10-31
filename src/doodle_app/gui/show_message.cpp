//
// Created by TD on 2022/10/28.
//

#include "show_message.h"

#include <doodle_app/gui/base/ref_base.h>

#include <lib_warp/imgui_warp.h>
namespace doodle::gui {
class show_message::impl {
 public:
  std::string title{"消息"};
  std::string message{};
  gui::gui_cache_name_id button_{"确认"};
};

show_message::show_message() : p_i(std::make_unique<impl>()){};

std::int32_t show_message::flags() const {
  boost::ignore_unused(this);
  return ImGuiWindowFlags_NoSavedSettings;
}
void show_message::set_message(const std::string& in_msg) { p_i->message = in_msg; }
std::string show_message::get_message() { return p_i->message; }

void show_message::set_attr() const {
  ImGui::OpenPopup(title().data());
  ImGui::SetNextWindowSize({640, 360});
}

void show_message::render() {
  ImGui::Text(p_i->message.c_str());

  if (ImGui::Button(*p_i->button_)) {
    ImGui::CloseCurrentPopup();
    show_attr = false;
  }
}
const std::string& show_message::title() const { return p_i->title; }
show_message::~show_message() = default;

}  // namespace doodle::gui