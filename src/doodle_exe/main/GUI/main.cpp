/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <doodle_lib/app/app.h>
#include <doodle_lib/doodle_lib_all.h>
// #include <doodle_lib/DoodleApp.h>
// #include <boost/locale.hpp>

extern "C" int WINAPI wWinMain(HINSTANCE hInstance,
                               HINSTANCE hPrevInstance,
                               PWSTR strCmdLine,
                               int nCmdShow) try {
  doodle::app app{hInstance};
  try {
    app.command_line_parser(strCmdLine);
    return app.run();
  } catch (const std::exception& err) {
    DOODLE_LOG_WARN(boost::diagnostic_information(boost::diagnostic_information(err)));
    return 1;
  }
} catch (...) {
  return 1;
}
