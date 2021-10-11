//
// Created by TD on 2021/9/15.
//

#pragma once
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

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
