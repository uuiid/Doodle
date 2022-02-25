//
// Created by TD on 2022/2/25.
//

#include "image_load_task.h"

namespace doodle {
class image_load_task::impl {
 public:
};
image_load_task::image_load_task(const entt::handle &in_handle)
    : p_i(std::make_unique<impl>()) {
}
void image_load_task::init() {
}
void image_load_task::succeeded() {
}
void image_load_task::failed() {
}
void image_load_task::aborted() {
}
void image_load_task::update(const chrono::duration<chrono::system_clock::rep, chrono::system_clock::period> &, void *data) {
}
image_load_task::~image_load_task() = default;
}  // namespace doodle
