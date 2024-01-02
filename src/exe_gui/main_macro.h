//
// Created by td_main on 2023/9/11.
//
#pragma once

#include <doodle_app/app/app_command.h>

#include <tchar.h>
#include <windows.h>
// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {

#define DOODLE_MAIN_IMPL_(facet)                                                     \
  try {                                                                              \
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
#define DOODLE_MAIN_IMPL(facet) extern "C" int main(int argc, const char* const argv[]) DOODLE_MAIN_IMPL_(facet)
#define DOODLE_WMAIN_IMPL(facet) extern "C" int _tmain(int argc, const TCHAR* const argv[]) DOODLE_MAIN_IMPL_(facet)
