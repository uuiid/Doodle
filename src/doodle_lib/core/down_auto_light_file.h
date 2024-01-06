//
// Created by TD on 2024/1/5.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
namespace doodle {
class down_auto_light_file {
  entt::handle msg_{};
  FSys::path maya_out_file_{};

  struct wait_op {
   public:
    using func_type = void (*)(wait_op *op);

   protected:
    boost::system::error_code ec_{};
    func_type func_{};  // The function to be called when the operation completes.
    wait_op(func_type func) : func_(func) {}
    ~wait_op() = default;
  };

 public:
  explicit down_auto_light_file(entt::handle in_msg, FSys::path in_maya_out_file)
      : msg_(std::move(in_msg)), maya_out_file_(std::move(in_maya_out_file)){};
  ~down_auto_light_file() = default;
};
}  // namespace doodle