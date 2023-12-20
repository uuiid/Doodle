//
// Created by TD on 2023/12/19.
//

#include "scan_assets.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/season.h>

#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle::gui {

namespace details {

class scan_assets_config {
 public:
  struct project_root_t {
    FSys::path path_;
    std::string name_;
  };
  // 扫瞄资产的根目录
  std::vector<project_root_t> project_roots_;
  // 扫瞄分类

  struct scan_category_t {
    scan_assets_config* config_;
    virtual std::vector<entt::handle> scan(const project_root_t& in_root) const                              = 0;
    virtual std::vector<entt::handle> check_path(const project_root_t& in_root, entt::handle& in_path) const = 0;
  };
  std::vector<std::unique_ptr<scan_category_t>> scan_categorys_;
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
        scan_categorys_{} {}

  /// 路径规范
  /// `项目根目录/6-moxing/BG/JD(季数)_(集数开始)/BG(编号)/(场景名称)/Content/(场景名称)/Map/(场景名称)_(版本).umap`
  struct scene_scan_category_t : scan_category_t {
    // 捕获数据
    struct capture_data_t {
      std::int32_t begin_episode_;
      // 编号
      std::string number_str_;
      // 版本名称
      std::string version_str_;
    };

    std::vector<entt::handle> scan(const project_root_t& in_root) const override {
      const FSys::path l_scene_path = in_root.path_ / "6-moxing/BG";
      const std::regex l_JD_regex{R"(JD(\d+)_(\d+))"};
      const std::regex l_BG_regex{R"(BG(\d+[a-zA-Z]\d*))"};

      if (!FSys::exists(l_scene_path)) {
        return std::vector<entt::handle>();
      }
      std::vector<entt::handle> l_out;
      std::smatch l_match{};

      for (const auto& l_s : FSys::directory_iterator{l_scene_path}) {  // 迭代一级目录
        auto l_name_str = l_s.path().filename().generic_string();
        if (l_s.is_directory() && std::regex_match(l_name_str, l_match, l_JD_regex)) {  // 检查一级目录
          season l_season{std::stoi(l_match[1].str())};
          capture_data_t l_capture_data{std::stoi(l_match[2].str()), ""};
          for (const auto& l_s2 : FSys::directory_iterator{l_s.path()}) {  // 迭代二级目录
            auto l_name2_str = l_s2.path().filename().generic_string();
            if (l_s2.is_directory() && std::regex_match(l_name2_str, l_match, l_BG_regex)) {  // 检查二级目录
              l_capture_data.number_str_ = l_match[1].str();
              for (auto&& l_s3 : FSys::directory_iterator{l_s2.path()}) {  // 迭代三级目录
                if (l_s3.is_directory()) {
                  auto l_dis_path = l_s3.path() / "Content" / l_s3.path().filename() / "Map";  // 确认目标路径
                  if (!FSys::exists(l_dis_path)) continue;
                  for (auto&& l_s4 : FSys::directory_iterator{l_dis_path}) {             // 迭代四级目录
                    if (l_s4.is_regular_file() && l_s4.path().extension() == ".umap") {  // 确认后缀名称
                      auto l_stem = l_s4.path().stem().generic_string();

                      if (l_stem.starts_with(l_s3.path().filename().generic_string()) &&  // 检查文件名和文件格式
                          std::count(l_stem.begin(), l_stem.end(), '_') == 1) {
                        auto l_uuid = FSys::software_flag_file(l_s4.path());
                        if (config_->assets_file_map_.contains(l_uuid)) {
                          l_out.emplace_back(*g_reg(), config_->assets_file_map_.at(l_uuid));
                        } else {
                          assets_file l_assets_file{l_s4.path(), l_s3.path().filename().generic_string(), 0};

                          auto l_handle = entt::handle{*g_reg(), g_reg()->create()};
                          l_handle.emplace<season>(l_season);
                          l_handle.emplace<assets_file>(std::move(l_assets_file));
                          auto l_capture_data_1         = l_capture_data;
                          l_capture_data_1.version_str_ = l_stem.substr(l_stem.find('_') + 1);
                          l_handle.emplace<capture_data_t>(l_capture_data);

                          l_out.push_back(l_handle);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      return l_out;
    }
    /// 检查对于的rig文件和maya文件, maya文件可以不存在, 但是rig文件必须存在
    ///
    /// maya文件(同时也是rig文件):
    ///    项目根目录/6-moxing/BG/JD(季数)_(集数开始)/BG(编号)/Mod/(场景名称)_(版本)_Low.ma

    std::vector<entt::handle> check_path(const project_root_t& in_root, entt::handle& in_path) const override {
      const FSys::path l_scene_path = in_root.path_ / "6-moxing/BG";

      if (!in_path.any_of<season, assets_file>()) return {};
      if (!FSys::exists(l_scene_path)) return {};

      auto& l_season  = in_path.get<season>();
      auto& l_assets  = in_path.get<assets_file>();
      auto& l_data    = in_path.get<capture_data_t>();

      // 检查rig文件
      auto l_rig_path = l_scene_path / fmt::format("JD{:02}_{}", l_season.get_season(), l_data.begin_episode_) /
                        fmt::format("BG{}", l_data.number_str_) / "Mod" /
                        fmt::format("{}_{}_Low.ma", l_assets.name_attr(), l_data.version_str_);
      if (!FSys::exists(l_rig_path)) {
        in_path.destroy();
        return {};
      }

      auto l_rig_assets_file = assets_file{l_rig_path, l_assets.name_attr(), 0};
      entt::handle l_rig_handle{};
      entt::handle l_file_association_handle{};
      auto l_uuid = FSys::software_flag_file(l_rig_path);
      if (config_->assets_file_map_.contains(l_uuid)) {
        l_rig_handle = entt::handle{*g_reg(), config_->assets_file_map_.at(l_uuid)};

      } else {
        l_rig_handle = entt::handle{*g_reg(), g_reg()->create()};
        l_rig_handle.emplace<season>(l_season);
        l_rig_handle.emplace<assets_file>(std::move(l_rig_assets_file));
      }

      if (in_path.any_of<file_association_ref>()) {
        l_file_association_handle = in_path.get<file_association_ref>();
      } else if (l_rig_handle.any_of<file_association_ref>()) {
        l_file_association_handle = l_rig_handle.get<file_association_ref>();
      } else {
        l_file_association_handle = entt::handle{*g_reg(), g_reg()->create()};
      }

      // 创建联系
      file_association l_file_association{};
      l_file_association.ue_file       = in_path;
      l_file_association.maya_file     = l_rig_handle;
      l_file_association.maya_rig_file = l_rig_handle;
      l_file_association_handle.emplace_or_replace<file_association>(std::move(l_file_association));
      l_rig_handle.emplace_or_replace<file_association_ref>(l_file_association_handle);
      in_path.emplace_or_replace<file_association_ref>(l_file_association_handle);
      return {l_rig_handle};
    }
  };

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