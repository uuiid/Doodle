//
// Created by TD on 2024/1/5.
//

#pragma once
#include <doodle_core/core/wait_op.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
namespace doodle {
class down_auto_light_anim_file {
  template <typename Handler>
  struct wait_handle : detail::wait_op {
   public:
    explicit wait_handle(Handler&& handler)
        : detail::wait_op(&wait_handle::on_complete, std::make_shared<Handler>(handler)) {}
    ~wait_handle() = default;
    FSys::path path_{};
    static void on_complete(wait_op* op) {
      auto l_self = static_cast<wait_handle*>(op);
      boost::asio::post(
          boost::asio::prepend(std::move(*static_cast<Handler*>(l_self->handler_.get())), l_self->ec_, l_self->path_)
      );
    }
  };
  using set_path_t = std::function<void(FSys::path)>;

  enum class status {
    begin,
    end,
  };

  struct data_impl_t {
    status status_{status::begin};
    logger_ptr logger_{};
  };

  entt::handle msg_{};
  FSys::path maya_out_file_{};
  std::shared_ptr<detail::wait_op> wait_op_{};
  set_path_t set_path_;
  std::shared_ptr<data_impl_t> data_{};  // 用于存储数据

  void init();

 public:
  explicit down_auto_light_anim_file(entt::handle in_msg, FSys::path in_maya_out_file)
      : msg_(std::move(in_msg)), maya_out_file_(std::move(in_maya_out_file)), data_(std::make_shared<data_impl_t>()){};
  ~down_auto_light_anim_file() = default;

  template <typename CompletionHandler>
  auto async_down_end(CompletionHandler&& handler) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, FSys::path)>(
        [this](auto&& handler) {
          wait_op_ =
              std::make_shared<wait_handle<std::decay_t<decltype(handler)>>>(std::forward<decltype(handler)>(handler));
          set_path_ = [this](FSys::path in_path) {
            std::dynamic_pointer_cast<wait_handle<std::decay_t<decltype(handler)>>>(wait_op_)->path_ =
                std::move(in_path);
          };
        },
        handler
    );
  }

  void operator()(boost::system::error_code in_error_code) const;
};
}  // namespace doodle