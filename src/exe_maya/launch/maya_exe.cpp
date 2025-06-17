//
// Created by TD on 2023/12/28.
//

#include "maya_exe.h"

#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/exe_warp/maya_exe.h>

#include <exe_maya/core/maya_lib_guard.h>
#include <exe_maya/facet/cloth_sim.h>
#include <exe_maya/facet/export_fbx.h>
#include <exe_maya/facet/export_rig_facet.h>
#include <exe_maya/facet/inspect_file.h>
#include <exe_maya/facet/replace_file.h>
namespace doodle {
bool maya_exe_main::init() {
  static constexpr auto cloth_sim_config{"cloth_sim"};
  static constexpr auto export_fbx_config{"export_fbx"};
  static constexpr auto replace_file_config{"replace_file"};
  static constexpr auto inspect_file_config{"inspect_file"};
  static constexpr auto export_rig_config{"export_rig"};

  default_logger_raw()->log(log_loc(), level::warn, "寻找到自身exe {}", register_file_type::program_location());
  nlohmann::json l_json;
  if (auto l_str = arg_({"config"}); l_str) {
    try {
      l_json = nlohmann::json::parse(FSys::ifstream{FSys::from_quotation_marks(l_str.str())});
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      return true;
    }
  }

  facets_.emplace_back(std::make_shared<maya_plug::maya_lib_guard>());

  if (arg_[cloth_sim_config]) {
    auto l_ptr = std::make_shared<doodle::maya_plug::cloth_sim>();
    boost::asio::post(g_io_context(), [l_json, l_ptr]() { l_ptr->post(l_json); });
    facets_.emplace_back(l_ptr);
  } else if (arg_[export_fbx_config]) {
    auto l_ptr = std::make_shared<doodle::maya_plug::export_fbx_facet>();
    boost::asio::post(g_io_context(), [l_json, l_ptr]() { l_ptr->post(l_json); });
    facets_.emplace_back(l_ptr);
  } else if (arg_[replace_file_config]) {
    auto l_ptr = std::make_shared<doodle::maya_plug::replace_file_facet>();
    boost::asio::post(g_io_context(), [l_json, l_ptr]() { l_ptr->post(l_json); });
    facets_.emplace_back(l_ptr);
  } else if (arg_[inspect_file_config]) {
    auto l_ptr = std::make_shared<doodle::maya_plug::inspect_file>();
    boost::asio::post(g_io_context(), [l_json, l_ptr]() { l_ptr->post(l_json); });
    facets_.emplace_back(l_ptr);
  } else if (arg_[export_rig_config]) {
    auto l_ptr = std::make_shared<doodle::maya_plug::export_rig_facet>();
    boost::asio::post(g_io_context(), [l_json, l_ptr]() { l_ptr->post(l_json); });
    facets_.emplace_back(l_ptr);
  }

  else {
    default_logger_raw()->error("没有寻找到正确的指示标帜");
    return false;
  }

  return true;
}

}  // namespace doodle