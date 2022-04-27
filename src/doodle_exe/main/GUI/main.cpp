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
//#include <doodle_lib/DoodleApp.h>
//#include <boost/locale.hpp>

extern "C" int WINAPI wWinMain(HINSTANCE hInstance,
                               HINSTANCE hPrevInstance,
                               PWSTR strCmdLine,
                               int nCmdShow) try {
  //  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF );
  //  std::locale::global(std::locale{".UTF8"});
  //  std::locale::global(std::locale::classic());
  //  std::wcout.imbue(std::locale{".UTF8"});
  //  boost::locale::generator k_gen{};
  //  k_gen.categories(boost::locale::all_categories ^
  //                   boost::locale::formatting_facet ^
  //                   boost::locale::parsing_facet);
  //  std::locale::global(k_gen("zh_CN.UTF-8"));
  //  std::setlocale(LC_NUMERIC, "C");
  //  std::setlocale(LC_COLLATE, "C");
  //  std::setlocale(LC_TIME, "C");
  //  std::setlocale(LC_MONETARY, "C");
  //  std::setlocale(LC_CTYPE, ".UTF8");

  doodle::app app{hInstance};
  app.command_line_parser(strCmdLine);
  return app.run();
} catch (const std::exception& err) {
  std::cout << err.what() << std::endl;
  return 1;
} catch (...) {
  return 1;
}
