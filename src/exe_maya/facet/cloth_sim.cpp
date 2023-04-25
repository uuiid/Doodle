//
// Created by td_main on 2023/4/25.
//

#include "cloth_sim.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/program_options.h>

#include <maya/MGlobal.h>
#include <maya/MLibrary.h>

namespace doodle {
const std::string& cloth_sim::name() const noexcept {
  static const std::string name = "cloth_sim";
  return name;
}
bool cloth_sim::post() {
  if (auto l_str = doodle_lib::Get().ctx().get<program_options>().arg(config).str(); l_str.empty()) {
    return false;
  }
  MLibrary::initialize(true, "maya_doodle");

  return true;
}
void cloth_sim::deconstruction() { MLibrary::cleanup(0, false); }

void cloth_sim::add_program_options() { doodle_lib::Get().ctx().get<program_options>().arg.add_param(config); }

cloth_sim::~cloth_sim() = default;

};  // namespace doodle
