//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle{

class DOODLELIB_API DoodleLib : public details::no_copy{
  friend DOODLELIB_API DoodleLibPtr make_doodle_lib();

  static DoodleLib * p_install;
  DoodleLib();

  ThreadPoolPtr p_thread_pool;
 public:
  static DoodleLib& Get();

  ThreadPoolPtr get_thread_pool();
};
}
