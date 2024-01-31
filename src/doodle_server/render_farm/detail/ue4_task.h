//
// Created by td_main on 2023/8/11.
//

#pragma once
#include <doodle_core/metadata/render_ue4_arg.h>

#include <entt/entt.hpp>

namespace doodle {
namespace render_farm {
namespace detail {

class ue4_task {
 public:
  using arg_t = ue4_arg;

  explicit ue4_task(entt::handle self_handle, arg_t in_arg)
      : arg_(std::move(in_arg)), self_handle_(std::move(self_handle)) {
    set_meg();
  }
  // 分派任务
  void assign_tasks();

  // 是否完成分配
  [[nodiscard]] bool is_assign() const;

  // 成功
  void success();
  bool is_success() const;

  // 失败
  void fail();

  // arg
  [[nodiscard]] inline const arg_t& arg() const { return arg_; }

  void set_meg();
  arg_t arg_;
  entt::handle self_handle_;
  entt::handle computer_handle_;
};

}  // namespace detail
using ue4_task = detail::ue4_task;
}  // namespace render_farm
}  // namespace doodle
