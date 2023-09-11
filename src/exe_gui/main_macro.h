//
// Created by td_main on 2023/9/11.
//
#pragma once
#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/facet/server_facet.h>
#include <doodle_lib/facet/work_facet.h>

#include <iostream>

// #include <doodle_lib/DoodleApp.h>
// #include <boost/locale.hpp>

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
#define DOODLE_MAIN_IMPL(facet)                                                      \
  extern "C" int main(int argc, const char* const argv[]) try {                      \
    using main_app = doodle::app_command<facet>;                                     \
    main_app app{argc, argv};                                                        \
    try {                                                                            \
      return app.run();                                                              \
    } catch (const std::exception& err) {                                            \
      DOODLE_LOG_WARN(boost::diagnostic_information(err));                           \
      return 1;                                                                      \
    } catch (...) {                                                                  \
      DOODLE_LOG_ERROR(boost::current_exception_diagnostic_information(true));       \
      return 1;                                                                      \
    }                                                                                \
    return 0;                                                                        \
  } catch (...) {                                                                    \
    std::cout << boost::current_exception_diagnostic_information(true) << std::endl; \
    return 1;                                                                        \
  }
