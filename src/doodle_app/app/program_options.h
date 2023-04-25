//
// Created by TD on 2021/10/18.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>

// #include <Windows.h>

#include <argh.h>

namespace doodle::details {
class DOODLE_APP_API program_options : boost::noncopyable {
 public:
  argh::parser arg;

  program_options() = default;
  program_options(int argc, const char* const argv[]) : arg{argc, argv} {};

  void init_project()
};
}  // namespace doodle::details