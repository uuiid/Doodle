//
// Created by TD on 2024/1/5.
//

#pragma once
#include <doodle_core/core/wait_op.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>

#include <exe_warp/maya_exe.h>
namespace doodle {
class down_auto_light_anim_file {
 public:
  struct down_info {
   public:
    FSys::path render_project_{};  // 渲染工程文件
    // 场景文件
    FSys::path scene_file_{};
  };

 private:
  template <typename Handler>
  struct wait_handle : detail::wait_op {
   public:
    explicit wait_handle(Handler&& handler)
        : detail::wait_op(&wait_handle::on_complete, std::make_shared<Handler>(handler)) {}
    ~wait_handle() = default;
    down_info down_info_{};
    static void on_complete(wait_op* op) {
      auto l_self = static_cast<wait_handle*>(op);
      boost::asio::post(boost::asio::prepend(
          std::move(*static_cast<Handler*>(l_self->handler_.get())), l_self->ec_, l_self->down_info_
      ));
    }
  };
  using set_path_t = std::function<void(down_info)>;

  struct data_impl_t {
    logger_ptr logger_{};
    maya_exe_ns::maya_out_arg out_maya_arg_{};

    down_info down_info_{};

    std::vector<FSys::path> extra_update_dir_{};  // 额外添加的上传路径
  };

  entt::handle msg_{};
  std::shared_ptr<detail::wait_op> wait_op_{};
  set_path_t set_info_;
  std::shared_ptr<data_impl_t> data_{};  // 用于存储数据

  void init();

  // 分析输出文件
  void analysis_out_file(boost::system::error_code in_error_code) const;
  void gen_render_config_file() const;

 public:
  explicit down_auto_light_anim_file(entt::handle in_msg)
      : msg_(std::move(in_msg)), data_(std::make_shared<data_impl_t>()){};
  ~down_auto_light_anim_file() = default;

  template <typename CompletionHandler>
  auto async_down_end(CompletionHandler&& handler) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, down_info)>(
        [this](auto&& handler) {
          wait_op_ =
              std::make_shared<wait_handle<std::decay_t<decltype(handler)>>>(std::forward<decltype(handler)>(handler));
          set_info_ = [this](down_info in_info) {
            std::dynamic_pointer_cast<wait_handle<std::decay_t<decltype(handler)>>>(wait_op_)->down_info_ =
                std::move(in_info);
          };
        },
        handler
    );
  }

  void operator()(boost::system::error_code in_error_code, const maya_exe_ns::maya_out_arg& in_vector) const;
  void operator()(boost::system::error_code in_error_code) const;
};
}  // namespace doodle