//
// Created by td_main on 2023/6/9.
//

#include "time_edit.h"

#include "doodle_core/metadata/time_point_wrap.h"

#include "imgui.h"

namespace doodle::gui::render {
bool time_edit_t::render(const entt::handle& in_handle_view) {
  bool on_change{false};

  if (!in_handle_view.all_of<time_point_wrap>()) {
    if (ImGui::Button(*add)) {
      in_handle_view.emplace<time_point_wrap>();
      on_change = true;
    }
  } else {
    if (in_handle_view != render_id) {
      init(in_handle_view);
    }
    if (ImGui::InputInt3(*time_ymd_id, time_ymd.data())) on_change = true;
    if (ImGui::InputInt3(*time_hms_id, time_hms.data())) on_change = true;
  }
  return on_change;
}
void time_edit_t::init(const entt::handle& in_handle) {
  auto& l_time_point_wrap = in_handle.get<time_point_wrap>();
  init(l_time_point_wrap);
  render_id = in_handle;
}
bool time_edit_t::render(const time_point_wrap& in_point_wrap, const entt::handle& in_handle_view) {
  bool on_change{false};

  if (in_handle_view != render_id) {
    init(in_point_wrap);
    render_id = in_handle_view;
  }
  if (ImGui::InputInt3(*time_ymd_id, time_ymd.data())) on_change = true;
  if (ImGui::InputInt3(*time_hms_id, time_hms.data())) on_change = true;
  return on_change;
}
void time_edit_t::init(const time_point_wrap& in_handle) {
  auto l_com  = in_handle.compose();
  time_ymd[0] = boost::pfr::get<0>(l_com);
  time_ymd[1] = boost::pfr::get<1>(l_com);
  time_ymd[2] = boost::pfr::get<2>(l_com);
  time_hms[0] = boost::pfr::get<3>(l_com);
  time_hms[1] = boost::pfr::get<4>(l_com);
  time_hms[2] = boost::pfr::get<5>(l_com);
}
}  // namespace doodle::gui::render