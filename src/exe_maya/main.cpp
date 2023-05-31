//
// Created by td_main on 2023/4/25.
//
#include "doodle_core/logger/logger.h"

#include <doodle_lib/facet/create_move_facet.h>
#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/facet/rpc_server_facet.h>

#include <boost/exception/diagnostic_information.hpp>

#include <exe_maya/facet/cloth_sim.h>
#include <exe_maya/facet/export_fbx.h>
#include <iostream>

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
extern "C" int main(int argc, const char* const argv[]) try {
  using main_app = doodle::app_command<doodle::maya_plug::cloth_sim, doodle::maya_plug::export_fbx_facet>;
  main_app app{argc, argv};
  try {
    return app.run();
  } catch (const std::exception& err) {
    DOODLE_LOG_WARN(boost::diagnostic_information(err));
    return 1;
  } catch (...) {
    DOODLE_LOG_ERROR(boost::current_exception_diagnostic_information(true));
    return 1;
  }
  return 0;
} catch (...) {
  std::cout << boost::current_exception_diagnostic_information(true) << std::endl;
  return 1;
}
