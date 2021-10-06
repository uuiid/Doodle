//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/action/command.h>

namespace doodle {
class DOODLELIB_API comm_files_up : public command_base {
 private:
  assets_file_ptr p_root;

  std::vector<FSys::path> p_files;
  bool set_child(const assets_file_ptr& in_ptr);

 public:
  comm_files_up();

  bool render() override;
};

}  // namespace doodle
