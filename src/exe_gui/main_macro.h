//
// Created by td_main on 2023/9/11.
//
#pragma once
#include <mimalloc.h>
#include <mimalloc-new-delete.h>

#pragma comment(linker, "/include:mi_version")

#include <doodle_lib/core/app_base.h>

#include <tchar.h>
#include <windows.h>
#define DOODLE_MI_VERSION mi_version();
//  int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
#define DOODLE_MAIN_IMPL_(app_class)                                                   \
  {                                                                                    \
    DOODLE_MI_VERSION                                                                  \
    using main_app = app_class;                                                        \
    main_app app{argc, argv};                                                          \
    try {                                                                              \
      return app.run();                                                                \
    } catch (...) {                                                                    \
      SPDLOG_ERROR(boost::current_exception_diagnostic_information(true));             \
      std::cout << boost::current_exception_diagnostic_information(true) << std::endl; \
      return 1;                                                                        \
    }                                                                                  \
    return 0;                                                                          \
  }

#define DOODLE_MAIN_IMPL(app_class) int main(int argc, const char* const argv[]) DOODLE_MAIN_IMPL_(app_class)
#define DOODLE_WMAIN_IMPL(app_class) int _tmain(int argc, const TCHAR* const argv[]) DOODLE_MAIN_IMPL_(app_class)
#define DOODLE_SERVICE_MAIN_IMPL(app_class) int main(int argc, const char* const argv[]) DOODLE_MAIN_IMPL_(app_class)
