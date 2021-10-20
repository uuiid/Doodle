//
// Created by TD on 2021/10/20.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {

class DOODLELIB_API doodle_app_base  :public details::no_copy {

  doodle_lib_ptr p_lib;

 public:
  doodle_app_base();

  virtual std::int32_t run() = 0;
};

class DOODLELIB_API doodle_app_fun : public doodle_app_base {
 public:
  std::function<std::int32_t()> p_fun;
  inline std::int32_t run() override {
    return p_fun();
  };
};

}  // namespace doodle
