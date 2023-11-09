//
// Created by TD on 2022/2/25.
//

#include "image_load_task.h"

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/project.h>

#include <boost/asio.hpp>
#include <boost/asio/post.hpp>

#include <core/image_loader.h>

namespace doodle {
class image_load_task::impl {
 public:
  entt::handle handle_;
  image_icon image_;
};
image_load_task::image_load_task() : p_i(std::make_unique<impl>()) {}

void image_load_task::read_image(const entt::handle& in_handle, const image_load_task::call_ptr& in_call) {
  boost::ignore_unused(p_i);
  image_loader l_tmp{};
  auto l_path  = in_handle.get<image_icon>().image_root(in_handle);
  auto l_image = std::make_shared<image_icon>(in_handle.get<image_icon>());
  boost::asio::post(g_thread(), [in_call, l_path, l_image, in_handle]() {
    DOODLE_LOG_INFO("准备加载图片 {}", l_path);
    if (FSys::exists(l_path)) {
      image_loader{}.load(*l_image, l_path);
      DOODLE_LOG_INFO("加载图片 {} 完成", l_path);
    }

    boost::asio::post(g_io_context(), [in_call, in_handle, l_image]() {
      if (in_handle && in_handle.any_of<image_icon>()) {
        in_handle.get<image_icon>() = *l_image;
      } else {
        log_error(fmt::format("无效的句柄 {} 或者缺失组件", in_handle));
      }
      (*in_call)();
    });
  });
}
image_load_task::~image_load_task() = default;
}  // namespace doodle
