/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <corelib/core/coreset.h>

#include <doodle_GUI/source/mainWidght/mainWindows.h>
#include <loggerlib/Logger.h>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>

//必要导入

#include <exception>
// DOODLE_NAMESPACE_S
// void doodleQuitClear() {
//   coreSet::getSet().writeDoodleLocalSet();
//   boost::log::core::get()->remove_all_sinks();
// }
// DOODLE_NAMESPACE_E
wxIMPLEMENT_APP_NO_MAIN(doodle::Doodle);

extern "C" int WINAPI WinMain(HINSTANCE hInstance,
                              HINSTANCE hPrevInstance,
                              wxCmdLineArgType WXUNUSED(lpCmdLine),
                              int nCmdShow) try {
  //设置一下文件系统后端
  auto k_local = boost::locale::generator().generate("");
  boost::filesystem::path::imbue(k_local);
  //初始化log
  Logger::doodle_initLog();

  //初始化设置
  auto &set = doodle::coreSet::getSet();
  set.init();

  boost::log::core::get()->remove_all_sinks();

  return wxEntry(hInstance, hPrevInstance, NULL, nCmdShow);
} catch (const std::exception &err) {
  DOODLE_LOG_ERROR(err.what());
  doodle::coreSet::getSet().writeDoodleLocalSet();
  boost::log::core::get()->remove_all_sinks();
  return 1;
} catch (...) {
  doodle::coreSet::getSet().writeDoodleLocalSet();
  boost::log::core::get()->remove_all_sinks();
  return 1;
}