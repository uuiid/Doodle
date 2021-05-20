/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <DoodleLib/DoodleLib.h>
#include <boost/locale.hpp>
extern "C" int WINAPI WinMain(HINSTANCE hInstance,
                              HINSTANCE hPrevInstance,
                              wxCmdLineArgType WXUNUSED(lpCmdLine),
                              int nCmdShow) try {
//  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //设置一下文件系统后端
  auto k_local = boost::locale::generator().generate("");
  boost::filesystem::path::imbue(k_local);
  //初始化log
  Logger::doodle_initLog();

  //初始化设置
  auto &set = doodle::coreSet::getSet();
  set.init();
  auto result = wxEntry(hInstance, hPrevInstance, NULL, nCmdShow);
  boost::log::core::get()->remove_all_sinks();

  return result;
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
