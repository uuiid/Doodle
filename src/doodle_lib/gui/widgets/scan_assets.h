//
// Created by TD on 2023/12/19.
//

#pragma once
#include <doodle_app/gui/base/ref_base.h>

namespace doodle::gui {

class DOODLELIB_API scan_assets_t {
  bool is_open{true};

  // 开始扫瞄资产
  gui_cache_name_id start_scan_id;

  void start_scan();

 public:
  scan_assets_t() : start_scan_id{"开始扫瞄资产"} {};

  constexpr static std::string_view name{gui::config::menu_w::scan_assets};
  bool render();
};

}  // namespace doodle::gui
