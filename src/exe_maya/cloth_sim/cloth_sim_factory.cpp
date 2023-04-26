//
// Created by td_main on 2023/4/25.
//

#include "cloth_sim_factory.h"

#include "maya/MApiNamespace.h"

namespace doodle::maya_plug {

void cloth_sim_factory::preparation(const maya_exe_ns::qcloth_arg& in_arg) {}

void cloth_sim_factory::operator()() const {
  for (auto&& i : run_args_) {
    i();
  }
}
}  // namespace doodle::maya_plug