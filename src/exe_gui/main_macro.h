//
// Created by td_main on 2023/9/11.
//
#pragma once
#include <mimalloc.h>

#pragma comment(linker, "/include:mi_version")

#include <doodle_core/core/app_base.h>

#include <tchar.h>
#include <windows.h>

//  int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {

#define DOODLE_MAIN_IMPL_(app_class)                                           \
  try {                                                                        \
    mi_version();                                                              \
    using main_app = app_class;                                                \
    main_app app{argc, argv};                                                  \
    try {                                                                      \
      return app.run();                                                        \
    } catch (const std::exception& err) {                                      \
      DOODLE_LOG_WARN(boost::diagnostic_information(err));                     \
      return 1;                                                                \
    } catch (...) {                                                            \
      DOODLE_LOG_ERROR(boost::current_exception_diagnostic_information(true)); \
      return 1;                                                                \
    }                                                                          \
    return 0;                                                                  \
  } catch (...) {                                                              \
    doodle::app_base::write_current_error_tmp_dir();                           \
    return 1;                                                                  \
  }
#define DOODLE_MAIN_IMPL(app_class) int main(int argc, const char* const argv[]) DOODLE_MAIN_IMPL_(app_class)
#define DOODLE_WMAIN_IMPL(app_class) int _tmain(int argc, const TCHAR* const argv[]) DOODLE_MAIN_IMPL_(app_class)
#define DOODLE_SERVICE_MAIN_IMPL(app_class) int main(int argc, const char* const argv[]) DOODLE_MAIN_IMPL_(app_class)
