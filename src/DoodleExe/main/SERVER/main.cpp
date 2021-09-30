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

void load_setting(const doodle::FSys::path& path) {
  using namespace doodle;
  FSys::ifstream k_file{path, std::ios::in};
  if(k_file){
    auto p_info = nlohmann::json::parse(k_file);
    core_set::getSet().from_json(p_info);
  }
};

#if 0
extern "C" int WINAPI WinMain(HINSTANCE hInstance,
                              HINSTANCE hPrevInstance,
                              LPSTR strCmdLine,
                              int nCmdShow)
#else
extern "C" int main(int argc, char** argv)
#endif
try {
  // ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //  std::locale::global(std::locale{".UTF8"});
  //  std::setlocale(LC_NUMERIC, "C");
  //  std::setlocale(LC_TIME, "C");
  //  std::setlocale(LC_MONETARY, "C");
  //  std::locale::global(std::locale::classic());
  //  std::wcout.imbue(std::locale{".UTF8"});
  std::setlocale(LC_CTYPE, ".UTF8");

  auto doodleLib           = doodle::make_doodle_lib();
  if(argc == 2){
    DOODLE_LOG_INFO("读取和加载配置文件", argv[1])
    load_setting(argv[1]);
  }
  auto& set                = doodle::core_set::getSet();
  auto p_rpc_server_handle = std::make_shared<doodle::rpc_server_handle>();
  p_rpc_server_handle->run_server_wait(set.get_meta_rpc_port(), set.get_file_rpc_port());

  return 0;
  // _CrtDumpMemoryLeaks();
} catch (const std::exception& err) {
  std::cout << err.what() << std::endl;
  DOODLE_LOG_ERROR(err.what());
  doodle::core_set::getSet().write_doodle_local_set();
  return 1;
} catch (...) {
  doodle::core_set::getSet().write_doodle_local_set();
  return 1;
}
