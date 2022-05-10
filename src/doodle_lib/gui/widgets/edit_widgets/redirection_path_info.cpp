//
// Created by TD on 2022/5/10.
//

#include "redirection_path_info.h"

namespace doodle {
namespace gui {

class redirection_path_info::impl {
 public:
  entt::handle handle_;
};
redirection_path_info::redirection_path_info()
    : ptr(std::make_unique<impl>()) {
}
void redirection_path_info::render(const entt::handle& in) {
}
void redirection_path_info::init_(const entt::handle& in) {
}
void redirection_path_info::save_(const entt::handle& in) const {
}

redirection_path_info::~redirection_path_info() = default;
}  // namespace gui
}  // namespace doodle
