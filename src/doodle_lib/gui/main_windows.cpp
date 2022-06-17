//
// Created by TD on 2022/6/17.
//

#include "main_windows.h"
namespace doodle::gui {
class main_windows::impl {
 public:
};
main_windows::main_windows() : pi_(std::make_unique<impl>()) {}

void main_windows::render() {
}
void main_windows::init() {
  window_panel::init();
}
main_windows::~main_windows() = default;
}  // namespace doodle::gui
