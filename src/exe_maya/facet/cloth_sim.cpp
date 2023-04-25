//
// Created by td_main on 2023/4/25.
//

#include "cloth_sim.h"

#include <maya/MGlobal.h>
#include <maya/MLibrary.h>
#include <doodle_core/
// #include <maya/

namespace doodle {
const std::string& cloth_sim::name() const noexcept {
  static const std::string name = "cloth_sim";
  return name;
}
bool cloth_sim::post() {
  MLibrary::initialize(true, "maya_doodle");
  if (files_attr.empty()) {
  }

  return true;
}
void cloth_sim::deconstruction() { MLibrary::cleanup(0, false); }

void cloth_sim::add_program_options() {
  opt.add_options()("cloth_sim_path", boost::program_options::value(&files_attr), "创建视频的序列json选项");
  auto& l_p = doodle_lib::Get().ctx().get<program_options>();
  l_p.add_opt(opt);
}

};  // namespace doodle
