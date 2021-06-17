//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle{

DoodleLibPtr make_doodle_lib();

class DOODLELIB_API DoodleLib : public details::no_copy{
  friend DoodleLibPtr make_doodle_lib();
  DoodleLib();
  static DoodleLib * p_install;
 public:
  DoodleLib& Get();

  ThreadPoolPtr get_thread_pool();
};
}
