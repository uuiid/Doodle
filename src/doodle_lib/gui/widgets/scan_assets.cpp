//
// Created by TD on 2023/12/19.
//

#include "scan_assets.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/season.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/gui/widgets/scan_assets/character_scan_category.h>
#include <doodle_lib/gui/widgets/scan_assets/prop_scan_category.h>
#include <doodle_lib/gui/widgets/scan_assets/scene_scan_category.h>
namespace doodle::gui {

namespace details {

class scan_assets_config {
 public:
  // 扫瞄资产的根目录
  std::vector<scan_category_t::project_root_t> project_roots_;
  // 扫瞄分类

  std::vector<std::unique_ptr<scan_category_t>> scan_categorys_;
  std::vector<scan_category_data_ptr> scam_datas_;
  std::map<uuid, entt::entity> assets_file_map_;

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
        assets_file_map_{} {}

  void start_sacn() {
    for (auto&& l_root : project_roots_) {
      for (auto&& l_data : scan_categorys_) {
        scam_datas_ |= ranges::actions::push_back(l_data->scan(l_root));
      }
    }
  }
};

}  // namespace details

void scan_assets_t::init_scan_categories() {
  project_roots_ = {
      project_root_gui_t{"//192.168.10.250/public/DuBuXiaoYao_3", "独步逍遥", true},
      {"//192.168.10.240/public/renjianzuideyi", "人间最得意", true},
      {"//192.168.10.240/public/WuJinShenYu", "无尽神域", true},
      {"//192.168.10.240/public/WuDiJianHun", "无敌剑魂", true},
      {"//192.168.10.240/public/WanGuShenHua", "万古神话", true},
      {"//192.168.10.240/public/LianQiShiWanNian", "炼气十万年", true},
      {"//192.168.10.240/public/WGXD", "万古邪帝", true},
      {"//192.168.10.240/public/LongMaiWuShen", "龙脉武神", true},
      {"//192.168.10.218/WanYuFengShen", "万域封神", true}
  };
  scan_categories_factory_vec_ = {
      scan_categories_factory_t{
          gui_cache_name_id{"扫描角色"}, true,
          []() -> std::shared_ptr<details::scan_category_t> {
            return std::make_shared<details::character_scan_category_t>();
          }
      },
      scan_categories_factory_t{
          gui_cache_name_id{"扫描场景"}, true,
          []() -> std::shared_ptr<details::scan_category_t> {
            return std::make_shared<details::scene_scan_category_t>();
          }
      },
      scan_categories_factory_t{
          gui_cache_name_id{"扫描道具"}, true,
          []() -> std::shared_ptr<details::scan_category_t> {
            return std::make_shared<details::prop_scan_category_t>();
          }
      }
  };
}

void scan_assets_t::create_scan_categories() {
  scan_categories_.clear();
  for (auto&& l_factory : scan_categories_factory_vec_) {
    if (!l_factory.has_) continue;
    scan_categories_.emplace_back(l_factory.factory_());
  }
}

void scan_assets_t::start_scan() {
  scam_data_vec_.clear();
  for (auto&& l_root : project_roots_) {
    if (!l_root.has_) continue;
    for (auto&& l_data : scan_categories_) {
      scam_data_vec_ |= ranges::actions::push_back(l_data->scan(l_root));
    }
  }
}

bool scan_assets_t::render() {
  bool l_changed{};

  if (auto l_table = dear::Table{"项目列表", 3}; l_table) {
    for (auto&& l_root : project_roots_) {
      ImGui::TableNextColumn();
      if (ImGui::Checkbox(l_root.name_.c_str(), &l_root.has_)) {
        l_changed = true;
      }
    }
  }

  for (auto&& l_factory : scan_categories_factory_vec_) {
    if (ImGui::Checkbox(*l_factory.name_id_, &l_factory.has_)) {
      l_changed = true;
    }
    ImGui::SameLine();
  }
  ImGui::NewLine();
  if (l_changed) {
    create_scan_categories();
  }

  if (ImGui::Button(*start_scan_id)) {
    start_scan();
  }

  if (auto l_table = dear::Table{*assets_table_id_, boost::numeric_cast<std::int32_t>(assets_table_header_.size())};
      l_table) {
    for (auto&& l_header : assets_table_header_) {
      ImGui::TableSetupColumn(l_header.c_str());
    }
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper{};
    clipper.Begin(boost::numeric_cast<std::int32_t>(assets_table_data_.size()));
    while (clipper.Step()) {
      for (auto l_i = clipper.DisplayStart; l_i < clipper.DisplayEnd; ++l_i) {
        std::size_t l_index{boost::numeric_cast<std::size_t>(l_i)};
        auto&& i = assets_table_data_[l_index];
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        dear::Text(i.name_);
        ImGui::TableNextColumn();
        dear::Text(i.season_);
        ImGui::TableNextColumn();
        dear::Text(i.version_name_);
        ImGui::TableNextColumn();
        dear::Text(i.ue_path_);
        ImGui::TableNextColumn();
        dear::Text(i.maya_rig_path_);
        ImGui::TableNextColumn();
        dear::Text(i.project_root_);
        ImGui::TableNextColumn();
        dear::Text(i.info_);
      }
    }
  }

  return is_open;
}
}  // namespace doodle::gui