//
// Created by TD on 2022/1/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/platform/win/windows_alias.h>
namespace doodle {
class DOODLELIB_API app_base {
 protected:
  std::wstring p_title;
  static app_base* self;

 public:
  virtual std::int32_t run() = 0;
  virtual void loop_one()    = 0;
  std::atomic_bool& stop();
  virtual bool valid() const;

  static app_base& Get();
};
}  // namespace doodle
