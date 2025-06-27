//
// Created by TD on 2023/12/20.
//

#pragma once

#include <doodle_lib/core/scan_assets/base.h>

namespace doodle::details {

/**
 * 路径规范
 * - 道具(maya文件)
 * `项目根目录/6-moxing/Prop/JD(季数)_(集数开始)/(道具名称)/(道具名称)_(版本).ma`
 * - 道具(Ue文件)
 * `项目根目录/6-moxing/Prop/JD(季数)_(集数开始)/JD(季数)_(集数开始)_UE/Content/Prop/(道具名称)/Mesh/(道具名称)_(版本).uasset`
 *  - rig道具:
 *  `项目根目录/6-moxing/Prop/JD(季数)_(集数开始)/(道具名称)/Rig/(道具名称)_(版本)_rig_(制作人).ma`
 */
class prop_scan_category_t : public scan_category_t {
 public:
  class prop_scan_category_data_t : public scan_category_data_t {
   public:
    std::int32_t begin_episode_;
    FSys::path JD_path_;
  };

  using scan_category_t::scan_category_t;

  ~prop_scan_category_t() override = default;

  std::vector<scan_category_data_ptr> scan(const std::shared_ptr<project_minimal>& in_root) const override;
};

}  // namespace doodle::details