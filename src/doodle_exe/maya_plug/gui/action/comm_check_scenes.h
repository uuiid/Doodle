//
// Created by TD on 2021/11/2.
//

#include <doodle_lib/gui/action/command.h>
namespace doodle::maya_plug {

class comm_check_scenes : public command_base_tool {
 public:
  comm_check_scenes();
  virtual bool render() override;
};
}  // namespace doodle::maya_plug
