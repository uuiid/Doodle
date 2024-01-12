//
// Created by TD on 2023/12/28.
//

#include "maya_exe.h"

#include <exe_maya/facet/cloth_sim.h>
#include <exe_maya/facet/export_fbx.h>
#include <exe_maya/facet/replace_file.h>
namespace doodle::launch {

bool maya_exe_launcher_t::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  static constexpr auto cloth_sim_config{"cloth_sim_config"};
  static constexpr auto export_fbx_config{"export_fbx_config"};
  static constexpr auto replace_file_config{"replace_file_config"};

  default_logger_raw()->log(log_loc(), level::warn, "开始初始化基本配置");
  core_set_init k_init{};
  default_logger_raw()->log(log_loc(), level::warn, "寻找用户配置文件目录");
  k_init.config_to_user();
  default_logger_raw()->log(log_loc(), level::warn, "读取配置文件");
  k_init.read_file();
  default_logger_raw()->log(log_loc(), level::warn, "寻找到自身exe {}", register_file_type::program_location());
  install_maya();

  if (auto l_str = in_arh(cloth_sim_config); l_str) {
    auto l_ptr = std::make_shared<doodle::maya_plug::cloth_sim>();
    l_ptr->post(l_str.str());
    in_vector.emplace_back(l_ptr);
  } else if (auto l_fbx = in_arh(export_fbx_config); l_fbx) {
    auto l_ptr = std::make_shared<doodle::maya_plug::export_fbx_facet>();
    l_ptr->post(l_fbx.str());
    in_vector.emplace_back(l_ptr);
  } else if (auto l_rep = in_arh(replace_file_config); l_rep) {
    auto l_ptr = std::make_shared<doodle::maya_plug::replace_file_facet>();
    l_ptr->post(l_rep.str());
    in_vector.emplace_back(l_ptr);
  } else {
    default_logger_raw()->error("没有寻找到正确的指示标帜");
    return true;
  }

  return false;
}

}  // namespace doodle::launch