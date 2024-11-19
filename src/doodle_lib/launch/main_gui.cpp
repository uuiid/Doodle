//
// Created by TD on 2023/12/28.
//
#include "main_gui.h"

#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/facet/main_facet.h>
namespace doodle::launch {
bool main_gui_launcher_t::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  default_logger_raw()->log(log_loc(), level::warn, "寻找到自身exe {}", register_file_type::program_location());
  auto l_gui_facet = std::make_shared<main_facet>();
  l_gui_facet->post();
  in_vector.emplace_back(l_gui_facet);
  return false;
}
}  // namespace doodle::launch