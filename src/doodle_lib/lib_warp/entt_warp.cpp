//
// Created by TD on 2022/1/7.
//
#include "entt_warp.h"

namespace doodle::entt_tool {
scoped_function::~scoped_function() {
  for (auto &&l_fun : fun_list) {
    if (l_fun)
      l_fun();
  }
}
}  // namespace doodle::entt_tool
