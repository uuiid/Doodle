//
// Created by td_main on 2023/8/11.
//

#pragma once
#include <doodle_lib/render_farm/detail/render_ue4.h>
namespace doodle {
namespace render_farm {
namespace detail {

class ue4_task {
 public:
  using arg = ue4_arg;

  explicit ue4_task(entt::handle self_handle, arg in_arg)
      : arg_(std::move(in_arg)), self_handle_(std::move(self_handle)) {
    set_meg();
  }
  // 分派任务
  void assign_tasks();

 private:
  void set_meg();
  arg arg_;
  entt::handle self_handle_;
};

}  // namespace detail
using ue4_task_ptr = std::shared_ptr<detail::ue4_task>;
}  // namespace render_farm
}  // namespace doodle