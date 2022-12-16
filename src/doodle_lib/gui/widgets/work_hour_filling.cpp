#include "work_hour_filling.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/metadata/work_task.h"

#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include <boost/lambda2/lambda2.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/operators.hpp>

#include "gui/widgets/work_hour_filling.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <date/date.h>
#include <entt/entity/fwd.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <memory>
#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/merge.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace doodle::gui {

namespace {
struct table_line : boost::totally_ordered<table_line> {
  table_line() = default;

  explicit table_line(const work_task_info& in_task)
      : cache_time{in_task.time},
        time_day{fmt::format("{:%F}", cache_time)},
        week{fmt::format("星期 {:%w}", cache_time)},
        am_or_pm{fmt::format("{:%p}", cache_time)},
        task{"##task"s, in_task.task_name},
        region{"##region"s, in_task.region},
        abstract{"##abstract"s, in_task.abstract} {}

  explicit table_line(time_point_wrap in_task)
      : cache_time{std::move(in_task)},
        time_day{fmt::format("{:%F}", cache_time)},
        week{fmt::format("星期 {:%w}", cache_time)},
        am_or_pm{fmt::format("{:%p}", cache_time)},
        task{"##task"s, ""s},
        region{"##region"s, ""s},
        abstract{"##abstract"s, ""s} {}

  explicit operator work_task_info() const { return {cache_time, task, region, abstract}; }

  bool operator==(const table_line& in) const { return cache_time == in.cache_time; }
  bool operator<(const table_line& in) const { return cache_time < in.cache_time; }

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
  gui_cache<std::array<std::int32_t, 2>> time_month{"年.月", std::int32_t{1}, std::int32_t{1}};
  gui_cache_name_id table{"工时信息"};
  std::vector<table_line> table_list;
  const std::array<std::string, 6> table_head{"日期"s, "星期"s, "时段"s, "项目"s, "地区"s, "工作内容摘要"s};
};

work_hour_filling::work_hour_filling() : ptr(std::make_unique<impl>()) { ptr->title = std::string{name}; }

void work_hour_filling::list_time(std::int32_t in_y, std::int32_t in_m) {
  using work_tub_t  = decltype(*g_reg()->view<work_task_info>().each().begin());
  auto l_list       = g_reg()->view<work_task_info>().each();
  auto l_time       = time_point_wrap{in_y, in_m, 1};
  auto l_begin_time = l_time.current_month_start();
  auto l_end_time   = l_time.current_month_end();
  std::vector<table_line> l_time_list{};
  /// 先生成
  for (auto i = l_begin_time; i <= l_end_time; i += chrono::hours{12}) {
    l_time_list.emplace_back(i);
  }

  /// 先过滤转换
  auto l_find_time_list = l_list | ranges::views::filter([&](const work_tub_t& in) -> bool {
                            auto&& [l_e, l_w] = in;
                            auto l_h          = make_handle(l_e);
                            if (ptr->current_user)
                              return l_w.time > l_begin_time && l_w.time < l_end_time && l_h.all_of<user>() &&
                                     l_h.get<user>() == ptr->current_user.get<user>();
                            else
                              return l_w.time > l_begin_time && l_w.time < l_end_time;
                          }) |
                          ranges::views::transform([](const work_tub_t& in) -> table_line {
                            auto&& [l_e, l_w] = in;
                            return table_line{l_w};
                          }) |
                          ranges::to_vector;
  /// 排序
  l_find_time_list |= ranges::actions::sort(boost::lambda2::_1 < boost::lambda2::_2);
  // 补齐
  ptr->table_list.resize(l_time_list.size() + l_find_time_list.size());
  ranges::merge(l_time_list, l_find_time_list, ptr->table_list.begin());
  ptr->table_list |= ranges::actions::unique(boost::lambda2::_1 == boost::lambda2::_2);
}

void work_hour_filling::init() {
  ptr->current_user = g_reg()->ctx().at<doodle::user::current_user>().get_handle();
  auto l_time       = time_point_wrap{}.compose();
  ptr->time_month() = {l_time.year, l_time.month};
}

const std::string& work_hour_filling::title() const { return ptr->title; }

void work_hour_filling::render() {
  ImGui::Text("基本信息:");

  if (ImGui::InputInt2(*ptr->time_month, ptr->time_month().data())) {
    list_time(ptr->time_month()[0], ptr->time_month()[1]);
  };

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