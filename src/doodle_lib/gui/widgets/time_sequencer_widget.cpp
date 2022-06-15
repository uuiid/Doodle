#include "time_sequencer_widget.h"

#include <implot.h>
#include <implot_internal.h>

#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_lib/core/work_clock.h>

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

 public:
  impl(){

  };
  ~impl() = default;
  std::vector<point_cache> time_list{};
  std::vector<double> time_list_x{};
  std::vector<double> time_list_y{};
  time_point_wrap p_min{2021, 1, 1, 0, 0, 0};
  time_point_wrap p_max{2021, 2, 1, 0, 0, 0};

  ImPlotRect rect_{};
  ImPlotRect rect_org_{};

  std::vector<doodle::chrono::hours_double> work_time;
  std::vector<std::double_t> work_time_plots;
  std::vector<std::pair<std::double_t, std::double_t>> work_time_plots_drag;
  std::double_t work_time_plots_max;

  /// \brief 时间规则
  doodle::business::rules rules_{};
  /// \brief 工作时间计算
  doodle::business::work_clock_mfm work_clock_mfm_{};
  /// \brief
  doodle::business::work_next_clock_mfm work_next_clock_mfm{};

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

    auto l_begin = l_list.front().time_point_.zoned_time_.get_local_time();
    work_clock_mfm_.set_time(doodle::chrono::floor<chrono::days>(l_begin));
    work_clock_mfm_.set_states(boost::msm::back::states_ << decltype(work_clock_mfm_)::work_state{});
    work_time = l_list |
                ranges::views::transform(
                    [&](const impl::point_cache& in_time) -> doodle::chrono::hours_double {
                      work_clock_mfm_.start();
                      return work_clock_mfm_.work_duration(in_time.time_point_.zoned_time_.get_local_time(),
                                                           rules_);
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
    refresh_work_time(l_time_list);
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
};

time_sequencer_widget::time_sequencer_widget()
    : p_i(std::make_unique<impl>()) {
  p_i->time_list = ranges::views::ints(1, 100) |
                   ranges::views::transform([](auto&& in) -> impl::point_cache {
                     return impl::point_cache{{}, time_point_wrap{2021, 1, in, 0, 0, 0}};
                   }) |
                   ranges::to_vector;

  p_i->refresh(p_i->time_list);
  p_i->refresh_work_time(p_i->time_list);
  ImPlot::GetStyle().UseLocalTime = true;
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
  /// \brief 时间折线
  if (ImPlot::BeginPlot("##time")) {
    /// 设置州为时间轴
    ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_Time);
    //    double t_min = p_i->p_min_c.time_since_epoch().count();  // 01/01/2021 @ 12:00:00am (UTC)
    //    double t_max = p_i->p_max_c.time_since_epoch().count();  // 01/01/2022 @ 12:00:00am (UTC)
    //    ImPlot::SetupAxesLimits(t_min, t_max, 0, 1);
//    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);

    if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyCtrl) {
      auto l_select = ImPlot::GetPlotSelection();
      p_i->modify_time();
      p_i->find_selects(l_select);
    }

    ImPlot::PlotLine("Time Series",
                     p_i->time_list_x.data(),
                     p_i->time_list_y.data(),
                     p_i->time_list.size());
    if (ImPlot::IsPlotSelected()) {
      if (ImGui::IsMouseClicked(ImPlot::GetInputMap().SelectCancel)) {
        ImPlot::CancelPlotSelection();
      }
    }

    if (p_i->rect_.X.Size() != 0 && p_i->rect_.Y.Size() != 0) {
      if (ImPlot::DragRect(2344,
                           &p_i->rect_.X.Min,
                           &p_i->rect_.Y.Min,
                           &p_i->rect_.X.Max,
                           &p_i->rect_.Y.Max,
                           ImVec4(1, 0, 1, 1)))
        p_i->modify_time_refresh(p_i->rect_);
    }
    /// 绘制可调整点
    //{
    //  auto l_linm = ImPlot::GetPlotLimits().X;
    //  ranges::for_each(p_i->time_list_y,
    //                   [&](std::double_t& in) {
    //                     std::size_t l_i = in;
    //                     if (ImPlot::DragPoint((std::int32_t)l_i,
    //                                           (std::double_t*)&(p_i->time_list_x[l_i]),
    //                                           &(p_i->time_list_y[l_i]), ImVec4{0, 0.9f, 0, 1})) {
    //                       //                           in_item.first                                               = l_i;
    //                       //                           p_i->work_time_plots[boost::numeric_cast<std::size_t>(l_i)] = in_item.second;
    //                     };
    //                   });
    //}

    ImPlot::EndPlot();
  }
  /// \brief 时间柱状图
  if (ImPlot::BeginPlot("Bar Plot")) {
    //    ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);
    ranges::for_each(p_i->work_time_plots_drag,
                     [&](std::pair<std::double_t, std::double_t>& in_item) {
                       auto l_i = in_item.first;
                       if (ImPlot::DragPoint((std::int32_t)in_item.first,
                                             &(in_item.first),
                                             &(in_item.second), ImVec4{0, 0.9f, 0, 1})) {
                         in_item.first                                               = l_i;
                         p_i->work_time_plots[boost::numeric_cast<std::size_t>(l_i)] = in_item.second;
                       };
                     });

    ImPlot::PlotBars("Bars",
                     p_i->work_time_plots.data(),
                     p_i->work_time_plots.size());
    ImPlot::EndPlot();
  }
}
}  // namespace doodle::gui
