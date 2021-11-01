//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/action/command.h>
//#include <boost/hana/transform.hpp>

namespace doodle {

class comm_file_image_to_move : public command_base {
 private:
  FSys::path p_out_file;
  bool_ptr p_not_up_file;
  bool_ptr p_not_up_source_file;
  entt::handle p_root;
  image_sequence_async_ptr p_image_create;
  image_sequence_ptr p_image;

  string p_text;

  void init();
  bool updata_file();

 public:
  comm_file_image_to_move()
      :         p_out_file(),
        p_not_up_file(new_object<bool>(false)),
        p_not_up_source_file(new_object<bool>(false)),
        p_root(),
        p_image_create(),
        p_image() {
    init();
  };
  bool render() override;
  bool set_data(const entt::handle& in) override;
};

class DOODLELIB_API comm_files_up : public command_base {
 private:
  entt::handle p_list_paths;
  FSys::path p_file;

 protected:
  virtual bool add_files();

 public:
  comm_files_up();
  bool render() override;
  bool set_data(const entt::handle& in) override;
};

class DOODLELIB_API comm_files_select : public command_base {
 private:
  entt::handle p_root;
  bool_ptr p_use_relative;
  FSys::path p_file;

  command_ptr p_comm_sub;

  bool add_files();

 public:
  comm_files_select();

  bool render() override;
  bool set_data(const entt::handle& in) override;
};

}  // namespace doodle
