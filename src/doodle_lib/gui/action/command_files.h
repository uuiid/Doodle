//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/action/command.h>
//#include <boost/hana/transform.hpp>

namespace doodle {
class DOODLELIB_API comm_files_up : public command_base {
 private:
  assets_file_ptr p_root;
  bool_ptr p_use_relative;

  assets_path_vector_ptr p_list_paths;


  FSys::path p_file;
  rpc_trans::trans_file_ptr p_tran_files;

  bool set_child();
  bool add_files();


 public:
  comm_files_up();

  bool render() override;
};

}  // namespace doodle
