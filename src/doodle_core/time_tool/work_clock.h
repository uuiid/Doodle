//
// Created by TD on 2022/4/1.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <bitset>
#include <utility>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/rules.h>

#include <doodle_core/lib_warp/boost_icl_warp.h>

#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>

namespace doodle {

namespace business {

class DOODLELIB_API work_clock {
  rules rules_;
  using time_d_t               = doodle::chrono::local_time_pos;
  using info_type              = std::set<std::string>;
  using discrete_interval_time = boost::icl::discrete_interval<time_d_t>;
  using interval_set_time      = boost::icl::interval_set<time_d_t>;
  using interval_map_time      = boost::icl::interval_map<time_d_t, info_type>;

  void gen_rules_(const discrete_interval_time& in_time);
  void generate_interval_map_time_(const discrete_interval_time& in_time);
  interval_set_time interval_set_time_;
  interval_map_time interval_map_time_;

 public:
  work_clock();

  void set_rules(const rules& in_rules);
  void set_interval(const chrono::local_time_pos& in_min,
                    const chrono::local_time_pos& in_max);
  /**
   * @brief 设置工作时间时钟的开始和结束(缓存)
   * @param in_min
   * @param in_max
   */
  inline void set_interval(const doodle::time_point_wrap& in_min,
                           const doodle::time_point_wrap& in_max) {
    set_interval(doodle::chrono::floor<chrono::local_time_pos::duration>(in_min.zoned_time_.get_local_time()),
                 doodle::chrono::floor<chrono::local_time_pos::duration>(in_max.zoned_time_.get_local_time()));
  };

  /**
   * @brief 获取两个时间点点工作时间(按照规则获取)
   * @param in_min 开始时间
   * @param in_max 结束时间
   * @return 工作时间
   */
  chrono::hours_double operator()(const chrono::local_time_pos& in_min,
                                  const chrono::local_time_pos& in_max) const;

  inline chrono::hours_double operator()(const doodle::time_point_wrap& in_min,
                                         const doodle::time_point_wrap& in_max) const {
    return (*this)(doodle::chrono::floor<chrono::local_time_pos::duration>(in_min.zoned_time_.get_local_time()),
                   doodle::chrono::floor<chrono::local_time_pos::duration>(in_max.zoned_time_.get_local_time()));
  };
  /**
   * @copybrief chrono::hours_double operator()(const chrono::local_time_pos& in_min, const chrono::local_time_pos& in_max) const
   * @tparam Duration_  模板时间段
   */
  template <typename Duration_, std::enable_if_t<
                                    !std::is_same_v<chrono::time_point<chrono::local_t, Duration_>,
                                                    chrono::local_time_pos>,
                                    bool> = true>
  chrono::hours_double operator()(
      const chrono::time_point<chrono::local_t, Duration_>& in_s,
      const chrono::time_point<chrono::local_t, Duration_>& in_e) {
    return (*this)(
        chrono::floor<chrono::seconds>(in_s),
        chrono::floor<chrono::seconds>(in_e));
  };

  /**
   * @brief 根据传入的开始时间和工作时间段获取下一个时间点
   * @param in_begin 开始时间
   * @param in_du 时间段
   * @return 下一个时间点
   */
  chrono::local_time_pos next_time(const chrono::local_time_pos& in_begin,
                                   const chrono::local_time_pos::duration& in_du) const;

  template <typename Duration_, typename Duration2_,
            std::enable_if_t<
                !std::is_same_v<chrono::time_point<chrono::local_t, Duration_>,
                                chrono::local_time_pos>,
                bool> = true>
  chrono::time_point<chrono::local_t, Duration_> next_time(
      const chrono::time_point<chrono::local_t, Duration_>& in_s,
      const Duration2_& in_du_time) {
    return next_time(
        chrono::floor<chrono::seconds>(in_s),
        chrono::floor<chrono::seconds>(in_du_time));
  };

  template <typename Duration_,
            std::enable_if_t<
                !std::is_same_v<chrono::time_point<chrono::local_t, Duration_>,
                                chrono::local_time_pos>,
                bool> = true>
  chrono::time_point<chrono::local_t, Duration_> next_time(
      const time_point_wrap& in_s,
      const Duration_& in_du_time) {
    return next_time(
        chrono::floor<chrono::seconds>(in_s.zoned_time_.get_local_time()),
        chrono::floor<chrono::seconds>(in_du_time));
  };

  /**
   * @brief 获取两个时间点之间点时间分段( 休息时间段 -> 工作时间段)
   * @param in_min 开始时间
   * @param in_max 结束时间
   * @return 时间段
   */
  [[nodiscard("")]] std::vector<std::pair<time_d_t, time_d_t>> get_work_du(
      const chrono::local_time_pos& in_min,
      const chrono::local_time_pos& in_max) const;

  [[nodiscard("")]] inline auto get_work_du(const doodle::time_point_wrap& in_min,
                                            const doodle::time_point_wrap& in_max) const {
    return get_work_du(doodle::chrono::floor<chrono::local_time_pos::duration>(in_min.zoned_time_.get_local_time()),
                       doodle::chrono::floor<chrono::local_time_pos::duration>(in_max.zoned_time_.get_local_time()));
  };

  /**
   * @brief 获取当前点所在时间段段的备注
   * @param in_time 时间点
   * @return 可选段备注
   */
  std::optional<std::string> get_time_info(
      const doodle::time_point_wrap& in_min,
      const doodle::time_point_wrap& in_max);

  std::string debug_print();
};
}  // namespace business
namespace detail {
chrono::hours_double work_duration(
    const chrono::local_time_pos& in_s,
    const chrono::local_time_pos& in_e,
    const business::rules& in_rules);

chrono::local_time_pos next_time(
    const chrono::local_time_pos& in_s,
    const chrono::local_time_pos::duration& in_du_time,
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


