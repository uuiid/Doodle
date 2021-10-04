//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/action/command.h>

namespace doodle {
class DOODLELIB_API comm_files_up : public command_meta {
 private:
  assets_file_ptr p_root;
  metadata_ptr p_parent;

  std::vector<FSys::path> p_files;

 public:
  comm_files_up();

  bool render() override;
  bool add_data(const metadata_ptr& in_parent, const metadata_ptr& in) override;
};

}  // namespace doodle
 