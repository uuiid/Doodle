//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API Action : public details::no_copy{
 public:
  Action();
  virtual void run() = 0;
};

}  // namespace doodle
