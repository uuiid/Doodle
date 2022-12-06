//
// Created by TD on 2022/8/4.
//

#include "time_rules_render.h"

#include <doodle_core/metadata/detail/time_point_info.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>

#include <doodle_app/gui/base/modify_guard.h>
#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <boost/asio.hpp>
#include <boost/optional.hpp>

namespace doodle::gui::time_sequencer_widget_ns {

namespace {
class work_gui_data {
 public:
  using base_type   = gui_cache<std::array<gui_cache<bool>, 7>>;

  using friend_type = business::rules::work_day_type;

  base_type gui_attr;
  work_gui_data() : work_gui_data(business::rules::work_Monday_to_Friday){};
  explicit work_gui_data(const friend_type& in_work_day_type)
      : gui_attr(
            "工作周"s,
            std::array<gui::gui_cache<bool>, 7>{
                gui::gui_cache<bool>{"星期日"s, in_work_day_type[0]},
                gui::gui_cache<bool>{"星期一"s, in_work_day_type[1]},
                gui::gui_cache<bool>{"星期二"s, in_work_day_type[2]},
                gui::gui_cache<bool>{"星期三"s, in_work_day_type[3]},
                gui::gui_cache<bool>{"星期四"s, in_work_day_type[4]},
                gui::gui_cache<bool>{"星期五"s, in_work_day_type[5]},
                gui::gui_cache<bool>{"星期六"s, in_work_day_type[6]}}
        ){};

  explicit operator friend_type() {
    friend_type l_r{};
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
  using base_type   = gui_cache<std::array<std::int32_t, 3>>;

  using friend_type = chrono::seconds;
  time_hh_mm_ss_gui_data()
      : base_type("时分秒", std::array<std::int32_t, 3>{}){

        };
  explicit time_hh_mm_ss_gui_data(const friend_type& in_seconds) : time_hh_mm_ss_gui_data() {
    chrono::hh_mm_ss l_hh_mm_ss{in_seconds};
    data[0] = boost::numeric_cast<std::int32_t>(l_hh_mm_ss.hours().count());
    data[1] = boost::numeric_cast<std::int32_t>(l_hh_mm_ss.minutes().count());
    data[2] = boost::numeric_cast<std::int32_t>(l_hh_mm_ss.seconds().count());
  };

  explicit operator friend_type() {
    return chrono::hours{data[0]} + chrono::minutes{data[1]} + chrono::seconds{data[2]};
  }
};
class time_yy_mm_dd_gui_data : public gui_cache<std::array<std::int32_t, 3>> {
 public:
  using base_type = gui_cache<std::array<std::int32_t, 3>>;

  time_yy_mm_dd_gui_data() : base_type("年月日"s, std::array<std::int32_t, 3>{}){};
  explicit time_yy_mm_dd_gui_data(const chrono::local_days& in_days) : time_yy_mm_dd_gui_data() {
    chrono::year_month_day l_year_month_day{in_days};
    data[0] = boost::numeric_cast<std::int32_t>((std::int32_t)l_year_month_day.year());
    data[1] = boost::numeric_cast<std::int32_t>((std::uint32_t)l_year_month_day.month());
    data[2] = boost::numeric_cast<std::int32_t>((std::uint32_t)l_year_month_day.day());
  };

  explicit operator chrono::local_days() {
    return chrono::local_days{chrono::year_month_day{
        chrono::year{data[0]}, chrono::month{boost::numeric_cast<std::uint32_t>(data[1])},
        chrono::day{boost::numeric_cast<std::uint32_t>(data[2])}}};
  }
};

class time_warp_gui_data : boost::equality_comparable<time_warp_gui_data> {
 public:
  using friend_type = time_point_wrap;
  time_yy_mm_dd_gui_data ymd{};
  time_hh_mm_ss_gui_data hms{};

  time_warp_gui_data() : time_warp_gui_data(friend_type{}){};

  explicit time_warp_gui_data(const friend_type& in_point_wrap) : time_warp_gui_data("时间"s, in_point_wrap){};

  explicit time_warp_gui_data(const std::string& in_string, const friend_type& in_point_wrap) {
    auto&& [l_y, l_s] = in_point_wrap.compose_1();
    ymd               = time_yy_mm_dd_gui_data{l_y};
    hms               = time_hh_mm_ss_gui_data{l_s};
  };
  explicit operator friend_type() { return friend_type{chrono::local_days{ymd} + chrono::seconds{hms}}; }
  bool operator==(const time_warp_gui_data& in) const { return std::tie(ymd, hms) == std::tie(in.ymd, in.hms); }
};

class time_info_gui_data : boost::equality_comparable<time_info_gui_data> {
 public:
  time_warp_gui_data begin_time;
  time_warp_gui_data end_time;
  gui_cache<std::string> info;
  gui_cache_name_id delete_node{"删除"s};

  using friend_type = ::doodle::business::rules::point_type;

  time_info_gui_data() : time_info_gui_data(friend_type{}) {}
  explicit time_info_gui_data(const friend_type& in_point_type)
      : begin_time(), end_time(), info("备注"s, in_point_type.info) {
    begin_time = time_warp_gui_data{"开始时间"s, in_point_type.first};
    end_time   = time_warp_gui_data{"结束时间"s, in_point_type.second};
  };

  explicit operator friend_type() {
    return friend_type{time_point_wrap{begin_time}, time_point_wrap{end_time}, info()};
  }

  bool operator==(const time_info_gui_data& in) const {
    return std::tie(begin_time, end_time, info, delete_node) ==
           std::tie(in.begin_time, in.end_time, in.info, in.delete_node);
  }
};

class time_work_gui_data : boost::equality_comparable<time_work_gui_data> {
 public:
  time_hh_mm_ss_gui_data begin;
  time_hh_mm_ss_gui_data end;
  gui_cache_name_id name_id_delete{"删除"s};
  time_work_gui_data() = default;

  using friend_type    = std::pair<chrono::seconds, chrono::seconds>;

  explicit time_work_gui_data(const friend_type& in_pair) : begin(in_pair.first), end(in_pair.second) {}

  explicit operator friend_type() {
    return std::make_pair(friend_type::first_type{begin}, friend_type::second_type{end});
  }
  bool operator==(const time_work_gui_data& in_rhs) const {
    return begin == in_rhs.begin && end == in_rhs.end && name_id_delete == in_rhs.name_id_delete;
  }
};

class work_gui_data_render {
 public:
  using gui_data_type = work_gui_data;
  modify_guard<gui_data_type::friend_type> modify_guard_;
  gui_data_type gui_data{};
  bool render() {
    modify_guard_.begin_flag();

    dear::CollapsingHeader{*gui_data.gui_attr} && [this]() {
      dear::HelpMarker{"按星期去计算工作时间"};
      ranges::for_each(gui_data.gui_attr(), [this](decltype(gui_data.gui_attr().front()) in_value) {
        modify_guard_ = ImGui::Checkbox(*in_value, &in_value);
      });
    };
    if (modify_guard_) modify_guard_(get());
    return modify_guard_.current_frame_modify();
  }
  gui_data_type::friend_type get() { return gui_data_type::friend_type{gui_data}; }

  void set(const gui_data_type::friend_type& in_type) { gui_data = gui_data_type{in_type}; }
};
class time_work_gui_data_render {
 public:
  using gui_data_type = time_work_gui_data;
  std::vector<gui_data_type> gui_data{};
  gui_cache_name_id name_id{"每日工作时间"};

  gui_cache_name_id name_id_add{"添加"s};
  gui_cache_name_id name_id_delete{"删除"s};
  modify_guard<std::vector<gui_data_type::friend_type>> modify_guard_;

  bool render() {
    if (modify_guard_) modify_guard_(get());

    modify_guard_.begin_flag();

    dear::CollapsingHeader{*name_id} && [this]() {
      dear::HelpMarker{"每天的开始和结束时间段"};
      if (modify_guard_ = imgui::Button(*name_id_add); modify_guard_.current_modify()) {
        boost::asio::post(g_io_context(), [this]() { this->add(); });
      }
      ranges::for_each(gui_data, [this](gui_data_type& in_type) {
        modify_guard_ = ImGui::SliderInt3(*in_type.begin, in_type.begin.data.data(), 0, 59);
        modify_guard_ = ImGui::SliderInt3(*in_type.end, in_type.end.data.data(), 0, 59);
        if (modify_guard_ = imgui::Button(*in_type.name_id_delete); modify_guard_.current_modify()) {
          boost::asio::post(g_io_context(), [this, in_type]() { this->delete_node(in_type); });
        }
      });
    };

    modify_guard_.async_begin_flag();
    return modify_guard_.current_frame_modify();
  };

  void add() {
    gui_data.emplace_back(gui_data_type{std::make_pair(9h, 12h)});
    this->modify_guard_ = true;
  }
  void delete_node(const gui_data_type& in_data_type) {
    gui_data |= ranges::actions::remove_if([&](auto&& in_item) { return in_item == in_data_type; });
    this->modify_guard_ = true;
  }

  std::vector<gui_data_type::friend_type> get() {
    return gui_data | ranges::view::transform([](auto&& in_item) { return gui_data_type::friend_type{in_item}; }) |
           ranges::to_vector;
  }

  void set(const std::vector<gui_data_type::friend_type>& in_type) {
    gui_data =
        in_type | ranges::view::transform([](auto&& in_item) { return gui_data_type{in_item}; }) | ranges::to_vector;
  }
};

class time_info_gui_data_render {
 public:
  using gui_data_type = time_info_gui_data;

  std::vector<time_info_gui_data> gui_data{};

  gui_cache_name_id gui_name{};
  gui_cache_name_id gui_name_add{"添加"s};

  modify_guard<std::vector<gui_data_type::friend_type>> modify_guard_{};

  bool render() {
    if (modify_guard_) modify_guard_(get());

    modify_guard_.begin_flag();

    dear::CollapsingHeader{*gui_name} && [this]() {
      if (modify_guard_ = ImGui::Button(*gui_name_add); modify_guard_.current_modify()) {
        boost::asio::post(g_io_context(), [this]() { this->add(); });
      }
      ranges::for_each(gui_data, [this](gui_data_type& in_data) {
        dear::Text("开始时间"s);
        modify_guard_ = ImGui::InputInt3(*in_data.begin_time.ymd.gui_name, in_data.begin_time.ymd.data.data());
        modify_guard_ = ImGui::InputInt3(*in_data.begin_time.hms.gui_name, in_data.begin_time.hms.data.data());

        dear::Text("结束时间"s);
        modify_guard_ = ImGui::InputInt3(*in_data.end_time.ymd.gui_name, in_data.end_time.ymd.data.data());
        modify_guard_ = ImGui::InputInt3(*in_data.end_time.hms.gui_name, in_data.end_time.hms.data.data());
        modify_guard_ = ImGui::InputText(*in_data.info, &in_data.info);

        if (modify_guard_ = ImGui::Button(*in_data.delete_node); modify_guard_.current_modify()) {
          boost::asio::post(g_io_context(), [this, in_data]() { this->delete_node(in_data); });
        }
      });
    };

    modify_guard_.async_begin_flag();
    return modify_guard_.current_frame_modify();
  }

  void add() {
    gui_data.emplace_back(gui_data_type::friend_type{});
    modify_guard_ = true;
  }

  void delete_node(const gui_data_type& in_data) {
    gui_data |= ranges::actions::remove_if([&](auto&& in_item) { return in_item == in_data; });
    modify_guard_ = true;
  }
  void set(const std::vector<gui_data_type::friend_type>& in_type) {
    gui_data =
        in_type | ranges::view::transform([](auto&& in_item) { return gui_data_type{in_item}; }) | ranges::to_vector;
  }
  std::vector<gui_data_type::friend_type> get() {
    return gui_data | ranges::view::transform([](auto&& in_item) { return gui_data_type::friend_type{in_item}; }) |
           ranges::to_vector;
  }
};

}  // namespace

class time_rules_render::impl {
 public:
  class render_time_rules {
   public:
    work_gui_data_render work_gui_data_attr{};
    time_work_gui_data_render time_work_gui_data_attr{};
    time_info_gui_data_render extra_work_attr{};
    time_info_gui_data_render extra_rest_attr{};
  };
  rules_type rules_attr;

  render_time_rules render_time{};
};

time_rules_render::time_rules_render() : p_i(std::make_unique<impl>()) {
  p_i->rules_attr = rules_type::get_default();
  rules_attr(p_i->rules_attr);
  p_i->render_time.extra_work_attr.gui_name = gui_cache_name_id{"加班时间"s};
  p_i->render_time.extra_rest_attr.gui_name = gui_cache_name_id{"调休时间"s};

  p_i->render_time.work_gui_data_attr.modify_guard_.connect([this](const auto& in) {
    p_i->rules_attr.work_weekdays() = in;
    this->modify_guard_             = true;
  });
  p_i->render_time.time_work_gui_data_attr.modify_guard_.connect([this](const auto& in) {
    p_i->rules_attr.work_time() = in;
    this->modify_guard_         = true;
  });

  p_i->render_time.extra_work_attr.modify_guard_.connect([this](const auto& in) {
    p_i->rules_attr.extra_work() = in;
    this->modify_guard_          = true;
  });
  p_i->render_time.extra_rest_attr.modify_guard_.connect([this](const auto& in) {
    p_i->rules_attr.extra_rest() = in;
    this->modify_guard_          = true;
  });

  g_reg()->ctx().at<core_sig>().project_end_open.connect([this]() {
    this->rules_attr(g_reg()->ctx().at<user::current_user>().get_handle().get<rules_type>());
  });
  g_reg()->ctx().at<core_sig>().select_handles.connect([this](auto) {
    this->rules_attr(g_reg()->ctx().at<user::current_user>().get_handle().get<rules_type>());
  });
}
const time_rules_render::rules_type& time_rules_render::rules_attr() const { return p_i->rules_attr; }
void time_rules_render::rules_attr(const time_rules_render::rules_type& in_rules_type) {
  p_i->rules_attr                              = in_rules_type;

  p_i->render_time.work_gui_data_attr.gui_data = work_gui_data{in_rules_type.work_weekdays()};
  p_i->render_time.time_work_gui_data_attr.set(in_rules_type.work_time());
  p_i->render_time.extra_work_attr.set(in_rules_type.extra_work());
  p_i->render_time.extra_rest_attr.set(in_rules_type.extra_rest());
}
bool time_rules_render::render() {
  if (modify_guard_) modify_guard_(p_i->rules_attr);

  modify_guard_.begin_flag();

  modify_guard_ = p_i->render_time.work_gui_data_attr.render();
  modify_guard_ = p_i->render_time.time_work_gui_data_attr.render();
  modify_guard_ = p_i->render_time.extra_work_attr.render();
  modify_guard_ = p_i->render_time.extra_rest_attr.render();

  return modify_guard_.current_frame_modify();
}
time_rules_render::~time_rules_render() = default;
}  // namespace doodle::gui::time_sequencer_widget_ns
