//
// Created by TD on 2022/2/25.
//

#include "image_load_task.h"

#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/thread_pool/thread_pool.h>
#include <doodle_core/metadata/project.h>

#include <core/image_loader.h>
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
    DOODLE_CHICK(p_i->handle_.any_of<image_icon>(), doodle_error{"缺失图片组件"});
    p_i->image_ = p_i->handle_.get<image_icon>();
    DOODLE_LOG_INFO("准备加载图片 {}", p_i->image_.path);
    if (!p_i->image_.image) {
      auto l_root  = p_i->image_.image_root(p_i->handle_);
      p_i->result_ = g_thread_pool().enqueue([this, l_p = l_root]() {
        DOODLE_LOG_INFO("后台开始加载图片 {}", p_i->image_.path);
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
  if (p_i->result_.valid()) {
    p_i->result_.get();
  }
}
void image_load_task::update() {
  if (p_i->result_.valid())
    switch (p_i->result_.wait_for(0ns)) {
      case std::future_status::ready: {
        try {
          p_i->result_.get();
          this->succeed();
        } catch (const doodle_error &error) {
          DOODLE_LOG_ERROR(boost::diagnostic_information(error.what()));
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
