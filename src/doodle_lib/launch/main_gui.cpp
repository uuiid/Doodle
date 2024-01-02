//
// Created by TD on 2023/12/28.
//
#include "main_gui.h"

#include <doodle_lib/facet/main_facet.h>
namespace doodle::launch {
bool main_gui_launcher_t::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  default_logger_raw()->log(log_loc(), level::warn, "开始初始化基本配置");
  core_set_init k_init{};
  default_logger_raw()->log(log_loc(), level::warn, "寻找用户配置文件目录");
  k_init.config_to_user();
  default_logger_raw()->log(log_loc(), level::warn, "读取配置文件");
  k_init.read_file();
  default_logger_raw()->log(log_loc(), level::warn, "寻找到自身exe {}", core_set::get_set().program_location());
  auto l_gui_facet = std::make_shared<main_facet>();
  l_gui_facet->post();
  in_vector.emplace_back(l_gui_facet);
  return false;
}
}  // namespace doodle::launch