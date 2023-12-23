//
// Created by TD on 2023/12/19.
//

#pragma once
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/gui/widgets/scan_assets/base.h>

namespace doodle::gui {
namespace details {
class scan_assets_config;
}
class DOODLELIB_API scan_assets_t {
  bool is_open{true};

  struct project_root_gui_t : details::scan_category_t::project_root_t {
    bool has_;
  };
  // 工厂
  struct scan_categories_factory_t {
    gui_cache_name_id name_id_;
    bool has_;
    std::function<std::shared_ptr<details::scan_category_t>()> factory_;
  };

  // 开始扫瞄资产
  gui_cache_name_id start_scan_id;
  std::shared_ptr<details::scan_assets_config> config_;
  std::vector<details::scan_category_data_ptr> scam_data_vec_;
  std::vector<std::shared_ptr<details::scan_category_t>> scan_categories_;
  std::array<project_root_gui_t, 9> project_roots_;
  std::array<scan_categories_factory_t, 3> scan_categories_factory_vec_;

  // 扫描后gui数据结构
  struct scan_gui_data_t {
    std::string name_;           // 名称
    std::string season_;         // 所属季数
    std::string version_name_;   // 版本名称
    std::string ue_path_;        // 主要的ue路径
    std::string maya_rig_path_;  // 对应的maya rig路径
    std::string project_root_;   // 项目根目录 + 项目名称
    std::string info_;           // 信息
  };
  // 资产表
  gui_cache_name_id assets_table_id_;
  std::vector<scan_gui_data_t> assets_table_data_;  // 资产表主要数据
  // 资产表头
  std::array<std::string, 7> assets_table_header_{"名称",         "季数",       "版本名称", "ue路径",
                                                  "maya rig路径", "项目根目录", "信息"};

  void start_scan();
  void init_scan_categories();
  void create_scan_categories();

 public:
  scan_assets_t() : start_scan_id{"开始扫瞄资产"}, project_roots_{}, scan_categories_factory_vec_{} {
    init_scan_categories();
  };

  constexpr static std::string_view name{gui::config::menu_w::scan_assets};
  bool render();
};

}  // namespace doodle::gui
