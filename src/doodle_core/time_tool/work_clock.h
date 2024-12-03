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

class DOODLE_CORE_API work_clock2 {
 public:
  using time_type     = chrono::local_time_pos;
  using duration_type = time_type::duration;
  using info_type     = std::set<std::string>;

 private:
  using discrete_interval_time = boost::icl::discrete_interval<time_type>;
  using interval_set_time      = boost::icl::interval_set<time_type>;
  using interval_map_time      = boost::icl::interval_map<time_type, info_type>;
  interval_set_time interval_set_time_;
  interval_map_time interval_map_time_;

 public:
  work_clock2();

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
  template <typename Duration>
  time_type next_time(const time_type& in_begin, const Duration& in_du) const {
    return next_time(in_begin, chrono::duration_cast<duration_type>(in_du));
  }

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
   * @param in_min 开始时间
   * @param in_max 结束时间
   * @return 时间段和相应的备注
   */
  std::vector<std::tuple<time_type, time_type, std::string>> get_time_info(
      const time_type& in_min, const time_type& in_max
  ) const;
  /**
   * 这个是添加额外的信息, 并不会加入到计算时间中, 只会添加一个额外的辅助信息时间
   * @param in_time
   */
  void add_info(const std::tuple<time_type, time_type, std::string>& in_time);

  /**
   * @brief 寻找下一段工作开始的时间点
   *
   * @param in_point
   * @return time_type
   */
  time_type next_point(const time_type& in_point);

  // /**
  //  * @brief 这个绝对扣除是扣除的每天 12:00 到13:00 还有 6:00 到 6:30
  //  *
  //  */
  // void absolute_deduction();

  work_clock2& operator+=(const std::tuple<time_type, time_type>& in_time);
  work_clock2& operator+=(const std::tuple<time_type, time_type, std::string>& in_time);
  work_clock2& operator-=(const std::tuple<time_type, time_type>& in_time);
  work_clock2& operator-=(const std::tuple<time_type, time_type, std::string>& in_time);

  template <typename Duration>
  work_clock2& operator+=(const std::tuple<chrono::local_time<Duration>, chrono::local_time<Duration>>& in_time) {
    *this += std::make_tuple(
        chrono::time_point_cast<duration_type>(std::get<0>(in_time)),
        chrono::time_point_cast<duration_type>(std::get<1>(in_time))
    );
    return *this;
  }
  template <typename Duration>
  work_clock2& operator+=(
      const std::tuple<chrono::local_time<Duration>, chrono::local_time<Duration>, std::string>& in_time
  ) {
    *this += std::make_tuple(
        chrono::time_point_cast<duration_type>(std::get<0>(in_time)),
        chrono::time_point_cast<duration_type>(std::get<1>(in_time)), std::get<2>(in_time)
    );
    return *this;
  }
  template <typename Duration>
  work_clock2& operator-=(const std::tuple<chrono::local_time<Duration>, chrono::local_time<Duration>>& in_time) {
    *this -= std::make_tuple(
        chrono::time_point_cast<duration_type>(std::get<0>(in_time)),
        chrono::time_point_cast<duration_type>(std::get<1>(in_time))
    );
    return *this;
  }
  template <typename Duration>
  work_clock2& operator-=(
      const std::tuple<chrono::local_time<Duration>, chrono::local_time<Duration>, std::string>& in_time
  ) {
    *this -= std::make_tuple(
        chrono::time_point_cast<duration_type>(std::get<0>(in_time)),
        chrono::time_point_cast<duration_type>(std::get<1>(in_time)), std::get<2>(in_time)
    );
    return *this;
  }

  template <typename Duration>
  work_clock2& operator+=(const std::tuple<chrono::zoned_time<Duration>, chrono::zoned_time<Duration>>& in_time) {
    *this += std::make_tuple(
        chrono::time_point_cast<duration_type>(std::get<0>(in_time).get_local_time()),
        chrono::time_point_cast<duration_type>(std::get<1>(in_time).get_local_time())
    );
    return *this;
  }
  template <typename Duration>
  work_clock2& operator+=(
      const std::tuple<chrono::zoned_time<Duration>, chrono::zoned_time<Duration>, std::string>& in_time
  ) {
    *this += std::make_tuple(
        chrono::time_point_cast<duration_type>(std::get<0>(in_time).get_local_time()),
        chrono::time_point_cast<duration_type>(std::get<1>(in_time).get_local_time()), std::get<2>(in_time)
    );
    return *this;
  }
  template <typename Duration>
  work_clock2& operator-=(const std::tuple<chrono::zoned_time<Duration>, chrono::zoned_time<Duration>>& in_time) {
    *this -= std::make_tuple(
        chrono::time_point_cast<duration_type>(std::get<0>(in_time).get_local_time()),
        chrono::time_point_cast<duration_type>(std::get<1>(in_time).get_local_time())
    );
    return *this;
  }
  template <typename Duration>
  work_clock2& operator-=(
      const std::tuple<chrono::zoned_time<Duration>, chrono::zoned_time<Duration>, std::string>& in_time
  ) {
    *this -= std::make_tuple(
        chrono::time_point_cast<duration_type>(std::get<0>(in_time).get_local_time()),
        chrono::time_point_cast<duration_type>(std::get<1>(in_time).get_local_time()), std::get<2>(in_time)
    );
    return *this;
  }

  std::string debug_print() const;
};

}  // namespace doodle::business
