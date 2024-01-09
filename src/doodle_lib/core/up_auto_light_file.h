//
// Created by TD on 2024/1/9.
//

#pragma once
#include <doodle_core/core/wait_op.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
namespace doodle {

// 上传自动灯光文件
class up_auto_light_anim_file {
 public:
 private:
  template <typename Handler>
  struct wait_handle : detail::wait_op {
   public:
    explicit wait_handle(Handler&& handler)
        : detail::wait_op(&wait_handle::on_complete, std::make_shared<Handler>(handler)) {}
    ~wait_handle() = default;
    FSys::path up_info_{};
    static void on_complete(wait_op* op) {
      auto l_self = static_cast<wait_handle*>(op);
      boost::asio::post(
          boost::asio::prepend(std::move(*static_cast<Handler*>(l_self->handler_.get())), l_self->ec_, l_self->up_info_)
      );
    }
  };
  using set_path_t = std::function<void(FSys::path)>;

  struct data_impl_t {
    logger_ptr logger_{};
  };

  entt::handle msg_{};
  std::shared_ptr<detail::wait_op> wait_op_{};
  set_path_t set_info_;
  std::shared_ptr<data_impl_t> data_{};  // 用于存储数据

  void init();

 public:
  explicit up_auto_light_anim_file(entt::handle in_msg) : data_(std::make_shared<data_impl_t>()), msg_(in_msg) {
    init();
  }
  ~up_auto_light_anim_file() = default;

  template <typename CompletionHandler>
  auto async_end(CompletionHandler&& handler) {
    return boost::asio::async_compose<CompletionHandler, void(boost::system::error_code, FSys::path)>(
        [this](auto&& handler) {
          wait_op_ =
              std::make_shared<wait_handle<std::decay_t<decltype(handler)>>>(std::forward<decltype(handler)>(handler));
          set_info_ = [this](FSys::path in_path) {
            auto l_wait_op      = std::dynamic_pointer_cast<wait_handle<std::decay_t<decltype(handler)>>>(wait_op_);
            l_wait_op->up_info_ = in_path;
          };
        },
        handler
    );
  }

  void operator()(boost::system::error_code in_error_code, FSys::path in_gen_path) const;
};
}  // namespace doodle