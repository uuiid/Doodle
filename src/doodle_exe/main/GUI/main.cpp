/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <doodle_lib/doodle_lib_all.h>
//#include <doodle_lib/DoodleApp.h>
//#include <boost/locale.hpp>

extern "C" int WINAPI WinMain(HINSTANCE hInstance,
                              HINSTANCE hPrevInstance,
                              LPSTR strCmdLine,
                              int nCmdShow) try {
  //  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF );
  //  std::locale::global(std::locale{".UTF8"});
  //  std::setlocale(LC_NUMERIC, "C");
  //  std::setlocale(LC_TIME, "C");
  //  std::setlocale(LC_MONETARY, "C");
  //  std::locale::global(std::locale::classic());
  //  std::wcout.imbue(std::locale{".UTF8"});
  std::setlocale(LC_CTYPE, ".UTF8");

  auto doodleLib = doodle::make_doodle_lib();

#ifndef NDEBUG
  auto& set                = doodle::core_set::getSet();
  auto p_rpc_server_handle = std::make_shared<doodle::rpc_server_handle>();
  p_rpc_server_handle->run_server(set.get_meta_rpc_port(), set.get_file_rpc_port());
#endif

  doodleLib->init_gui();
  return doodle::new_object<doodle::doodle_app>()->run();
} catch (const std::exception& err) {
  std::cout << err.what() << std::endl;
  //  DOODLE_LOG_ERROR(err.what());
  //  doodle::CoreSet::getSet().writeDoodleLocalSet();
  return 1;
} catch (...) {
  //  doodle::CoreSet::getSet().writeDoodleLocalSet();
  return 1;
}
