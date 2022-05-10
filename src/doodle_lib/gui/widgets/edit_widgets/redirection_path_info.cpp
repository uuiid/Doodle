//
// Created by TD on 2022/5/10.
//

#include "redirection_path_info_edit.h"

namespace doodle {
namespace gui {

class redirection_path_info_edit::impl {
 public:
  entt::handle handle_;
};
redirection_path_info_edit::redirection_path_info_edit()
    : ptr(std::make_unique<impl>()) {
}
void redirection_path_info_edit::render(const entt::handle& in) {
}
void redirection_path_info_edit::init_(const entt::handle& in) {
  ptr->handle_ = in;
}
void redirection_path_info_edit::save_(const entt::handle& in) const {
}

redirection_path_info_edit::~redirection_path_info_edit() = default;
}  // namespace gui
}  // namespace doodle
