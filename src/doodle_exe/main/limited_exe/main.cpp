//
// Created by TD on 2022/5/27.
//
#include <gui/app.h>
// #include <doodle_lib/DoodleApp.h>
// #include <boost/locale.hpp>

extern "C" int WINAPI wWinMain(HINSTANCE hInstance,
                               HINSTANCE hPrevInstance,
                               PWSTR strCmdLine,
                               int nCmdShow) try {
  limited_app app{hInstance};
  app.command_line_parser(strCmdLine);
  return app.run();
} catch (const std::exception& err) {
  std::cout << boost::diagnostic_information(err) << std::endl;
  return 1;
} catch (...) {
  return 1;
}
