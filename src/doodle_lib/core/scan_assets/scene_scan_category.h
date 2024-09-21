//
// Created by TD on 2023/12/20.
//

#pragma once

#include <doodle_lib/core/scan_assets/base.h>

namespace doodle::details {
/// 路径规范
///    `项目根目录/6-moxing/BG/JD(季数)_(集数开始)/BG(编号)/(场景名称)/Content/(场景名称)/Map/(场景名称)_(版本).umap`
/// 检查对于的rig文件和maya文件, maya文件可以不存在, 但是rig文件必须存在
///
/// maya文件(同时也是rig文件):
///    项目根目录/6-moxing/BG/JD(季数)_(集数开始)/BG(编号)/Mod/(场景名称)_(版本)_Low.ma
class scene_scan_category_t : public scan_category_t {
 public:
  class scene_scan_category_data_t : public scan_category_data_t {
   public:
    std::int32_t begin_episode_;
    FSys::path BG_path_;
  };

  scene_scan_category_t()           = default;
  ~scene_scan_category_t() override = default;

  std::vector<scan_category_data_ptr> scan(const std::shared_ptr<project_helper::database_t>&in_root) const override;
};

}  // namespace doodle::gui::details