#include "work_hour_filling.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/metadata/user.h"

#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include "gui/widgets/work_hour_filling.h"
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <imgui.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::gui {

class work_hour_filling::impl {
 public:
  std::string title{};

  std::vector<entt::handle> work_list;
  entt::handle current_user;
  gui_cache<std::int32_t> time_combox{"月份", 1};
};

work_hour_filling::work_hour_filling() : ptr(std::make_unique<impl>()) { ptr->title = std::string{name}; }

void work_hour_filling::init() {
  ptr->current_user  = g_reg()->ctx().at<doodle::user::current_user>().get_handle();
  ptr->time_combox() = time_point_wrap{}.compose().month;
}

const std::string& work_hour_filling::title() const { return ptr->title; }

void work_hour_filling::render() {
  ImGui::Text("基本信息:");

  ImGui::InputInt(*ptr->time_combox, &ptr->time_combox);

  ImGui::Text("工时信息");

  dear::Table{"", 7};
}

work_hour_filling::~work_hour_filling() = default;

}  // namespace doodle::gui