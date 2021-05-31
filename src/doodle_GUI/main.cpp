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
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF );
  //设置一下文件系统后端
  auto k_local = boost::locale::generator().generate("");
  boost::filesystem::path::imbue(k_local);
  //初始化log
  Logger::doodle_initLog();

  ///@warning 初始化设置,在这里我们只初始化最基本的设置,即构造函数中的设置
  ///在这里是没有复杂设置的, 比如一些客户端设置
  auto &set = doodle::CoreSet::getSet();

  auto result = wxEntry(hInstance, hPrevInstance, nullptr, nCmdShow);
  boost::log::core::get()->remove_all_sinks();
  set.clear();
  return result;
} catch (const std::exception &err) {
  DOODLE_LOG_ERROR(err.what());
  doodle::CoreSet::getSet().writeDoodleLocalSet();
  boost::log::core::get()->remove_all_sinks();
  doodle::CoreSet::getSet().clear();
  return 1;
} catch (...) {
  doodle::CoreSet::getSet().writeDoodleLocalSet();
  boost::log::core::get()->remove_all_sinks();
  doodle::CoreSet::getSet().clear();
  return 1;
}
