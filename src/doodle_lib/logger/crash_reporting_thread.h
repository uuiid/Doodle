//
// Created by td_main on 2023/8/29.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <Windows.h>
namespace doodle::detail {

class crash_reporting_thread {
  class impl;
  std::unique_ptr<impl> impl_ptr_;

  void stop();
  void handle_crash();

 public:
  crash_reporting_thread();
  ~crash_reporting_thread();

  // copy
  crash_reporting_thread(const crash_reporting_thread&) = delete;
  auto& operator=(const crash_reporting_thread&)        = delete;
  // move
  crash_reporting_thread(crash_reporting_thread&&)      = delete;
  auto& operator=(crash_reporting_thread&&)             = delete;

  void run();

  void on_crash_during_static_init(LPEXCEPTION_POINTERS ExceptionInfo);
  void on_crashed(LPEXCEPTION_POINTERS InExceptionInfo);

};

}  // namespace doodle::detail
