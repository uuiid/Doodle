//
// Created by td_main on 2023/7/12.
//

#pragma once

#include "doodle_core/configure/static_value.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/image_icon.h>

#include "doodle_app/gui/base/ref_base.h"

#include <string>

namespace doodle::gui {

class upload_files {
 private:
  std::string title_{name};
  bool show_{true};

  entt::handle upload_file_{};

  gui_cache<std::string> ue_file_{"UE文件"};
  gui_cache<std::string> maya_file_{"Maya 文件"};
  gui_cache<std::string> rig_file_{"Maya 绑定文件"};
  image_icon image_icon_{};

 public:
  /// 窗口显示名称
  constexpr static std::string_view name{gui::config::menu_w::upload_files};

  [[nodiscard("")]] const std::string& title() const;
  /// 每帧刷新函数
  bool render();
};

}  // namespace doodle::gui
