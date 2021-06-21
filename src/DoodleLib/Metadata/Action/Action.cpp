//
// Created by TD on 2021/6/17.
//

#include "Action.h"

#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>
namespace doodle {
Action::Action(std::any&& in_any)
    : p_any(std::move(in_any)),
      sig_get_input() {}
Action::Action()
    : p_any(),
      sig_get_input() {
}
void Action::set_any(std::any&& in_any) {
  p_any = std::move(in_any);
}
void Action::set_any() {
  auto k_a = sig_get_input();
  if (!k_a) {
    DOODLE_LOG_DEBUG("没有获得值");
    throw DoodleError{"没有获得值"};
  }
  p_any = k_a.get();
}
}  // namespace doodle
