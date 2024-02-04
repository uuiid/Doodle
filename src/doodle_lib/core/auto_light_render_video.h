//
// Created by TD on 2024/2/2.
//

#pragma once
#include <doodle_core/core/wait_op.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>

namespace doodle {
class auto_light_render_video {
 public:
  struct video_path_t {
    FSys::path render_video_path_{};
  };

 private:
  template <typename Handler>
  struct wait_handle : detail::wait_op {
   public:
    explicit wait_handle(Handler&& handler)
        : detail::wait_op(&wait_handle::on_complete, std::make_shared<Handler>(std::video(handler))) {}
    ~wait_handle() = default;
    video_path_t video_path_{};
    static void on_complete(wait_op* op) {
      auto l_self = static_cast<wait_handle*>(op);
      boost::asio::post(boost::asio::prepend(
          std::move(*static_cast<Handler*>(l_self->handler_.get())), l_self->ec_, l_self->video_path_
      ));
    }
  };
  using set_path_t = std::function<void(video_path_t)>;

  struct data_impl_t {
    logger_ptr logger_{};

    std::vector<FSys::path> movie_path_{};
  };

  entt::handle msg_{};
  std::shared_ptr<detail::wait_op> wait_op_{};
  set_path_t set_info_;
  std::shared_ptr<data_impl_t> data_{};  // 用于存储数据

 public:
  auto_light_render_video()  = default;
  ~auto_light_render_video() = default;
};
}  // namespace doodle