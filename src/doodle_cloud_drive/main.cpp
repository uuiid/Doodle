//
// Created by td_main on 2023/7/5.
//
/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <doodle_app/app/app_command.h>
#include <doodle_app/doodle_app_fwd.h>

#include <doodle_cloud_drive/facet/cloud_drive_facet.h>
#include <filesystem>
#include <fstream>
#include <iostream>

// #include <doodle_lib/DoodleApp.h>
// #include <boost/locale.hpp>

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
extern "C" int main(int argc, const char* const argv[]) try {
  using main_app = doodle::app_command<doodle::cloud_drive_facet>;
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
} catch (...) {
  auto l_path = std::filesystem::temp_directory_path() / "doodle_cloud_drive.log";
  std::ofstream{l_path} << boost::current_exception_diagnostic_information(true);
  return 1;
}
