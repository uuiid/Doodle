//
// Created by TD on 2021/9/15.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/base_windwos.h>
#include <DoodleLib/core/CoreSet.h>

namespace doodle {
class DOODLELIB_API setting_windows : public base_windows {
  decltype(magic_enum::enum_names<Department>()) p_dep_list;
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
  virtual void frame_render(const bool_ptr& is_show) override;
  void save();
};
}  // namespace doodle
