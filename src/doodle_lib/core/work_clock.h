//
// Created by TD on 2022/4/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <bitset>
#include <utility>

#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/tools.hpp>
#include <boost/msm/front/functor_row.hpp>

#include <doodle_lib/lib_warp/boost_icl_warp.h>
#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/discrete_interval.hpp>

namespace doodle {

namespace business {

class DOODLELIB_API rules {
 public:
  /// \brief 周六 ->周日(index 6->0)
  constexpr static std::bitset<7> work_Monday_to_Friday{0b0111110};
  constexpr static std::pair<chrono::seconds,
                             chrono::seconds>
      work_9_12{9h, 12h};
  constexpr static std::pair<chrono::seconds,
                             chrono::seconds>
      work_13_18{13h, 18h};

  explicit rules(const std::bitset<7>& in_work_day = work_Monday_to_Friday,
                 std::vector<std::pair<
                     chrono::seconds,
                     chrono::seconds>>
                     in_work_time = std::vector<std::pair<
                         chrono::seconds,
                         chrono::seconds>>{work_9_12, work_13_18})
      : work_weekdays(in_work_day),
        work_pair(std::move(in_work_time)),
        extra_work(),
        extra_rest() {}

  /// \brief 工作日 从周一到周日
  std::bitset<7> work_weekdays{};
  std::vector<std::pair<
      chrono::seconds,
      chrono::seconds>>
      work_pair{};
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>> extra_holidays{};
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>> extra_work{};
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>> extra_rest{};
};

class DOODLELIB_API work_clock {
  rules rules_;
  using time_d_t               = doodle::chrono::local_time_pos;
  using discrete_interval_time = boost::icl::discrete_interval<time_d_t>;
  using interval_set_time      = boost::icl::interval_set<time_d_t>;

  void gen_rules_(const discrete_interval_time& in_time);
  interval_set_time interval_set_time_;

 public:
  work_clock();

  void set_rules(const rules& in_rules);
  void set_interval(const chrono::local_time_pos& in_min,
                    const chrono::local_time_pos& in_max);

  chrono::hours_double operator()(const chrono::local_time_pos& in_min,
                                  const chrono::local_time_pos& in_max) const;

  chrono::local_time_pos next_time(const chrono::local_time_pos& in_begin,
                                   const chrono::local_time_pos::duration& in_du) const;
};

}  // namespace business
namespace detail {
chrono::hours_double work_duration(
    const chrono::local_time_pos& in_s,
    const chrono::local_time_pos& in_e,
    const business::rules& in_rules);

chrono::local_time_pos next_time(
    const chrono::local_time_pos& in_s,
    const chrono::local_time_pos::duration & in_du_time,
    const business::rules& in_rules);
}  // namespace detail

template <typename Duration_>
chrono::hours_double work_duration(
    const chrono::time_point<chrono::local_t, Duration_>& in_s,
    const chrono::time_point<chrono::local_t, Duration_>& in_e,
    const business::rules& in_rules) {
  return detail::work_duration(
      chrono::floor<chrono::seconds>(in_s),
      chrono::floor<chrono::seconds>(in_e),
      in_rules);
};

template <typename Duration_, typename Duration2_>
chrono::time_point<chrono::local_t, Duration_> next_time(
    const chrono::time_point<chrono::local_t, Duration_>& in_s,
    const Duration2_& in_du_time,
    const business::rules& in_rules) {
  return detail::next_time(
      chrono::floor<chrono::seconds>(in_s),
      chrono::floor<chrono::seconds>(in_du_time),
      in_rules);
};
}  // namespace doodle
