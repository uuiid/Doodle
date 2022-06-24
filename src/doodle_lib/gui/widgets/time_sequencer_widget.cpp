#include "time_sequencer_widget.h"

#include <doodle_lib/lib_warp/imgui_warp.h>
#include <implot.h>
#include <implot_internal.h>

#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_lib/core/work_clock.h>
#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_core/core/core_sig.h>

namespace doodle::gui {

class time_sequencer_widget::impl {
 public:
  class point_cache {
   public:
    explicit point_cache(const entt::handle& in_h, const time_point_wrap& in_time)
        : handle_(in_h),
          time_point_(in_time),
          has_select(false){};
    entt::handle handle_{};
    time_point_wrap time_point_{};
    bool has_select;

    bool operator==(const point_cache& in_rhs) const {
      return handle_ == in_rhs.handle_ &&
             time_point_ == in_rhs.time_point_;
    }
    bool operator!=(const point_cache& in_rhs) const {
      return !(in_rhs == *this);
    }

    bool operator<(const point_cache& in_rhs) const {
      return time_point_ < in_rhs.time_point_;
    }
    bool operator>(const point_cache& in_rhs) const {
      return in_rhs < *this;
    }
    bool operator<=(const point_cache& in_rhs) const {
      return !(in_rhs < *this);
    }
    bool operator>=(const point_cache& in_rhs) const {
      return !(*this < in_rhs);
    }
  };

  struct gui_rules_cache {
    gui::gui_cache<std::array<gui::gui_cache<bool>, 7>> work_day{
        "工作周"s,
        std::array<gui::gui_cache<bool>, 7>{gui::gui_cache<bool>{"星期日"s, false},
                                            gui::gui_cache<bool>{"星期一"s, true},
                                            gui::gui_cache<bool>{"星期二"s, true},
                                            gui::gui_cache<bool>{"星期三"s, true},
                                            gui::gui_cache<bool>{"星期四"s, true},
                                            gui::gui_cache<bool>{"星期五"s, true},
                                            gui::gui_cache<bool>{"星期六"s, false}}};
    using time_du_cache   = std::array<std::int32_t, 3>;
    using gui_time_pair_t = std::pair<gui::gui_cache<time_du_cache>, gui::gui_cache<time_du_cache>>;

    struct gui_time_pair_button {
      gui_time_pair_button(const gui_time_pair_t& in_p) : data(in_p), button("删除") {}
      gui_time_pair_t data{};
      gui_cache_name_id button{"删除"};
    };

    gui::gui_cache<std::vector<gui_time_pair_button>> work_time{};
    using gui_time_pair_t2 = std::pair<gui::gui_cache<gui_time_pair_t>, gui::gui_cache<gui_time_pair_t>>;

    struct gui_time_pair2_button {
      gui_time_pair2_button(const gui_time_pair_t2& in_p) : data(in_p), button("删除") {}
      gui_time_pair_t2 data{};
      gui_cache_name_id button{"删除"};
    };
    struct button_cache {
      gui_cache_name_id button{"添加"};
    };

    gui::gui_cache<std::vector<gui_time_pair2_button>, button_cache> extra_holidays{};
    gui::gui_cache<std::vector<gui_time_pair2_button>, button_cache> extra_work{};
    gui::gui_cache<std::vector<gui_time_pair2_button>, button_cache> extra_rest{};

    explicit gui_rules_cache() = default;

    static gui_time_pair2_button to_gui_time_pair_t2(const decltype(std::declval<doodle::business::rules>().extra_holidays)::value_type& in_value) {
      time_point_wrap l_time_point{in_value.first};
      time_point_wrap l_time_point2{in_value.second};

      auto l_p1 = l_time_point.compose();
      auto l_p2 = l_time_point2.compose();
      return gui_time_pair2_button{std::make_pair(
          gui::gui_cache<gui_time_pair_t>{
              "开始时间"s,
              std::make_pair(gui::gui_cache<time_du_cache>{"年月日"s,
                                                           time_du_cache{
                                                               std::get<0>(l_p1),
                                                               std::get<1>(l_p1),
                                                               std::get<2>(l_p1)}},
                             gui::gui_cache<time_du_cache>{"时分秒"s,
                                                           time_du_cache{
                                                               std::get<3>(l_p1),
                                                               std::get<4>(l_p1),
                                                               std::get<5>(l_p1)}})},
          gui::gui_cache<gui_time_pair_t>{
              "结束时间"s,
              std::make_pair(gui::gui_cache<time_du_cache>{"年月日"s,
                                                           time_du_cache{
                                                               std::get<0>(l_p2),
                                                               std::get<1>(l_p2),
                                                               std::get<2>(l_p2)}},
                             gui::gui_cache<time_du_cache>{"时分秒"s,
                                                           time_du_cache{
                                                               std::get<3>(l_p2),
                                                               std::get<4>(l_p2),
                                                               std::get<5>(l_p2)}})})};
    }
    static decltype(std::declval<doodle::business::rules>().extra_holidays)::value_type form_gui_time_pair_t2(const gui_time_pair2_button& in_value) {
      doodle::chrono::local_time_pos l_pos{
          doodle::chrono::local_days{
              doodle::chrono::year_month_day{
                  doodle::chrono::year{in_value.data.first.data.first.data[0]},
                  doodle::chrono::month{boost::numeric_cast<std::uint32_t>(in_value.data.first.data.first.data[0])},
                  doodle::chrono::day{boost::numeric_cast<std::uint32_t>(in_value.data.first.data.first.data[0])}}} +
          chrono::hours{in_value.data.first.data.second.data[0]} +
          chrono::minutes{in_value.data.first.data.second.data[1]} +
          doodle::chrono::seconds{in_value.data.first.data.second.data[2]}};

      doodle::chrono::local_time_pos l_pos2{
          doodle::chrono::local_days{
              doodle::chrono::year_month_day{
                  doodle::chrono::year{in_value.data.second.data.first.data[0]},
                  doodle::chrono::month{boost::numeric_cast<std::uint32_t>(in_value.data.second.data.first.data[0])},
                  doodle::chrono::day{boost::numeric_cast<std::uint32_t>(in_value.data.second.data.first.data[0])}}} +
          chrono::hours{in_value.data.second.data.second.data[0]} +
          chrono::minutes{in_value.data.second.data.second.data[1]} +
          doodle::chrono::seconds{in_value.data.second.data.second.data[2]}};
      return std::make_pair(l_pos, l_pos2);
    }

    static bool render_gui_time_pair2_button(decltype(extra_holidays)& in_data) {
      dear::CollapsingHeader{*in_data} && [&]() {
        auto& l_list = in_data();
        if (imgui::Button(*in_data.button)) {
          l_list.emplace_back(
              impl::gui_rules_cache::gui_time_pair2_button{
                  std::make_pair(
                      gui::gui_cache<gui_time_pair_t>{
                          "开始时间"s,
                          std::make_pair(gui::gui_cache<time_du_cache>{"年月日"s,
                                                                       time_du_cache{}},
                                         gui::gui_cache<time_du_cache>{"时分秒"s,
                                                                       time_du_cache{}})},
                      gui::gui_cache<gui_time_pair_t>{
                          "结束时间"s,
                          std::make_pair(gui::gui_cache<time_du_cache>{"年月日"s,
                                                                       time_du_cache{}},
                                         gui::gui_cache<time_du_cache>{"时分秒"s,
                                                                       time_du_cache{}})})});
        }

        for (auto l_i = l_list.begin(); l_i != l_list.end();) {
          ImGui::InputInt3(*(l_i->data).first().first, (l_i->data).first().first().data());
          ImGui::SliderInt3(*(l_i->data).first().second, (l_i->data).first().second().data(), 0, 59);

          ImGui::InputInt3(*(l_i->data).second().first, (l_i->data).second().first().data());
          ImGui::SliderInt3(*(l_i->data).second().second, (l_i->data).second().second().data(), 0, 59);
          if (ImGui::Button(*l_i->button)) {
            l_i = l_list.erase(l_i);
          } else {
            ++l_i;
          }
          ImGui::Separator();
        }
      };
      return false;
    };

    explicit gui_rules_cache(const doodle::business::rules& in_rules)
        : work_day{
              "工作周"s,
              std::array<gui::gui_cache<bool>, 7>{gui::gui_cache<bool>{"星期日"s, in_rules.work_weekdays[0]},
                                                  gui::gui_cache<bool>{"星期一"s, in_rules.work_weekdays[1]},
                                                  gui::gui_cache<bool>{"星期二"s, in_rules.work_weekdays[2]},
                                                  gui::gui_cache<bool>{"星期三"s, in_rules.work_weekdays[3]},
                                                  gui::gui_cache<bool>{"星期四"s, in_rules.work_weekdays[4]},
                                                  gui::gui_cache<bool>{"星期五"s, in_rules.work_weekdays[5]},
                                                  gui::gui_cache<bool>{"星期六"s, in_rules.work_weekdays[6]}}},
          work_time("工作时间"s, in_rules.work_pair | ranges::view::transform([](const decltype(in_rules.work_pair)::value_type& in_value) -> gui_time_pair_button {
                                   chrono::hh_mm_ss l_hh_mm_ss{in_value.first};
                                   chrono::hh_mm_ss l_hh_mm_ss2{in_value.second};
                                   auto l_p = std::make_pair(
                                       gui::gui_cache<time_du_cache>{"开始时间"s,
                                                                     time_du_cache{boost::numeric_cast<std::int32_t>(l_hh_mm_ss.hours().count()),
                                                                                   boost::numeric_cast<std::int32_t>(l_hh_mm_ss.minutes().count()),
                                                                                   boost::numeric_cast<std::int32_t>(l_hh_mm_ss.seconds().count())}},
                                       gui::gui_cache<time_du_cache>{"结束时间"s,
                                                                     time_du_cache{boost::numeric_cast<std::int32_t>(l_hh_mm_ss2.hours().count()),
                                                                                   boost::numeric_cast<std::int32_t>(l_hh_mm_ss2.minutes().count()),
                                                                                   boost::numeric_cast<std::int32_t>(l_hh_mm_ss2.seconds().count())}});
                                   return gui_time_pair_button{l_p};
                                 }) | ranges::to_vector),
          extra_holidays("节假日"s, in_rules.extra_holidays | ranges::view::transform(to_gui_time_pair_t2) | ranges::to_vector),
          extra_work("加班时间"s, in_rules.extra_work | ranges::view::transform(to_gui_time_pair_t2) | ranges::to_vector),
          extra_rest("调休时间"s, in_rules.extra_rest | ranges::view::transform(to_gui_time_pair_t2) | ranges::to_vector){

          };

    explicit operator doodle::business::rules() {
      doodle::business::rules l_r{};
      l_r.work_weekdays[0] = work_day.data[0].data;
      l_r.work_weekdays[1] = work_day.data[1].data;
      l_r.work_weekdays[2] = work_day.data[2].data;
      l_r.work_weekdays[3] = work_day.data[3].data;
      l_r.work_weekdays[4] = work_day.data[4].data;
      l_r.work_weekdays[5] = work_day.data[5].data;
      l_r.work_weekdays[6] = work_day.data[6].data;

      l_r.work_pair        = work_time.data |
                      ranges::views::transform([](const decltype(work_time.data)::value_type& in_value) -> decltype(l_r.work_pair)::value_type {
                        chrono::hh_mm_ss l_hh_mm_ss{chrono::hours{in_value.data.first.data[0]} +
                                                    chrono::minutes{in_value.data.first.data[1]} +
                                                    doodle::chrono::seconds{in_value.data.first.data[2]}};
                        chrono::hh_mm_ss l_hh_mm_ss2{chrono::hours{in_value.data.second.data[0]} +
                                                     chrono::minutes{in_value.data.second.data[1]} +
                                                     doodle::chrono::seconds{in_value.data.second.data[2]}};
                        return std::make_pair(
                            l_hh_mm_ss.to_duration(),
                            l_hh_mm_ss2.to_duration());
                      }) |
                      ranges::to_vector;
      l_r.extra_holidays = extra_holidays() | ranges::views::transform(form_gui_time_pair_t2) | ranges::to_vector;
      l_r.extra_work     = extra_work() | ranges::views::transform(form_gui_time_pair_t2) | ranges::to_vector;
      l_r.extra_rest     = extra_rest() | ranges::views::transform(form_gui_time_pair_t2) | ranges::to_vector;
      return l_r;
    }
  };

  class view_cache {
   public:
    bool into_select{false};
    bool outto_select{false};
    ImPlotRect rect_select_{};
    bool set_other_view{false};
  };

 public:
  impl(){

  };
  ~impl() = default;
  std::vector<point_cache> time_list{};
  std::vector<double> time_list_x{};
  std::vector<double> time_list_y{};

  ImPlotRect rect_{};
  ImPlotRect rect_org_{};

  std::vector<doodle::chrono::hours_double> work_time;
  std::vector<std::double_t> work_time_plots;
  std::vector<std::pair<std::double_t, std::double_t>> work_time_plots_drag;

  /// \brief 时间规则
  doodle::business::rules rules_{};
  /// \brief 工作时间计算
  doodle::business::work_clock work_clock_{};

  view_cache view1_{};
  view_cache view2_{};

  std::size_t index_begin_{0};
  std::size_t index_end_{0};
  std::size_t index_view_end{0};
  boost::signals2::scoped_connection l_select_conn{};

  gui_cache<gui_rules_cache> rules_cache{"计算规则", rules_};

  std::vector<ImRect> shaded_works_time{};

  void set_shaded_works_time(const std::vector<std::pair<doodle::chrono::local_time_pos, doodle::chrono::local_time_pos>>& in_works) {
    shaded_works_time.clear();
    ranges::for_each(in_works, [this](const std::pair<doodle::chrono::local_time_pos, doodle::chrono::local_time_pos>& in_pair) {
      shaded_works_time.emplace_back(
          in_pair.first.time_since_epoch().count(), 0,                         //(左下角)
          in_pair.second.time_since_epoch().count(), time_list.size() + 500);  // 右上角
    });
  }

  void refresh_work_clock_() {
    if (!time_list.empty()) {
      work_clock_.set_interval(time_list.front().time_point_ - chrono::days{4},
                               time_list.back().time_point_ + chrono::days{4});
      refresh(time_list);
      refresh_work_time(time_list);
      set_shaded_works_time(work_clock_.get_work_du(time_list.front().time_point_ - chrono::days{4},
                                                    time_list.back().time_point_ + chrono::days{4}));
    }
  }

  bool find_selects(const ImPlotRect& in_rect) {
    using tmp_value_t = decltype((time_list | ranges::views::enumerate).begin())::value_type;
    auto l_index      = ranges::views::ints(0, (std::int32_t)time_list.size());
    ranges::for_each(l_index,
                     [&](const std::int32_t& in) {
                       time_list[in].has_select = in_rect.Contains(time_list_x[in], time_list_y[in]);
                     });
    auto l_r = ranges::any_of(time_list, [](const point_cache& in) {
      return in.has_select;
    });
    if (l_r)
      rect_org_ = rect_ = in_rect;
    return l_r;
  }

  void refresh(const decltype(time_list)& in_list) {
    time_list_x = in_list |
                  ranges::views::transform([](const impl::point_cache& in) -> double {
                    return doodle::chrono::floor<doodle::chrono::seconds>(
                               in.time_point_.zoned_time_.get_sys_time())
                        .time_since_epoch()
                        .count();
                  }) |
                  ranges::to_vector;

    time_list_y = in_list |
                  ranges::views::enumerate |
                  ranges::views::transform([](const auto& in) -> double {
                    return in.first;
                  }) |
                  ranges::to_vector;
  }
  void refresh_work_time(const decltype(time_list)& in_list) {
    if (in_list.empty())
      return;
    auto l_list = in_list;
    l_list |= ranges::actions::sort;

    decltype(l_list.front().time_point_)::time_local_point l_begin =
        doodle::chrono::floor<chrono::days>(l_list.front().time_point_.zoned_time_.get_local_time());
    work_time = l_list |
                ranges::views::transform(
                    [&](const impl::point_cache& in_time) -> doodle::chrono::hours_double {
                      auto l_end = in_time.time_point_.zoned_time_.get_local_time();
                      auto l_d   = work_clock_(l_begin, l_end);
                      l_begin    = l_end;
                      return l_d;
                    }) |
                ranges::to_vector;
    work_time_plots = work_time |
                      ranges::views::transform(
                          [&](const doodle::chrono::hours_double& in_time) -> std::double_t {
                            return in_time.count();
                          }) |
                      ranges::to_vector;
    work_time_plots_drag = work_time |
                           ranges::views::enumerate |
                           ranges::views::transform(
                               [&](const std::pair<std::size_t, doodle::chrono::hours_double>& in_item) {
                                 return std::make_pair(
                                     boost::numeric_cast<std::double_t>(in_item.first),
                                     in_item.second.count());
                               }) |
                           ranges::to_vector;
  }

  static std::double_t ImPlotRange_Centre(const ImPlotRange& in) {
    return in.Min + in.Size() / 2;
  };

  void modify_time_refresh(const ImPlotRect& in_rect) {
    auto l_mod_size = boost::numeric_cast<std::int64_t>(
        ImPlotRange_Centre(in_rect.X) - ImPlotRange_Centre(rect_org_.X));
    if (l_mod_size == 0)
      return;
    auto l_time_list = time_list | ranges::views::transform([&](impl::point_cache in) -> impl::point_cache {
                         if (in.has_select) {
                           in.time_point_ += doodle::chrono::seconds{l_mod_size};
                         }
                         return in;
                       }) |
                       ranges::to_vector;
    rect_ = in_rect;
    refresh(l_time_list);
    boost::asio::post(g_io_context(),
                      [=]() { refresh_work_time(l_time_list); });
  };
  void modify_time() {
    auto l_mod_size = boost::numeric_cast<std::int64_t>(
        ImPlotRange_Centre(rect_.X) - ImPlotRange_Centre(rect_org_.X));
    if (l_mod_size == 0)
      return;
    ranges::for_each(time_list, [&](impl::point_cache& in) {
      if (in.has_select) {
        in.time_point_ += doodle::chrono::seconds{l_mod_size};
      }
    });
    time_list |= ranges::actions::sort;
    refresh(time_list);
    refresh_work_time(time_list);
  };

  void add_time_du(const std::size_t& in_index, const std::double_t& in_add_time_du) {
    chick_true<doodle_error>(in_index < time_list.size(), DOODLE_LOC, "错误的索引 {}", in_index);

    work_time_plots[in_index] += in_add_time_du;
    work_time_plots[in_index]             = std::max(work_time_plots[in_index], std::double_t(0));
    work_time_plots_drag[in_index].second = std::max(work_time_plots_drag[in_index].second, std::double_t(0));
    auto l_start_time                     = time_list[std::max(in_index - 1, std::size_t(0))].time_point_.zoned_time_.get_local_time();
    if (in_index == 0)
      l_start_time = doodle::chrono::floor<doodle::chrono::days>(l_start_time);

    auto l_time_list = time_list;

    ranges::for_each(ranges::views::ints(in_index, l_time_list.size()),
                     [&](const std::int32_t& in_ints) {
                       l_time_list[in_ints].time_point_ += doodle::chrono::seconds{
                           boost::numeric_cast<doodle::chrono::seconds::rep>(in_add_time_du)};
                     });
    //    refresh(l_time_list);
    refresh_work_time(l_time_list);
  }

  void set_time_point(const std::size_t& in_index, const std::double_t& in_time_s) {
    chick_true<doodle_error>(in_index < time_list.size(), DOODLE_LOC, "错误的索引 {}", in_index);

    auto l_time_list = time_list;

    ranges::for_each(ranges::views::ints(in_index, l_time_list.size()),
                     [&](const std::int32_t& in_ints) {
                       l_time_list[in_ints].time_point_ += doodle::chrono::seconds{
                           boost::numeric_cast<doodle::chrono::seconds::rep>(in_time_s)};
                     });
    refresh(l_time_list);
    refresh_work_time(l_time_list);
  }

  void average_time(std::size_t in_begin,
                    std::size_t in_end) {
    if (time_list.empty()) return;

    auto l_begin_index = std::max(std::size_t(0), std::min(in_begin, in_end));
    auto l_end_index   = std::min(time_list.size() - 1, std::max(in_begin, in_end));
    if (l_begin_index == l_end_index) return;

    decltype(time_list.front().time_point_)::time_local_point l_begin =
        doodle::chrono::floor<chrono::days>(
            time_list[l_begin_index].time_point_.zoned_time_.get_local_time());

    auto l_all_len  = work_clock_(time_list[l_begin_index].time_point_,
                                  time_list[l_end_index].time_point_);
    const auto l_du = l_all_len / boost::numeric_cast<std::double_t>(l_end_index - l_begin_index + 1);

    ranges::for_each(time_list | ranges::views::slice(l_begin_index, l_end_index + 1),
                     [&](decltype(time_list)::value_type& in_) {
                       in_.time_point_ = time_point_wrap{work_clock_.next_time(l_begin, l_du)};
                       l_begin         = in_.time_point_.zoned_time_.get_local_time();
                     });
    refresh(time_list);
    refresh_work_time(time_list);
  }

  void refresh_view1_index() {
    if (view1_.into_select && view1_.outto_select) {
      auto l_ben         = ranges::find_if(time_list_y,
                                           [&](const std::double_t& in_) -> bool {
                                     auto l_index = boost::numeric_cast<std::size_t>(in_);
                                     return view1_.rect_select_.Contains(time_list_x[l_index], in_);
                                   });
      auto l_begin_index = ranges::distance(time_list_y.begin(), l_ben);

      auto l_end         = ranges::find_if(l_ben, time_list_y.end(),
                                           [&](const std::double_t& in_) -> bool {
                                     auto l_index = boost::numeric_cast<std::size_t>(in_);
                                     return !view1_.rect_select_.Contains(time_list_x[l_index], in_);
                                   });
      auto l_end_index   = ranges::distance(time_list_y.begin(), l_end);

      index_begin_       = std::max(std::size_t(0), std::size_t(l_begin_index));
      index_end_         = std::min(time_list.size() - 1, index_begin_ + 10);
      index_view_end     = std::min(time_list.size() - 1, std::size_t(l_end_index));
      view1_.into_select = view1_.outto_select = false;
      DOODLE_LOG_INFO(" index {}->{}->{} ", index_begin_, index_end_, index_view_end);
      view1_.set_other_view = true;
    }
  }
  void refresh_view2_index() {
    if (view2_.into_select && view2_.outto_select) {
      index_begin_       = std::max(std::size_t(0), std::size_t(view2_.rect_select_.X.Min));
      index_end_         = std::min(time_list.size() - 1, index_begin_ + 10);
      index_view_end     = std::min(time_list.size() - 1, std::size_t(std::size_t(view2_.rect_select_.X.Max)));
      view2_.into_select = view2_.outto_select = false;
      DOODLE_LOG_INFO(" index {}->{}->{} ", index_begin_, index_end_, index_view_end);

      view2_.set_other_view = true;
    }
  }
  ImPlotRect get_view1_rect() {
    ImPlotRect l_r{time_list_x[index_begin_] - 360,
                   time_list_x[index_view_end] + 360,
                   time_list_y[index_begin_] - 1,
                   time_list_y[index_view_end] + 1};
    return l_r;
  }
  ImPlotRect get_view2_rect() {
    auto l_tmp  = work_time_plots;
    auto l_list = l_tmp | ranges::views::slice(index_begin_, index_view_end);
    l_list |= ranges::actions::sort;
    ImPlotRect l_r{
        std::double_t(index_begin_) - 1,
        std::double_t(index_view_end + 1),
        0,
        l_list.empty() ? std::double_t(8) : l_list.back()};
    return l_r;
  }

  void save() {
    ranges::for_each(time_list | ranges::views::filter([](const point_cache& in) -> bool {
                       return in.handle_.valid() && in.handle_.any_of<database>();
                     }),
                     [](const point_cache& in) {
                       in.handle_.replace<time_point_wrap>(in.time_point_);
                       database::save(in.handle_);
                     });
  }
};

time_sequencer_widget::time_sequencer_widget()
    : p_i(std::make_unique<impl>()) {
  ImPlot::GetStyle().UseLocalTime = true;

  p_i->l_select_conn =
      g_reg()
          ->ctx()
          .at<core_sig>()
          .select_handles.connect(
              [this](const std::vector<entt::handle>& in_vector) {
                p_i->time_list = in_vector |
                                 ranges::views::filter(
                                     [](const entt::handle& in) -> bool {
                                       return in.any_of<time_point_wrap>();
                                     }) |
                                 ranges::views::transform(
                                     [](const entt::handle& in) -> impl::point_cache {
                                       return impl::point_cache{in, in.get<time_point_wrap>()};
                                     }) |
                                 ranges::to_vector;
                p_i->time_list |= ranges::actions::sort;
                p_i->work_clock_.set_rules(p_i->rules_);
                p_i->refresh_work_clock_();
              });
}

time_sequencer_widget::~time_sequencer_widget() = default;

void time_sequencer_widget::init() {
}
void time_sequencer_widget::succeeded() {
}
void time_sequencer_widget::failed() {
}
void time_sequencer_widget::aborted() {
}

void time_sequencer_widget::update(
    const chrono::duration<chrono::system_clock::rep,
                           chrono::system_clock::period>&,
    void* in_data) {
  ImGui::Checkbox("本地时间", &ImPlot::GetStyle().UseLocalTime);
  ImGui::SameLine();
  ImGui::Checkbox("24 小时制", &ImPlot::GetStyle().Use24HourClock);
  if (p_i->time_list.empty())
    return;

  std::size_t l_index_begin{0};
  std::size_t l_index_end{0};

  if (ImGui::Button("提交更新")) p_i->save();

  if (ImPlot::BeginPlot("时间折线图")) {
    /// 设置州为时间轴
    ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_Time);
    double t_min = doodle::chrono::floor<doodle::chrono::seconds>(
                       p_i->time_list.front().time_point_.zoned_time_.get_sys_time())
                       .time_since_epoch()
                       .count();  // 01/01/2021 @ 12:00:00am (UTC)
    double t_max = doodle::chrono::floor<doodle::chrono::seconds>(
                       p_i->time_list.back().time_point_.zoned_time_.get_sys_time())
                       .time_since_epoch()
                       .count();  // 01/01/2022 @ 12:00:00am (UTC)
    ImPlot::SetupAxisLimits(ImAxis_X1, t_min, t_max);
    ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond);
    if (p_i->view2_.set_other_view) {
      auto l_rect = p_i->get_view1_rect();
      ImPlot::SetupAxesLimits(l_rect.X.Min,
                              l_rect.X.Max,
                              l_rect.Y.Min,
                              l_rect.Y.Max,
                              ImGuiCond_Always);
      p_i->view2_.set_other_view = false;
    }
    /// \brief 时间折线
    ImPlot::PlotLine("Time Series",
                     p_i->time_list_x.data(),
                     p_i->time_list_y.data(),
                     p_i->time_list.size());
    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
    /// \brief 时间背景
    ranges::for_each(p_i->shaded_works_time, [](const ImRect& in) {
      ImPlot::GetPlotDrawList()->AddRectFilled(in.Min, in.Max, IM_COL32(128, 0, 255, 255));
    });
    ImPlot::PopStyleVar();

    if (ImPlot::IsPlotSelected()) {
      //      auto l_e   = ImPlot::GetPlotLimits().X;
      p_i->view1_.rect_select_ = ImPlot::GetPlotSelection();

      p_i->view1_.into_select  = true;
    } else if (p_i->view1_.into_select && !ImPlot::IsPlotSelected()) {
      p_i->view1_.outto_select = true;
    }
    p_i->refresh_view1_index();

    // 绘制可调整点
    {
      auto l_linm = ImPlot::GetPlotLimits().X;
      for (int l_i = p_i->index_begin_; l_i < p_i->index_end_; ++l_i) {
        std::double_t l_org_x = doodle::chrono::floor<chrono::seconds>(
                                    p_i->time_list[l_i].time_point_.zoned_time_.get_sys_time())
                                    .time_since_epoch()
                                    .count();
        if (ImPlot::DragPoint((std::int32_t)l_i,
                              (std::double_t*)&(p_i->time_list_x[l_i]),
                              &(p_i->time_list_y[l_i]), ImVec4{0, 0.9f, 0, 1})) {
          p_i->time_list_y[l_i] = l_i;
          p_i->set_time_point(l_i, p_i->time_list_x[l_i] - l_org_x);
        };
      }
    }

    ImPlot::EndPlot();
  }
  /// \brief 时间柱状图
  if (ImPlot::BeginPlot("工作柱状图")) {
    if (p_i->view1_.set_other_view) {
      auto l_rect = p_i->get_view2_rect();
      ImPlot::SetupAxesLimits(l_rect.X.Min,
                              l_rect.X.Max,
                              l_rect.Y.Min,
                              l_rect.Y.Max,
                              ImGuiCond_Always);
      p_i->view1_.set_other_view = false;
    }
    ImPlot::PlotBars("Bars",
                     p_i->work_time_plots.data(),
                     p_i->work_time_plots.size());
    if (ImPlot::IsPlotSelected()) {
      //      auto l_e   = ImPlot::GetPlotLimits().X;
      p_i->view2_.rect_select_ = ImPlot::GetPlotSelection();

      p_i->view2_.into_select  = true;

    } else if (p_i->view2_.into_select && !ImPlot::IsPlotSelected()) {
      p_i->view2_.outto_select = true;
    }
    p_i->refresh_view2_index();

    ImPlot::EndPlot();
  }

  if (ImGui::Button("平均时间")) p_i->average_time(0, p_i->time_list.size());
  ImGui::SameLine();
  if (ImGui::Button("平均视图内时间")) p_i->average_time(p_i->index_begin_, p_i->index_view_end);

  ImGui::Separator();
  dear::Text(p_i->rules_cache.gui_name.name);

  if (ImGui::Button("应用规则")) {
    p_i->rules_ = static_cast<decltype(p_i->rules_)>(p_i->rules_cache());
    p_i->work_clock_.set_rules(p_i->rules_);
    p_i->refresh_work_clock_();
  }

  dear::CollapsingHeader{*p_i->rules_cache().work_day} && [this]() {
    dear::HelpMarker{"按星期去计算工作时间"};
    ranges::for_each(p_i->rules_cache().work_day(), [](decltype(p_i->rules_cache().work_day().front()) in_value) {
      ImGui::Checkbox(*in_value, &in_value);
    });
  };

  dear::CollapsingHeader{*p_i->rules_cache().work_time} && [this]() {
    auto& l_list = p_i->rules_cache().work_time();
    dear::HelpMarker{"每天的开始和结束时间段"};
    if (imgui::Button("添加")) {
      l_list.emplace_back(
          impl::gui_rules_cache::gui_time_pair_button{
              std::make_pair(
                  gui::gui_cache<impl::gui_rules_cache::time_du_cache>{
                      "开始时间"s,
                      impl::gui_rules_cache::time_du_cache{0, 0, 0}},
                  gui::gui_cache<impl::gui_rules_cache::time_du_cache>{"结束时间"s, impl::gui_rules_cache::time_du_cache{0, 0, 0}})});
    }

    for (auto l_i = l_list.begin(); l_i != l_list.end();) {
      ImGui::SliderInt3(*(l_i->data).first, (l_i->data).first().data(), 0, 59);
      ImGui::SliderInt3(*(l_i->data).second, (l_i->data).second().data(), 0, 59);
      if (ImGui::Button(*l_i->button)) {
        l_i = l_list.erase(l_i);
      } else {
        ++l_i;
      }
      ImGui::Separator();
    }
  };
  impl::gui_rules_cache::render_gui_time_pair2_button(p_i->rules_cache().extra_holidays);
  dear::HelpMarker{"节假日"};
  impl::gui_rules_cache::render_gui_time_pair2_button(p_i->rules_cache().extra_work);
  dear::HelpMarker{"加班时间"};
  impl::gui_rules_cache::render_gui_time_pair2_button(p_i->rules_cache().extra_rest);
  dear::HelpMarker{"调休sh时间"};
}
}  // namespace doodle::gui
