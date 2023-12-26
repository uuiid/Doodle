//
// Created by TD on 2023/12/26.
//

#include "windwos_service_auto_light.h"

#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/platform/win/register_file_type.h>

namespace doodle {

void windwos_service_auto_light_facet_t::add_program_options() {}

bool windwos_service_auto_light_facet_t::post() {
  scan_win_service_ptr_ = std::make_shared<scan_win_service_t>();
  g_ctx().get<database_n::file_translator_ptr>()->async_open(register_file_type::get_main_project());
  g_ctx().get<core_sig>().project_end_open.connect([this]() { scan_win_service_ptr_->start(); });
  return true;
}
}  // namespace doodle