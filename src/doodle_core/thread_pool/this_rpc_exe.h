//
// Created by TD on 2022/10/9.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::detail {

class DOODLE_CORE_API this_rpc_exe {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

  void create_rpc_child();

 public:
  this_rpc_exe();

  void stop_exit();
  entt::handle create_move();

  virtual ~this_rpc_exe();
};

}  // namespace doodle::detail
namespace doodle {

using this_rpc_exe_ptr = std::shared_ptr<detail::this_rpc_exe>;

}
