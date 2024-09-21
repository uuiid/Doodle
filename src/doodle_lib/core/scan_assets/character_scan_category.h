//
// Created by TD on 2023/12/20.
//

#pragma once

#include <doodle_lib/core/scan_assets/base.h>
namespace doodle::details {

/**
 * 角色扫描, 路径规范
 * "\_(版本)" 可空**
 * maya文件可能没有**
 * 编号是 (数字)(A-Z)**
 *
 * `项目根目录/6-moxing/Ch/JD01_01/Ch(编号)/Mod/Ch(编号).ma`
 * `项目根目录/6-moxing/Ch/JD01_01/Ch(编号)/(名字)_UE(主版本号)/(名字)_UE(主版本号).uproject`
 * `项目根目录/6-moxing/Ch/JD01_01/Ch(编号)/(名字)_UE(主版本号)/Content/Character/(名字)/Meshs/(名字)_(版本).uasset`
 *
 *  rig文件:
 *   `项目根目录/6-moxing/Ch/JD02_21/Ch(编号)/Rig/Ch(编号)_rig_(制作人).ma`
 */
class character_scan_category_t : public scan_category_t {
 public:
  class character_scan_category_data_t : public scan_category_data_t {
   public:
    std::int32_t begin_episode_;

    FSys::path Ch_path_;
  };
  std::vector<scan_category_data_ptr> scan(const std::shared_ptr<project_helper::database_t>&in_root) const override;
};
}  // namespace doodle::gui::details