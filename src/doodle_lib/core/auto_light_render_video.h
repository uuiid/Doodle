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
        : detail::wait_op(&wait_handle::on_complete, std::make_shared<Handler>(std::move(handler))) {}
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

  void init();

 public:
  explicit auto_light_render_video(entt::handle in_msg)
      : msg_(std::move(in_msg)), data_(std::make_shared<data_impl_t>()) {
    init();
  };
  ~auto_light_render_video() = default;

  template <typename CompletionHandler>
  auto async_render_video(const entt::handle& in_handle, CompletionHandler&& in_handler) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, video_path_t)>(
        [this](auto&& handler) {
          auto l_op =
              std::make_shared<wait_handle<std::decay_t<decltype(handler)>>>(std::forward<decltype(handler)>(handler));
          wait_op_  = l_op;
          set_info_ = [l_opt = l_op, msg = msg_](video_path_t in_info) {
            l_opt->video_path_ = in_info;
            msg.emplace<video_path_t>(std::move(in_info));
          };
        },
        in_handler
    );
  }

  /// 这个对接ue输出
  void operator()(boost::system::error_code in_error_code, const FSys::path& in_vector) const;
  /// 这个对接合成视屏回调
  void operator()(const FSys::path& in_vector, boost::system::error_code in_error_code) const;
};
}  // namespace doodle