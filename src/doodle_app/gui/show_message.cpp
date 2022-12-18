//
// Created by TD on 2022/10/28.
//

#include "show_message.h"

#include "doodle_core/logger/logger.h"

#include <doodle_app/gui/base/ref_base.h>

#include <lib_warp/imgui_warp.h>
#include <string>

namespace doodle::gui {
class show_message::impl {
 public:
  gui_cache_name_id title{"消息"};
  std::string message{};
  gui::gui_cache_name_id button_{"确认"};
};

show_message::show_message() : p_i(std::make_unique<impl>()){};
show_message::show_message(const std::string& in_msg) : p_i(std::make_unique<impl>()) { set_message(in_msg); };

std::int32_t show_message::flags() const {
  boost::ignore_unused(this);
  return ImGuiWindowFlags_NoSavedSettings;
}
show_message& show_message::set_message(const std::string& in_msg) {
  p_i->message = in_msg;
  DOODLE_LOG_WARN(in_msg);
  return *this;
}
std::string show_message::get_message() { return p_i->message; }

void show_message::set_attr() const {
  ImGui::OpenPopup(title().data());
  ImGui::SetNextWindowSize({640, 360});
}

void show_message::render() {
  dear::Text(p_i->message);

  if (ImGui::Button(*p_i->button_)) {
    ImGui::CloseCurrentPopup();
    show_attr = false;
  }
}
const std::string& show_message::title() const { return p_i->title.name_id; }
show_message::~show_message() = default;

}  // namespace doodle::gui