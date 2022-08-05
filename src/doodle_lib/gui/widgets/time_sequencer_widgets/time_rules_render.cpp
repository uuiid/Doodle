//
// Created by TD on 2022/8/4.
//

#include "time_rules_render.h"

#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/detail/time_point_info.h>

#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_lib/gui/gui_ref/ref_base.h>
namespace doodle {
namespace gui {
namespace time_sequencer_widget_ns {

namespace {
class work_gui_data {
 public:
  using base_type = gui_cache<std::array<gui_cache<bool>, 7>>;
  base_type gui_attr;
  work_gui_data()
      : work_gui_data(business::rules::work_Monday_to_Friday){};
  explicit work_gui_data(const business::rules::work_day_type& in_work_day_type)
      : gui_attr(
            "工作周"s,
            std::array<gui::gui_cache<bool>, 7>{gui::gui_cache<bool>{"星期日"s, in_work_day_type[0]},
                                                gui::gui_cache<bool>{"星期一"s, in_work_day_type[1]},
                                                gui::gui_cache<bool>{"星期二"s, in_work_day_type[2]},
                                                gui::gui_cache<bool>{"星期三"s, in_work_day_type[3]},
                                                gui::gui_cache<bool>{"星期四"s, in_work_day_type[4]},
                                                gui::gui_cache<bool>{"星期五"s, in_work_day_type[5]},
                                                gui::gui_cache<bool>{"星期六"s, in_work_day_type[6]}}){};

  explicit operator business::rules::work_day_type() {
    business::rules::work_day_type l_r{};
    l_r[0] = gui_attr.data[0]();
    l_r[1] = gui_attr.data[1]();
    l_r[2] = gui_attr.data[2]();
    l_r[3] = gui_attr.data[3]();
    l_r[4] = gui_attr.data[4]();
    l_r[5] = gui_attr.data[5]();
    l_r[6] = gui_attr.data[6]();
    return l_r;
  }
};

class time_hh_mm_ss_gui_data : public gui_cache<std::array<std::int32_t, 3>> {
 public:
  using base_type = gui_cache<std::array<std::int32_t, 3>>;
  //  explicit time_hh_mm_ss_gui_data(std::int32_t in_h,
  //                                  std::int32_t in_m,
  //                                  std::int32_t in_s)
  //      : base_type("时分秒",
  //                  std::array<std::int32_t, 3>{in_h, in_m, in_s}){};
  time_hh_mm_ss_gui_data()
      : base_type("时分秒",
                  std::array<std::int32_t, 3>{}){

        };
  explicit time_hh_mm_ss_gui_data(const chrono::seconds& in_seconds)
      : time_hh_mm_ss_gui_data() {
    chrono::hh_mm_ss l_hh_mm_ss{in_seconds};
    data[0] = boost::numeric_cast<std::int32_t>(l_hh_mm_ss.hours().count());
    data[1] = boost::numeric_cast<std::int32_t>(l_hh_mm_ss.minutes().count());
    data[2] = boost::numeric_cast<std::int32_t>(l_hh_mm_ss.seconds().count());
  };

  explicit operator chrono::seconds() {
    return chrono::hours{data[0]} + chrono::minutes{data[1]} + chrono::seconds{data[2]};
  }
};
class time_yy_mm_dd_gui_data : public gui_cache<std::array<std::int32_t, 3>> {
 public:
  using base_type = gui_cache<std::array<std::int32_t, 3>>;
  time_yy_mm_dd_gui_data()
      : base_type("年月日"s,
                  std::array<std::int32_t, 3>{}){};
  explicit time_yy_mm_dd_gui_data(const chrono::local_days& in_days)
      : time_yy_mm_dd_gui_data() {
    chrono::year_month_day l_year_month_day{in_days};
    data[0] = boost::numeric_cast<std::int32_t>((std::int32_t)l_year_month_day.year());
    data[1] = boost::numeric_cast<std::int32_t>((std::uint32_t)l_year_month_day.month());
    data[2] = boost::numeric_cast<std::int32_t>((std::uint32_t)l_year_month_day.day());
  };

  explicit operator chrono::local_days() {
    return chrono::local_days{
        chrono::year_month_day{
            chrono::year{data[0]},
            chrono::month{boost::numeric_cast<std::uint32_t>(data[1])},
            chrono::day{boost::numeric_cast<std::uint32_t>(data[2])}}};
  }
};

class time_warp_gui_data
    : public gui_cache<std::pair<time_yy_mm_dd_gui_data, time_hh_mm_ss_gui_data>> {
 public:
  using base_type = gui_cache<std::pair<time_yy_mm_dd_gui_data, time_yy_mm_dd_gui_data>>;
  time_warp_gui_data() : time_warp_gui_data(time_point_wrap{}){};
  explicit time_warp_gui_data(const time_point_wrap& in_point_wrap)
      : time_warp_gui_data("时间"s, in_point_wrap){};
  explicit time_warp_gui_data(const std::string& in_string, const time_point_wrap& in_point_wrap) {
    this->gui_name    = gui_cache_name_id{in_string};
    auto&& [l_y, l_s] = in_point_wrap.compose_1();
    data.first        = time_yy_mm_dd_gui_data{l_y};
    data.second       = time_hh_mm_ss_gui_data{l_s};
  };
  explicit operator time_point_wrap() {
    return time_point_wrap{chrono::local_days{data.first} + chrono::seconds{data.second}};
  }
};

class time_info_gui_data
    : public gui_cache<std::pair<time_warp_gui_data, time_warp_gui_data>> {
 public:
  gui_cache<std::string> info;
  gui_cache<bool> delete_node{"删除"s, false};

  time_info_gui_data() : time_info_gui_data(::doodle::business::rules::point_type{}) {}
  explicit time_info_gui_data(const ::doodle::business::rules::point_type& in_point_type)
      : gui_cache<std::pair<time_warp_gui_data, time_warp_gui_data>>(
            "时间"s),
        info("备注"s, in_point_type.info) {
    data.first  = time_warp_gui_data{"开始时间"s, in_point_type.first};
    data.second = time_warp_gui_data{"结束时间"s, in_point_type.second};
  };

  explicit operator ::doodle::business::rules::point_type() {
    return ::doodle::business::rules::point_type{
        time_point_wrap{data.first},
        time_point_wrap{data.second},
        info()};
  }
};

class time_work_gui_data {
 public:
  gui_cache<time_hh_mm_ss_gui_data> begin;
  gui_cache<time_hh_mm_ss_gui_data> end;
  time_work_gui_data() = default;
};

}  // namespace

class time_rules_render::impl {
 public:
  rules_type rules_attr;
  work_gui_data work_gui_data_attr{};
};

time_rules_render::time_rules_render()
    : p_i(std::make_unique<impl>()) {
  p_i->rules_attr = g_reg()->ctx().emplace<rules_type>();
}
const time_rules_render::rules_type& time_rules_render::rules_attr() const {
  return p_i->rules_attr;
}
void time_rules_render::rules_attr(const time_rules_render::rules_type& in_rules_type) {
  p_i->rules_attr = in_rules_type;
}
bool time_rules_render::render() {
  return false;
}
time_rules_render::~time_rules_render() = default;
}  // namespace time_sequencer_widget_ns
}  // namespace gui
}  // namespace doodle
