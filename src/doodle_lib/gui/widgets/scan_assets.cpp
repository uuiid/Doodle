//
// Created by TD on 2023/12/19.
//

#include "scan_assets.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/season.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/gui/widgets/scan_assets/scene_scan_category.h>
namespace doodle::gui {

namespace details {

class scan_assets_config {
 public:
  // 扫瞄资产的根目录
  std::vector<scan_category_t::project_root_t> project_roots_;
  // 扫瞄分类

  std::vector<std::unique_ptr<scan_category_t>> scan_categorys_;
  std::unique_ptr<std::map<uuid, entt::entity>> assets_file_map_;

  // 独步消遥            {"//192.168.10.250/public/DuBuXiaoYao_3", "独步逍遥" },
  // 人间最得意           {"//192.168.10.240/public/renjianzuideyi", "人间最得意" },
  // 无尽神域            {"//192.168.10.240/public/WuJinShenYu", "无尽神域" },
  // 无敌剑魂            {"//192.168.10.240/public/WuDiJianHun", "无敌剑魂" },
  // 万古神话            {"//192.168.10.240/public/WanGuShenHua", "万古神话" },
  // 炼气十万年          {"//192.168.10.240/public/LianQiShiWanNian", "炼气十万年" },
  // 独步万古            {"//192.168.10.240/public/WGXD", "万古邪帝" },
  // 我埋葬了诸天神魔     {"//192.168.10.240/public/LongMaiWuShen", "龙脉武神" },
  // 万域封神           {"//192.168.10.218/WanYuFengShen", "万域封神" }

  scan_assets_config()
      : project_roots_{{"//192.168.10.250/public/DuBuXiaoYao_3", "独步逍遥"},
                       {"//192.168.10.240/public/renjianzuideyi", "人间最得意"},
                       {"//192.168.10.240/public/WuJinShenYu", "无尽神域"},
                       {"//192.168.10.240/public/WuDiJianHun", "无敌剑魂"},
                       {"//192.168.10.240/public/WanGuShenHua", "万古神话"},
                       {"//192.168.10.240/public/LianQiShiWanNian", "炼气十万年"},
                       {"//192.168.10.240/public/WGXD", "万古邪帝"},
                       {"//192.168.10.240/public/LongMaiWuShen", "龙脉武神"},
                       {"//192.168.10.218/WanYuFengShen", "万域封神"}},
        scan_categorys_{},
        assets_file_map_{std::make_unique<std::map<uuid, entt::entity>>()} {
    scan_categorys_.emplace_back(std::make_unique<scene_scan_category_t>());
  }

  void start_sacn() {}

  // 检查路径
  void check_path() {}
};

}  // namespace details

void scan_assets_t::start_scan() {}

bool scan_assets_t::render() {
  if (ImGui::Button(*start_scan_id)) {
    start_scan();
  }
  return is_open;
}
}  // namespace doodle::gui