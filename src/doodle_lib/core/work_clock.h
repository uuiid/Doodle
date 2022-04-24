//
// Created by TD on 2022/4/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <bitset>

namespace doodle {

namespace business {

namespace work_attr {
/**
 * @brief
 *  - true, false
 *  - 工作,  休息
 */
using time_state = std::bitset<1>;

constexpr const static time_state work_begin{0b1};
constexpr const static time_state work_end{0b0};
constexpr const static time_state rest_begin{0b0};
constexpr const static time_state rest_end{0b1};
}  // namespace work_attr

class DOODLELIB_API adjust {
 public:
  chrono::local_time_pos start_;
  chrono::local_time_pos end_;
};

class DOODLELIB_API time_attr {
 public:
  time_attr() = default;
  explicit time_attr(const chrono::local_time_pos& in_pos,
                     const work_attr::time_state& in_state)
      : time_point(in_pos),
        state_(in_state){};
  chrono::local_time_pos time_point{};

  /**
   * @brief 时间状态
   */
  work_attr::time_state state_{};
  constexpr const static work_attr::time_state work_begin = work_attr::work_begin;
  constexpr const static work_attr::time_state work_end   = work_attr::work_end;
  constexpr const static work_attr::time_state rest_begin = work_attr::rest_begin;
  constexpr const static work_attr::time_state rest_end   = work_attr::rest_end;
  bool operator<(const time_attr& in_rhs) const;
  bool operator>(const time_attr& in_rhs) const;
  bool operator<=(const time_attr& in_rhs) const;
  bool operator>=(const time_attr& in_rhs) const;
  bool operator==(const time_attr& in_rhs) const;
  bool operator!=(const time_attr& in_rhs) const;
};

class DOODLELIB_API rules {
 public:
  /// \brief 工作日 从周一到周日
  std::bitset<7> work_weekdays{};
  std::set<std::pair<
      chrono::seconds,
      chrono::seconds>>
      work_pair{};
  std::vector<adjust> extra_work{};
  std::vector<adjust> extra_rest{};
  std::optional<chrono::local_time_pos> get_work_time(const chrono::local_time_pos& in_s);

  /**
   * @brief 获取当天的工作时间段
   * @param in_day 传入的天数
   * @return
   */
  std::vector<time_attr> operator()(const chrono::year_month_day& in_day) const;
};

class DOODLELIB_API work_clock {
 public:
  explicit work_clock(chrono::local_time_pos in_pos)
      : time_point(in_pos),
        work_time_(),
        state_list(),
        work_limit_(){};

 private:
  std::optional<chrono::seconds> work_limit_;

 public:
  chrono::local_time_pos time_point{};
  chrono::seconds work_time_;
  std::vector<work_attr::time_state> state_list{};

  void set_work_limit(const chrono::local_time_pos& in_pos,
                      const chrono::seconds& in_work_du);
  chrono::seconds work_time() const;
  work_clock& operator+=(const time_attr& in_attr);
  bool ok() const;
  inline explicit operator bool() const {
    return ok();
  }
};

}  // namespace business

chrono::hours_double work_duration(
    const chrono::local_time_pos& in_s,
    const chrono::local_time_pos& in_e,
    const business::rules& in_rules);

chrono::local_time_pos next_time(
    const chrono::local_time_pos& in_s,
    const chrono::milliseconds& in_du_time,
    const business::rules& in_rules);
}  // namespace doodle
