//
// Created by TD on 2022/4/1.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_icl_warp.h>
#include <doodle_core/metadata/detail/boost_lcl_time_point_adaptation.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>

#include <bitset>
#include <utility>

namespace doodle::business {

class DOODLE_CORE_API work_clock {
  using time_type              = doodle::time_point_wrap;
  using duration_type          = doodle::time_point_wrap::duration;
  using info_type              = std::set<std::string>;
  using discrete_interval_time = boost::icl::discrete_interval<time_type>;
  using interval_set_time      = boost::icl::interval_set<time_type>;
  using interval_map_time      = boost::icl::interval_map<time_type, info_type>;

  interval_set_time interval_set_time_;
  interval_map_time interval_map_time_;

 public:
  work_clock();

  /**
   * @brief 设置工作时间时钟的开始和结束(缓存)
   * @param in_min
   * @param in_max
   */
  void cut_interval(const time_type& in_min, const time_type& in_max);

  /**
   * @brief 获取两个时间点点工作时间(按照规则获取)
   * @param in_min 开始时间
   * @param in_max 结束时间
   * @return 工作时间
   */
  duration_type operator()(const time_type& in_min, const time_type& in_max) const;

  /**
   * @brief 根据传入的开始时间和工作时间段获取下一个时间点
   * @param in_begin 开始时间
   * @param in_du 时间段
   * @return 下一个时间点
   */
  time_type next_time(const time_type& in_begin, const duration_type& in_du) const;

  /**
   * @brief 获取两个时间点之间点时间分段( 休息时间段 -> 工作时间段)
   * @param in_min 开始时间
   * @param in_max 结束时间
   * @return 时间段
   */
  [[nodiscard("")]] std::vector<std::pair<time_type, time_type>> get_work_du(
      const time_type& in_min, const time_type& in_max
  ) const;

  /**
   * @brief 获取当前点所在时间段段的备注
   * @param in_time 时间点
   * @return 可选段备注
   */
  std::optional<std::string> get_time_info(const time_type& in_min, const time_type& in_max) const;
  /**
   * 这个是添加额外的信息, 并不会加入到计算时间中, 只会添加一个额外的辅助信息时间
   * @param in_time
   */
  void add_info(const std::tuple<time_point_wrap, time_point_wrap, std::string>& in_time);

  /**
   * @brief 寻找下一段工作开始的时间点
   *
   * @param in_point
   * @return time_point_wrap
   */
  time_type next_point(const time_point_wrap& in_point);

  // /**
  //  * @brief 这个绝对扣除是扣除的每天 12:00 到13:00 还有 6:00 到 6:30
  //  *
  //  */
  // void absolute_deduction();

  work_clock& operator+=(const std::tuple<time_point_wrap, time_point_wrap>& in_time);
  work_clock& operator+=(const std::tuple<time_point_wrap, time_point_wrap, std::string>& in_time);
  work_clock& operator-=(const std::tuple<time_point_wrap, time_point_wrap>& in_time);
  work_clock& operator-=(const std::tuple<time_point_wrap, time_point_wrap, std::string>& in_time);
  std::string debug_print() const;
};
}  // namespace doodle::business
