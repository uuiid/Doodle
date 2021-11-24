//
// Created by TD on 2021/11/04.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/action/command.h>
namespace doodle {
class DOODLELIB_API comm_file_down : public command_base {
  entt::handle p_root;
  string_ptr p_str;
 public:
  comm_file_down();
  virtual bool render() override;
  virtual bool set_data(const entt::handle& in_any) override;
};
}  // namespace doodle
