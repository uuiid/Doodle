//
// Created by TD on 2021/9/19.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/command.h>
namespace doodle {
class DOODLELIB_API comm_export_fbx : public command_tool {
  std::vector<FSys::path> p_files;

 public:
  comm_export_fbx();
  virtual bool is_async() override;
  virtual bool run() override;
};

class DOODLELIB_API comm_qcloth_sim : public command_tool {
  FSys::path p_cloth_path;
  std::shared_ptr<std::string> p_text;
  std::vector<FSys::path> p_sim_path;
  bool p_only_sim;

 public:
  comm_qcloth_sim();
  virtual bool is_async() override;
  virtual bool run() override;
};

class DOODLELIB_API comm_create_video : public command_tool {
 public:
  comm_create_video();
  virtual bool is_async() override;
  virtual bool run() override;
};

class DOODLELIB_API comm_connect_video : public command_tool {
 public:
  comm_connect_video();
  virtual bool is_async() override;
  virtual bool run() override;
};

class DOODLELIB_API comm_import_ue_files : public command_tool {
 public:
  comm_import_ue_files();
  virtual bool is_async() override;
  virtual bool run() override;
};

class DOODLELIB_API comm_create_ue_project : public command_tool {
 public:
  comm_create_ue_project();
  virtual bool is_async() override;
  virtual bool run() override;
};
}  // namespace doodle
