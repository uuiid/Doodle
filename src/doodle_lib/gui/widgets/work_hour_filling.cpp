#include "work_hour_filling.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/metadata/work_task.h"

#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include <boost/numeric/conversion/cast.hpp>

#include "gui/widgets/work_hour_filling.h"
#include <array>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <memory>
#include <range/v3/algorithm/for_each.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::gui {

namespace {
struct table_line {
  table_line() = default;

  explicit table_line(const work_task_info& in_task)
      : cache_time{in_task.time},
        time_day{fmt::format("{:%F}", cache_time)},
        week{fmt::format("星期 {:%w}", cache_time)},
        am_or_pm{fmt::format("星期 {:%p}", cache_time)},
        task{"##task"s, in_task.task_name},
        region{"##region"s, in_task.region},
        abstract{"##abstract"s, in_task.abstract} {}

  explicit operator work_task_info() const { return {cache_time, task, region, abstract}; }
  time_point_wrap cache_time{};

  std::string time_day{};
  std::string week{};
  std::string am_or_pm{};
  gui_cache<std::string> task{"##task"s, ""s};
  gui_cache<std::string> region{"##region"s, ""s};
  gui_cache<std::string> abstract{"##abstract"s, ""s};
};
}  // namespace

class work_hour_filling::impl {
 public:
  std::string title{};

  std::vector<entt::handle> work_list;
  entt::handle current_user;
  gui_cache<std::int32_t> time_combox{"月份", 1};
  gui_cache_name_id table{"工时信息"};
  std::vector<table_line> table_list;
  const std::array<std::string, 6> table_head{"日期"s, "星期"s, "时段"s, "项目"s, "地区"s, "工作内容摘要"s};
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

  dear::Table{*ptr->table, boost::numeric_cast<std::int32_t>(ptr->table_head.size())} && [&]() {
    ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
    ranges::for_each(ptr->table_head, [](const std::string& in) { ImGui::TableSetupColumn(in.c_str()); });
    ImGui::TableHeadersRow();
    ranges::for_each(ptr->table_list, [](table_line& in) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      dear::Text(in.time_day);
      ImGui::TableNextColumn();
      dear::Text(in.week);
      ImGui::TableNextColumn();
      dear::Text(in.am_or_pm);

      ImGui::TableNextColumn();
      dear::InputText(*in.task, &in.task);
      ImGui::TableNextColumn();
      dear::InputText(*in.region, &in.region);
      ImGui::TableNextColumn();
      dear::InputText(*in.abstract, &in.abstract);
    });
  };
}

work_hour_filling::~work_hour_filling() = default;

}  // namespace doodle::gui