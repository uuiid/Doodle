//
// Created by TD on 2023/12/28.
//
#include "main_gui.h"

#include <doodle_lib/facet/main_facet.h>
namespace doodle::launch {
bool main_gui_launcher_t::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  auto l_gui_facet = std::make_shared<main_facet>();
  l_gui_facet->post();
  in_vector.emplace_back(l_gui_facet);
  return true;
}
}  // namespace doodle::launch