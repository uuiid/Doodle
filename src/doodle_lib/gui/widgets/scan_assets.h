//
// Created by TD on 2023/12/19.
//

#pragma once
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/core/scan_assets/base.h>
#include <doodle_lib/core/scan_assets/scan_category_service.h>

#include <imgui.h>
namespace doodle::gui {

class DOODLELIB_API scan_assets_t {
  bool is_open{true};
  template <class Mutex>
  friend class scan_sink_t;
  struct logger_data_t {
    std::mutex mutex_;
    std::string data_;

    void clear() {
      std::lock_guard const _lock{mutex_};
      data_.clear();
    }
  };
  using logger_data_ptr = std::shared_ptr<logger_data_t>;

  logger_data_ptr logger_data_;

  logger_ptr logger_;
  struct project_root_gui_t : doodle::details::scan_category_t::project_root_t {
    bool has_;
  };
  // 工厂
  struct scan_categories_factory_t {
    gui_cache_name_id name_id_;
    bool has_;
    std::function<std::shared_ptr<doodle::details::scan_category_t>()> factory_;
  };

  // 开始扫瞄资产
  gui_cache_name_id start_scan_id;
  std::vector<doodle::details::scan_category_data_ptr> scam_data_vec_;
  std::vector<std::shared_ptr<doodle::details::scan_category_t>> scan_categories_;
  std::vector<project_root_gui_t> project_roots_;
  std::array<scan_categories_factory_t, 3> scan_categories_factory_vec_;
  std::vector<std::shared_ptr<std::atomic_bool>> scan_categories_is_scan_;

  // 扫描后gui数据结构
  struct scan_gui_data_t {
    std::string name_;          // 名称
    std::string season_;        // 所属季数
    std::string version_name_;  // 版本名称
    std::string ue_path_;       // 主要的ue路径
    ImVec4 ue_path_color_;      // 主要的ue路径颜色

    std::string maya_rig_path_;   // 对应的maya rig路径
    ImVec4 maya_rig_path_color_;  // 对应的maya rig路径颜色

    gui_cache_name_id base_path_show;  // 基础路径
    FSys::path base_path_;             // 基础路径

    std::string project_root_;  // 项目根目录 + 项目名称
    std::string info_;          // 信息
  };
  // 资产表
  gui_cache_name_id assets_table_id_;
  std::vector<scan_gui_data_t> assets_table_data_;  // 资产表主要数据
  // 资产表头
  std::array<std::tuple<std::string, std::float_t>, 8> assets_table_header_{
      std::tuple<std::string, std::float_t>{"名称", 30.0f},
      std::tuple<std::string, std::float_t>{"季数", 30.0f},
      std::tuple<std::string, std::float_t>{"版本名称", 30.0f},
      std::tuple<std::string, std::float_t>{"基本路径", 30.0f},
      std::tuple<std::string, std::float_t>{"ue路径", 130.0f},
      std::tuple<std::string, std::float_t>{"maya rig路径", 130.0f},
      std::tuple<std::string, std::float_t>{"项目根目录", 30.0f},
      std::tuple<std::string, std::float_t>{"信息", 30.0f}
  };

  void start_scan();
  void init_scan_categories();
  void create_scan_categories();

  void create_assets_table_data();
  void append_assets_table_data(const std::vector<doodle::details::scan_category_data_ptr>& in_data);

 public:
  scan_assets_t() : start_scan_id{"开始扫瞄资产"}, project_roots_{}, scan_categories_factory_vec_{} {
    init_scan_categories();
  };

  constexpr static std::string_view name{gui::config::menu_w::scan_assets};
  bool render();

 private:
  std::string log_str_;
};

}  // namespace doodle::gui
