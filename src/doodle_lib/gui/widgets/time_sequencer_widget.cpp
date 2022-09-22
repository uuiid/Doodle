#include "time_sequencer_widget.h"

#include <doodle_lib/lib_warp/imgui_warp.h>
#include <implot.h>
#include <implot_internal.h>

#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/detail/time_point_info.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/time_tool/work_clock.h>
#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_lib/gui/gui_ref/cross_frame_check.h>
#include <doodle_lib/gui/widgets/time_sequencer_widgets/time_rules_render.h>

#include <doodle_core/core/core_sig.h>

#include <utility>

namespace doodle::gui {

namespace {

}

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

  class view_cache {
   public:
    bool into_select{false};
    bool outto_select{false};
    ImPlotRect rect_select_{};
    bool set_other_view{false};
  };

 public:
  impl() = default;
  ;
  ~impl() = default;
  std::vector<point_cache> time_list{};
  std::vector<double> time_list_x{};
  std::vector<double> time_list_y{};

  std::vector<doodle::chrono::hours_double> work_time;
  std::vector<std::double_t> work_time_plots;

  /// \brief 时间规则
  doodle::business::rules rules_{doodle::business::rules::get_default()};
  /// \brief 工作时间计算
  doodle::business::work_clock work_clock_{};

  view_cache view1_{};
  view_cache view2_{};

  std::size_t index_begin_{0};
  std::size_t index_end_{0};
  std::size_t index_view_end{0};

  std::int32_t drag_point_current{0};

  boost::signals2::scoped_connection l_select_conn{};

  gui_cache<time_sequencer_widget_ns::time_rules_render> rules_cache{"计算规则"s};

  std::vector<std::double_t> shaded_works_time{};

  detail::cross_frame_check<std::tuple<std::int32_t, std::double_t>> edit_chick{};
  detail::cross_frame_check<ImPlotRect> chick_view1{};
  detail::cross_frame_check<ImPlotRect> chick_view2{};
  std::string title_name_;

  void set_shaded_works_time(const std::vector<std::pair<time_point_wrap, time_point_wrap>>& in_works) {
    shaded_works_time.clear();
    ranges::for_each(in_works, [this](const std::pair<time_point_wrap, time_point_wrap>& in_pair) {
      shaded_works_time.emplace_back(
          doodle::chrono::floor<doodle::chrono::seconds>(in_pair.first.get_sys_time()).time_since_epoch().count()
      );
      shaded_works_time.emplace_back(
          doodle::chrono::floor<doodle::chrono::seconds>(in_pair.second.get_sys_time()).time_since_epoch().count()
      );
    });
  }

  void refresh_work_clock_() {
    if (!time_list.empty()) {
      work_clock_.set_interval(time_list.front().time_point_.current_month_start() - chrono::days{4}, time_list.back().time_point_.current_month_end() + chrono::days{4});
      DOODLE_LOG_INFO(work_clock_.debug_print());
      refresh_cache(time_list);
      refresh_work_time(time_list);
      set_shaded_works_time(work_clock_.get_work_du(time_list.front().time_point_.current_month_start(), time_list.back().time_point_.current_month_end()));
    }
  }

  void refresh_cache(const decltype(time_list)& in_list) {
    time_list_x = in_list |
                  ranges::views::transform([](const impl::point_cache& in) -> double {
                    return doodle::chrono::floor<doodle::chrono::seconds>(
                               in.time_point_.get_sys_time()
                    )
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

    decltype(l_list.front().time_point_) l_begin =
        l_list.front().time_point_.current_month_start();

    work_time = l_list |
                ranges::views::transform(
                    [&](const impl::point_cache& in_time) -> doodle::chrono::hours_double {
                      auto l_d = work_clock_(l_begin, in_time.time_point_);
                      l_begin  = in_time.time_point_;
                      return l_d;
                    }
                ) |
                ranges::to_vector;
    work_time_plots = work_time |
                      ranges::views::transform(
                          [&](const doodle::chrono::hours_double& in_time) -> std::double_t {
                            return in_time.count() > 0.5 ? in_time.count() : 0.0;
                          }
                      ) |
                      ranges::to_vector;
  }

  void _set_time_point(decltype(time_list)& in_list, const std::size_t& in_index, const std::double_t& in_time_s) {
    DOODLE_CHICK(0 <= in_index && in_index < in_list.size(), doodle_error{"错误的索引 {}", in_index});

    auto l_index = boost::numeric_cast<std::int64_t>(in_index);
    auto l_min   = in_list[std::max(0ll, l_index - 1)].time_point_;
    auto l_max   = in_list[std::min(boost::numeric_cast<std::int64_t>(in_list.size() - 1), l_index + 1)].time_point_;

    if (l_index == (in_list.size() - 1)) {
      l_max = time_point_wrap::max();
    } else if (l_index == 0) {
      l_min = time_point_wrap::min();
    }

    time_point_wrap l_time{time_point_wrap::time_point{
        doodle::chrono::seconds{
            boost::numeric_cast<doodle::chrono::seconds::rep>(in_time_s)}}};
    DOODLE_LOG_INFO("时间 {} 限制为 {} -> {}", l_time, l_min, l_max);

    auto l_value                 = std::max(l_min, std::min(l_time, l_max));
    in_list[l_index].time_point_ = l_value;

    refresh_cache(in_list);
    refresh_work_time(in_list);
  }
  inline void set_time_point(const std::size_t& in_index, const std::double_t& in_time_s) {
    _set_time_point(time_list, in_index, in_time_s);
  }

  inline void refresh_DragPoint_time_point(const std::size_t& in_index, const std::double_t& in_time_s) {
    auto l_time_list = time_list;
    _set_time_point(l_time_list, in_index, in_time_s);
  }

  void average_time() {
    if (time_list.empty()) return;
    time_list |= ranges::actions::sort;

    decltype(time_list.front().time_point_) l_begin =
        time_list.front().time_point_.current_month_start();
    auto l_all_len  = work_clock_(l_begin, time_list.back().time_point_.current_month_end());
    const auto l_du = l_all_len / time_list.size();

    ranges::for_each(time_list, [&](decltype(time_list)::value_type& in_) {
      in_.time_point_ = work_clock_.next_time(l_begin, l_du);
      l_begin         = in_.time_point_;
    });
    refresh_cache(time_list);
    refresh_work_time(time_list);
  }

  void refresh_view1_index() {
    if (view1_.into_select && view1_.outto_select) {
      auto l_ben         = ranges::find_if(time_list_y, [&](const std::double_t& in_) -> bool {
        auto l_index = boost::numeric_cast<std::size_t>(in_);
        return view1_.rect_select_.Contains(time_list_x[l_index], in_);
      });
      auto l_begin_index = ranges::distance(time_list_y.begin(), l_ben);

      auto l_end         = ranges::find_if(l_ben, time_list_y.end(), [&](const std::double_t& in_) -> bool {
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
    ImPlotRect l_r{time_list_x[index_begin_] - 360, time_list_x[index_view_end] + 360, time_list_y[index_begin_] - 1, time_list_y[index_view_end] + 1};
    return l_r;
  }
  ImPlotRect get_view2_rect() {
    auto l_tmp = work_time_plots;
    if (0 <= index_begin_ &&
        index_begin_ < index_view_end &&
        index_view_end < l_tmp.size()) {
      auto l_list = l_tmp | ranges::views::slice(index_begin_, index_view_end);
      l_list |= ranges::actions::sort;
      ImPlotRect l_r{
          std::double_t(index_begin_) - 1,
          std::double_t(index_view_end + 1),
          0,
          l_list.empty() ? std::double_t(8) : l_list.back()};
      return l_r;
    }
    return ImPlotRect{};
  }

  std::size_t get_iter_DragPoint(std::double_t in_current) {
    auto l_current = ranges::lower_bound(time_list_x, in_current);
    std::int64_t l_begin{};
    if (l_current != time_list_x.end()) {
      l_begin = std::distance(time_list_x.begin(), l_current);
    }
    return boost::numeric_cast<std::size_t>(l_begin);
  }

  void save() {
    decltype(time_list.front().time_point_) l_begin =
        time_list.front().time_point_.current_month_start();
    ranges::for_each(time_list | ranges::views::filter([](const point_cache& in) -> bool {
                       return in.handle_.valid() && in.handle_.any_of<database>();
                     }),
                     [&](const point_cache& in) {
                       in.handle_.replace<time_point_wrap>(in.time_point_);
                       DOODLE_LOG_INFO("设置时间点 {}", in.time_point_);
                       auto& l_com = in.handle_.get_or_emplace<comment>();
                       l_com.p_time_info.clear();
                       if (auto l_info = work_clock_.get_time_info(l_begin, in.time_point_); l_info) {
                         l_com.p_time_info = *l_info;
                       }

                       l_begin = in.time_point_;
                       database::save(in.handle_);
                     });
  }
};

time_sequencer_widget::time_sequencer_widget()
    : p_i(std::make_unique<impl>()) {
  p_i->title_name_                 = std::string{name};
  ImPlot::GetStyle().UseLocalTime   = true;
  ImPlot::GetStyle().Use24HourClock = true;

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
                                     }
                                 ) |
                                 ranges::views::transform(
                                     [](const entt::handle& in) -> impl::point_cache {
                                       return impl::point_cache{in, in.get<time_point_wrap>()};
                                     }
                                 ) |
                                 ranges::to_vector;
                p_i->time_list |= ranges::actions::sort;
                p_i->rules_ = user::get_current_handle().get<business::rules>();
                p_i->work_clock_.set_rules(p_i->rules_);
                p_i->refresh_work_clock_();
              }
          );
  p_i->edit_chick.connect([this](const std::tuple<std::int32_t, std::double_t>& in) {
    DOODLE_LOG_INFO("开始设置时间点 {} 增量 {}", std::get<0>(in), chrono::seconds{boost::numeric_cast<doodle::chrono::seconds::rep>(std::get<1>(in))});
    p_i->set_time_point(std::get<0>(in), std::get<1>(in));
  });

  p_i->rules_cache().modify_guard_.connect([this](const business::rules& in) {
    p_i->rules_                                       = in;
    auto l_user                                       = user::get_current_handle();
    user::get_current_handle().get<business::rules>() = in;
    database::save(l_user);
    p_i->work_clock_.set_rules(p_i->rules_);
    p_i->refresh_work_clock_();
    p_i->save();
  });
}

time_sequencer_widget::~time_sequencer_widget() = default;

void time_sequencer_widget::render() {
  ImGui::Checkbox("本地时间", &ImPlot::GetStyle().UseLocalTime);
  ImGui::SameLine();
  ImGui::Checkbox("24 小时制", &ImPlot::GetStyle().Use24HourClock);
  if (p_i->time_list.empty())
    return;

  if (ImGui::Button("提交更新")) p_i->save();

  if (ImPlot::BeginPlot("时间折线图")) {
    /// 设置州为时间轴
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_::ImPlotScale_Time);
    double t_min = doodle::chrono::floor<doodle::chrono::seconds>(
                       p_i->time_list.front().time_point_.get_sys_time()
    )
                       .time_since_epoch()
                       .count();  // 01/01/2021 @ 12:00:00am (UTC)
    double t_max = doodle::chrono::floor<doodle::chrono::seconds>(
                       p_i->time_list.back().time_point_.get_sys_time()
    )
                       .time_since_epoch()
                       .count();  // 01/01/2022 @ 12:00:00am (UTC)
    ImPlot::SetupAxisLimits(ImAxis_X1, t_min, t_max);
    ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond);
    if (p_i->view2_.set_other_view) {
      auto l_rect = p_i->get_view1_rect();
      ImPlot::SetupAxesLimits(l_rect.X.Min, l_rect.X.Max, l_rect.Y.Min, l_rect.Y.Max, ImGuiCond_Always);
      p_i->view2_.set_other_view = false;
    }
    /// \brief 时间折线
    ImPlot::PlotLine("Time Series", p_i->time_list_x.data(), p_i->time_list_y.data(), p_i->time_list.size());
    //    ImPlot::PlotVLines("HLines", p_i->shaded_works_time.data(), p_i->shaded_works_time.size());

    //    {
    //      auto l_guard = p_i->chick_view();
    //
    //      l_guard = ImPlot::IsPlotSelected();
    //    }

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
      if (ImPlot::IsPlotHovered()) {
        auto l_point            = ImPlot::GetPlotMousePos();
        p_i->drag_point_current = p_i->get_iter_DragPoint(l_point.x);
      }
      auto l_guard = p_i->edit_chick();
      for (std::int32_t l_i = p_i->drag_point_current;
           l_i < std::min(p_i->drag_point_current + 2, (std::int32_t)p_i->time_list.size());
           ++l_i) {
        auto l_tmp = boost::numeric_cast<std::double_t>(l_i);
        if (
            l_guard = ImPlot::DragPoint(
                (std::int32_t)l_i,
                (std::double_t*)&(p_i->time_list_x[l_i]),
                &(l_tmp),
                ImVec4{0, 0.9f, 0, 1}
            );
            l_guard
        ) {
          l_guard ^ std::make_tuple(l_i, p_i->time_list_x[l_i]);
          //          p_i->time_list_y[l_i] = l_i;
          p_i->refresh_DragPoint_time_point(l_i, p_i->time_list_x[l_i]);
        };
      }
    }

    ImPlot::EndPlot();
  }
  /// \brief 时间柱状图
  if (ImPlot::BeginPlot("工作柱状图")) {
    if (p_i->view1_.set_other_view) {
      auto l_rect = p_i->get_view2_rect();
      ImPlot::SetupAxesLimits(l_rect.X.Min, l_rect.X.Max, l_rect.Y.Min, l_rect.Y.Max, ImGuiCond_Always);
      p_i->view1_.set_other_view = false;
    }
    ImPlot::PlotBars("Bars", p_i->work_time_plots.data(), p_i->work_time_plots.size());
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

  if (ImGui::Button("平均时间")) p_i->average_time();

  ImGui::Separator();
  dear::Text(p_i->rules_cache.gui_name.name);

  p_i->rules_cache().render();
}
const std::string& time_sequencer_widget::title() const {
  return p_i->title_name_;
}
}  // namespace doodle::gui
