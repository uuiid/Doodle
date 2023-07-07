//
// Created by td_main on 2023/7/5.
//

#include "cloud_drive_facet.h"

#include <doodle_core/core/doodle_lib.h>

#include <doodle_app/app/program_options.h>

#include <doodle_cloud_drive/cloud/cloud_provider_registrar.h>
#include <doodle_cloud_drive/cloud/directory_watcher.h>

namespace doodle {

bool cloud_drive_facet::post() {
  bool l_r{};

  return l_r;
}
void cloud_drive_facet::add_program_options() {
  doodle_lib::Get().ctx().get<program_options>().arg.add_param(cloud_drive_arg::name);
}

}  // namespace doodle