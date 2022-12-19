#include "work_hour_filling.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/metadata/work_task.h"

#include "doodle_app/gui/base/base_window.h"
#include "doodle_app/gui/base/modify_guard.h"
#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/gui/show_message.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include <boost/asio/post.hpp>
#include <boost/lambda2/lambda2.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/operators.hpp>

#include "gui/widgets/derail/all_user_combox.h"
#include "gui/widgets/work_hour_filling.h"
#include <__msvc_chrono.hpp>
#include <array>
#include <chrono>
#include <cstdint>
#include <date/date.h>
#include <date/tz.h>
#include <entt/entity/fwd.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <map>
#include <memory>
#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/merge.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/cache1.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/take.hpp>
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

  // explicit table_line(const time_point_wrap& in_task)
  //     : cache_time{chrono::round<chrono::hours>(in_task.get_local_time())},
  //       time_day{fmt::format("{:%F}", cache_time)},
  //       week{fmt::format("星期 {:%w}", cache_time)},
  //       am_or_pm{fmt::format("{:%p}", cache_time)},
  //       task{"##task"s},
  //       region{"##region"s},
  //       abstract{"##abstract"s} {}

  explicit table_line(const chrono::local_time<chrono::hours>& in_task)
      : cache_time{in_task},
        time_day{fmt::format("{:%F}", cache_time)},
        week{fmt::format("星期 {:%w}", cache_time)},
        am_or_pm{fmt::format("{:%p}", cache_time)},
        task{"##task"s},
        region{"##region"s},
        abstract{"##abstract"s} {}

  explicit operator work_task_info() const { return work_task_info{user_handle, cache_time, task, region, abstract}; }

  bool operator==(const table_line& in) const { return cache_time == in.cache_time; }
  bool operator<(const table_line& in) const { return cache_time < in.cache_time; }

  chrono::local_time<chrono::hours> cache_time{};

  std::string time_day{};
  std::string week{};
  std::string am_or_pm{};
  gui_cache<std::string> task{"##task"s};
  gui_cache<std::string> region{"##region"s};
  gui_cache<std::string> abstract{"##abstract"s};
  entt::handle user_handle;
};
}  // namespace

class work_hour_filling::impl {
 public:
  std::string title{};

  std::vector<entt::handle> work_list;
  std::map<chrono::local_time<chrono::hours>, entt::handle> time_cache;
  entt::handle current_user;
  gui_cache<std::array<std::int32_t, 2>> time_month{"年.月", std::int32_t{1}, std::int32_t{1}};
  gui_cache_name_id table{"工时信息"};
  std::vector<table_line> table_list;
  const std::array<std::string, 6> table_head{"日期"s, "星期"s, "时段"s, "项目"s, "地区"s, "工作内容摘要"s};

  gui_cache_name_id advanced_setting{"高级设置"};

  all_user_combox combox{true};
};

work_hour_filling::work_hour_filling() : ptr(std::make_unique<impl>()) {
  ptr->title        = std::string{name};
  ptr->current_user = g_reg()->ctx().at<doodle::user::current_user>().get_handle();
}

void work_hour_filling::list_time(std::int32_t in_y, std::int32_t in_m) {
  using work_tub_t  = decltype(*g_reg()->view<work_task_info>().each().begin());

  auto l_time       = time_point_wrap{in_y, in_m, 1};
  auto l_begin_time = chrono::time_point_cast<chrono::hours>(
      chrono::clock_cast<chrono::local_t>(l_time.current_month_start().get_sys_time())
  );
  auto l_end_time = chrono::time_point_cast<chrono::hours>(
      chrono::clock_cast<chrono::local_t>(l_time.current_month_end().get_sys_time())
  );
  /// 先生成
  ptr->time_cache.clear();
  for (auto i = l_begin_time; i <= l_end_time; i += chrono::hours{12}) {
    // DOODLE_LOG_INFO("生成时间 {}", i);
    ptr->time_cache[i] = {};
  }
  for (auto&& [l_e, l_w] : g_reg()->view<work_task_info>().each()) {
    if (l_w.time >= l_begin_time && l_w.time <= l_end_time && l_w.user_ref.user_attr() == ptr->current_user) {
      // DOODLE_LOG_INFO("时间 {} 信息 {}", l_w.time, l_w.task_name);
      // auto l_t = make_handle(l_e).get<work_task_info>();
      // DOODLE_LOG_INFO("句柄时间 {} 信息 {}", l_t.time, l_t.task_name);
      ptr->time_cache[l_w.time] = make_handle(l_e);
    }
  }

  ptr->table_list =
      ptr->time_cache | ranges::views::transform([&](const decltype(ptr->time_cache)::value_type& in) -> table_line {
        // if (in.second && in.second.any_of<work_task_info>()) {
        //   auto l_t = in.second.get<work_task_info>();
        //   DOODLE_LOG_INFO("原始时间 {} 信息 {}", l_t.time, l_t.task_name);
        // }
        auto l_line = (in.second && in.second.any_of<work_task_info>()) ? table_line{in.second.get<work_task_info>()}
                                                                        : table_line{in.first};
        // DOODLE_LOG_INFO("时间 {} 信息 {}", l_line.time_day, l_line.task());
        l_line.user_handle = ptr->current_user;
        return l_line;
      }) |
      ranges::to_vector;
  /// 排序
  ptr->table_list |= ranges::actions::sort(boost::lambda2::_1 < boost::lambda2::_2);
}

void work_hour_filling::modify_item(std::size_t in_index) {
  auto&& l_i = ptr->table_list[in_index];

  DOODLE_LOG_INFO("编辑时间 {}", l_i.cache_time);
  if (!ptr->time_cache[l_i.cache_time]) ptr->time_cache[l_i.cache_time] = make_handle();
  auto l_h      = ptr->time_cache[l_i.cache_time];

  auto&& l_task = l_h.get_or_emplace<work_task_info>() = (work_task_info)l_i;
  if (!l_h.all_of<database>()) l_h.emplace<database>();
  database::save(l_h);
}

void work_hour_filling::init() {
  auto l_time       = time_point_wrap{}.compose();
  ptr->time_month() = {l_time.year, l_time.month};
  list_time(l_time.year, l_time.month);
}

const std::string& work_hour_filling::title() const { return ptr->title; }

void work_hour_filling::render() {
  ImGui::Text("基本信息:");

  if (ImGui::InputInt2(*ptr->time_month, ptr->time_month().data())) {
    list_time(ptr->time_month()[0], ptr->time_month()[1]);
  };

  dear::TreeNode{*ptr->advanced_setting} && [&]() {
    /// 打开新的窗口显示用户
    if (ptr->combox.render()) {
      auto l_u      = std::make_shared<work_hour_filling>();
      auto l_user_h = l_u->ptr->current_user = ptr->combox.get_user();
      l_u->ptr->title = l_user_h.get<user>().get_name().empty() ? fmt::format("匿名用户 {}", l_user_h)
                                                                : l_user_h.get<user>().get_name();
      boost::asio::post([l_u]() { make_handle().emplace<gui_windows>(l_u); });
    }
  };

  ImGui::Text("工时信息");

  dear::Table{
      *ptr->table, boost::numeric_cast<std::int32_t>(ptr->table_head.size()),
      ImGuiTableFlags_::ImGuiTableFlags_RowBg} &&
      [&]() {
        ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
        ranges::for_each(ptr->table_head, [](const std::string& in) { ImGui::TableSetupColumn(in.c_str()); });
        ImGui::TableHeadersRow();

        for (auto i = 0ull; i < ptr->table_list.size(); ++i) {
          auto&& in = ptr->table_list[i];
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          dear::Text(in.time_day);
          ImGui::TableNextColumn();
          dear::Text(in.week);
          ImGui::TableNextColumn();
          dear::Text(in.am_or_pm);

          ImGui::TableNextColumn();
          if (dear::InputText(*in.task, &in.task)) modify_item(i);
          ImGui::TableNextColumn();
          if (dear::InputText(*in.region, &in.region)) modify_item(i);
          ImGui::TableNextColumn();
          if (dear::InputText(*in.abstract, &in.abstract)) modify_item(i);
        }
      };
}

work_hour_filling::~work_hour_filling() = default;

}  // namespace doodle::gui