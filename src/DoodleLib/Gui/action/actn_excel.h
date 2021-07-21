//
// Created by TD on 2021/7/21.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>
namespace doodle {
namespace action_arg {
class arg_excel {
};
}  // namespace action_arg
class DOODLELIB_API actn_excel : public action_indirect<action_arg::arg_excel> {
 public:
  actn_excel();
};
}  // namespace doodle
