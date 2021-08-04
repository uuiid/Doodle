//
// Created by TD on 2021/8/4.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include "action.h"

namespace doodle::toolbox {

class DOODLELIB_API actn_export_maya
    : public action_toolbox<action_arg::arg_path> {
 public:
  actn_export_maya();
  virtual bool is_async() override;

 protected:
  virtual long_term_ptr run() override;
};
}  // namespace doodle::toolbox
