//
// Created by td_main on 2023/6/9.
//

#pragma once

#include "doodle_app/gui/base/ref_base.h"

#include "doodle_lib_fwd.h"

#include "entt/entity/fwd.hpp"
#include <cstdint>
#include <string>
namespace doodle::gui::render {

class time_edit_t {
  entt::handle render_id{};

  void init(const entt::handle& in_handle);
  void init(const time_point_wrap& in_handle);

  gui_cache_name_id time_ymd_id{"年月日"s};
  gui_cache_name_id time_hms_id{"时分秒"s};
  gui_cache_name_id add{"添加时间"s};
  std::array<std::int32_t, 3> time_ymd{};
  std::array<std::int32_t, 3> time_hms{};

 public:
  bool render(const entt::handle& in_handle_view);
  bool render(const time_point_wrap& in_point_wrap, const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
