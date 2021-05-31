/*
 * @Author: your name
 * @Date: 2020-12-12 13:21:34
 * @LastEditTime: 2020-12-15 12:00:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\main.cpp
 */

#include <zmq.hpp>
#include <doodle_server/source/seting.h>
#include <doodle_server/source/server.h>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
// #include <Windows.h>
#include <loggerlib/Logger.h>

#include <exception>
#include <iostream>
#include <thread>
#include <queue>
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

  //初始化设置
  auto &set = doodle::CoreSet::getSet();
  set.init();

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
