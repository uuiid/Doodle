//
// Created by TD on 2021/9/15.
//

#pragma once
#include <DoodleLib/Gui/base_windwos.h>
#include <DoodleLib/core/core_set.h>
#include <DoodleLib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API setting_windows : public base_widget {
  decltype(magic_enum::enum_names<department>()) p_dep_list;
  std::int32_t p_cur_dep_index;
  std::shared_ptr<std::string> p_user;
  std::shared_ptr<std::string> p_cache;
  std::shared_ptr<std::string> p_doc;
  std::shared_ptr<std::string> p_maya_path;
  std::shared_ptr<std::string> p_ue_path;
  std::shared_ptr<std::string> p_ue_version;
  std::shared_ptr<std::int32_t> p_batch_max;

 public:
  setting_windows();
  virtual void frame_render() override;
  void save();
};
}  // namespace doodle
