//
// Created by TD on 2022/1/22.
//

#include "screenshot_widget.h"
#include <doodle_lib/lib_warp/imgui_warp.h>

namespace doodle {

class screenshot_widget::impl {
 public:
  std::vector<std::function<void()>> begen_loop;
};

screenshot_widget::screenshot_widget()
    : p_i(std::make_unique<impl>()) {
}
screenshot_widget::~screenshot_widget() = default;
void screenshot_widget::init() {
  p_i->begen_loop.emplace_back([]() {
    auto& io = imgui::GetIO();
    imgui::SetNextWindowSize(io.DisplaySize);
    imgui::SetNextWindowPos({0, 0});
    imgui::OpenPopup(name.data());
  });
}
void screenshot_widget::succeeded() {
}
void screenshot_widget::failed() {
}
void screenshot_widget::aborted() {
}
void screenshot_widget::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  for (auto&& fun : p_i->begen_loop) {
    fun();
  }
  p_i->begen_loop.clear();

  dear::PopupModal{name.data()} && []() {
    if (imgui::Button("ok"))
      imgui::CloseCurrentPopup();
  };
}
}  // namespace doodle
