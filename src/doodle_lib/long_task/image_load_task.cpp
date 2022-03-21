//
// Created by TD on 2022/2/25.
//

#include "image_load_task.h"

#include <metadata/image_icon.h>
#include <thread_pool/thread_pool.h>
#include <core/image_loader.h>
#include <metadata/project.h>
namespace doodle {
class image_load_task::impl {
 public:
  entt::handle handle_;
  image_icon image_;
  std::future<void> result_;
};
image_load_task::image_load_task(const entt::handle &in_handle)
    : p_i(std::make_unique<impl>()) {
  p_i->handle_ = in_handle;
}
void image_load_task::init() {
  if (p_i->handle_) {
    chick_true<doodle_error>(p_i->handle_.any_of<image_icon>(), DOODLE_LOC, "缺失图片组件");
    p_i->image_ = p_i->handle_.get<image_icon>();
    if (!p_i->image_.image) {
      p_i->result_ = g_thread_pool().enqueue([this, l_p = p_i->handle_.registry()->ctx<project>().p_path]() {
        image_loader{}.load(p_i->image_, l_p);
      });
    }
  }
}
void image_load_task::succeeded() {
  if (p_i->handle_)
    p_i->handle_.replace<image_icon>(p_i->image_);
}
void image_load_task::failed() {
}
void image_load_task::aborted() {
  if (p_i->result_.valid())
    p_i->result_.get();
}
void image_load_task::update(const chrono::duration<chrono::system_clock::rep, chrono::system_clock::period> &, void *data) {
  if (p_i->result_.valid())
    switch (p_i->result_.wait_for(0ns)) {
      case std::future_status::ready: {
        try {
          p_i->result_.get();
          this->succeed();
        } catch (const doodle_error &error) {
          DOODLE_LOG_ERROR(error.what());
          this->fail();
          throw;
        }
      } break;
      default:
        break;
    }
  else
    this->succeed();
}
image_load_task::~image_load_task() = default;
}  // namespace doodle
