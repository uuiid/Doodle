#include "time_sequencer_widget.h"

#include <implot.h>
#include <implot_internal.h>

#include <doodle_core/metadata/time_point_wrap.h>

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
  impl() {
    p_min_c = doodle::chrono::floor<doodle::chrono::seconds>(
        p_min.zoned_time_.get_sys_time());
    p_max_c = doodle::chrono::floor<doodle::chrono::seconds>(
        p_max.zoned_time_.get_sys_time());
  };
  ~impl() = default;
  std::vector<point_cache> time_list{};
  std::vector<double> time_list_x{};
  std::vector<double> time_list_y{};
  time_point_wrap p_min{2021, 1, 1, 0, 0, 0};
  time_point_wrap p_max{2021, 2, 1, 0, 0, 0};

  doodle::chrono::sys_seconds p_min_c{};
  doodle::chrono::sys_seconds p_max_c{};
  ImPlotRect rect_{};
  ImPlotRect rect_org_{};

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

  void modify_time_refresh(const ImPlotRect& in_rect) {
    std::int64_t l_mod_size = in_rect.X.Min - rect_org_.X.Min;
    if (l_mod_size == 0)
      return;
    auto l_time_list = time_list | ranges::views::transform([&](impl::point_cache in) -> impl::point_cache {
                         if (in.has_select) {
                           in.time_point_ += doodle::chrono::seconds{l_mod_size};
                         }
                         return in;
                       }) |
                       ranges::to_vector;
    //    ranges::for_each(time_list, [&](impl::point_cache& in) {
    //      if (in.has_select) {
    //        in.time_point_ += doodle::chrono::seconds{l_mod_size};
    //      }
    //    });
    //            time_list |= ranges::actions::sort;
    rect_ = in_rect;
    refresh(l_time_list);
  };
  void modify_time() {
    std::int64_t l_mod_size = rect_.X.Min - rect_org_.X.Min;
    if (l_mod_size == 0)
      return;
    ranges::for_each(time_list, [&](impl::point_cache& in) {
      if (in.has_select) {
        in.time_point_ += doodle::chrono::seconds{l_mod_size};
      }
    });
    time_list |= ranges::actions::sort;
    refresh(time_list);
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
  if (ImPlot::BeginPlot("##time")) {
    ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_Time);
    //    double t_min = p_i->p_min_c.time_since_epoch().count();  // 01/01/2021 @ 12:00:00am (UTC)
    //    double t_max = p_i->p_max_c.time_since_epoch().count();  // 01/01/2022 @ 12:00:00am (UTC)
    //    ImPlot::SetupAxesLimits(t_min, t_max, 0, 1);
    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);

    if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(0) && ImGui::GetIO().KeyCtrl) {
      auto l_select = ImPlot::GetPlotSelection();
      p_i->modify_time();
      p_i->find_selects(l_select);
    }

    ImPlot::PlotLine("Time Series", p_i->time_list_x.data(),
                     p_i->time_list_y.data(),
                     p_i->time_list.size());
    if (ImPlot::IsPlotSelected()) {
      //      ImPlotPoint centroid = FindCentroid(data, select, cnt);
      //      if (cnt > 0) {
      //        ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6);
      //        ImPlot::PlotScatter("Centroid", &centroid.x, &centroid.y, 1);
      //      }
      if (ImGui::IsMouseClicked(ImPlot::GetInputMap().SelectCancel)) {
        ImPlot::CancelPlotSelection();
      }
    }
    if (p_i->rect_.X.Size() != 0 && p_i->rect_.Y.Size() != 0) {
      p_i->modify_time_refresh(p_i->rect_);
      ImPlot::DragRect(2344, &p_i->rect_.X.Min, &p_i->rect_.Y.Min, &p_i->rect_.X.Max, &p_i->rect_.Y.Max, ImVec4(1, 0, 1, 1));
    }
    //    double t_now = time_point_wrap{}.zoned_time_.get_sys_time().time_since_epoch().count();
    //    double y_now = HugeTimeData::GetY(t_now);
    //    ImPlot::PlotScatter("Now", &t_now, &y_now, 1);
    //    ImPlot::Annotation(t_now, y_now, ImPlot::GetLastItemColor(), ImVec2(10, 10), false, "Now");
    ImPlot::EndPlot();
  }
}
}  // namespace doodle::gui
