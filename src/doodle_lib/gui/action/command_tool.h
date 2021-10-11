//
// Created by TD on 2021/9/19.
//

#pragma once

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class DOODLELIB_API comm_export_fbx : public command_base {
  std::vector<FSys::path> p_files;

 public:
  comm_export_fbx();
  bool is_async() override;
  bool render() override;
};

class DOODLELIB_API comm_qcloth_sim : public command_base {
  FSys::path p_cloth_path;
  std::shared_ptr<std::string> p_text;
  std::vector<FSys::path> p_sim_path;
  bool p_only_sim;

 public:
  comm_qcloth_sim();
  bool is_async() override;
  bool render() override;
};

class DOODLELIB_API comm_create_video : public command_base {
  struct image_paths {
    std::vector<FSys::path> p_path_list;
    FSys::path p_out_path;
    std::string p_show_name;
    bool use_dir;
  };
  std::vector<FSys::path> p_video_path;
  std::vector<image_paths> p_image_path;
  std::shared_ptr<std::string> p_out_path;

 public:
  comm_create_video();
  bool is_async() override;
  bool render() override;
};

class DOODLELIB_API comm_import_ue_files : public command_base {
  FSys::path p_ue4_prj;
  std::shared_ptr<std::string> p_ue4_show;

  std::vector<FSys::path> p_import_list;

 public:
  comm_import_ue_files();
  bool is_async() override;
  bool render() override;
};

}  // namespace doodle
