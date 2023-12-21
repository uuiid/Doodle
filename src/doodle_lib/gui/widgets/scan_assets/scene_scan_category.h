//
// Created by TD on 2023/12/20.
//

#pragma once

#include <doodle_lib/gui/widgets/scan_assets/base.h>

namespace doodle::gui::details {
/// 路径规范
/// `项目根目录/6-moxing/BG/JD(季数)_(集数开始)/BG(编号)/(场景名称)/Content/(场景名称)/Map/(场景名称)_(版本).umap`
class scene_scan_category_t : public scan_category_t {
 public:
  // 捕获数据
  struct capture_data_t {
    std::int32_t begin_episode_;
    // 编号
    std::string number_str_;
    // 版本名称
    std::string version_str_;
  };
  scene_scan_category_t()           = default;
  ~scene_scan_category_t() override = default;

  std::vector<entt::handle> scan(const project_root_t& in_root) const override;
  /// 检查对于的rig文件和maya文件, maya文件可以不存在, 但是rig文件必须存在
  ///
  /// maya文件(同时也是rig文件):
  ///    项目根目录/6-moxing/BG/JD(季数)_(集数开始)/BG(编号)/Mod/(场景名称)_(版本)_Low.ma

  std::vector<entt::handle> check_path(const project_root_t& in_root, entt::handle& in_path) const override;
};

}  // namespace doodle::gui::details