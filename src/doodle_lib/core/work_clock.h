//
// Created by TD on 2022/4/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <bitset>
#include <utility>

#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/functor_row.hpp>
namespace doodle {

namespace business {

namespace detail {
namespace bmsm = boost::msm::front;

struct normal_work_begin {
  doodle::chrono::local_time_pos time_;
};
struct normal_work_end {
  doodle::chrono::local_time_pos time_;
};
struct adjust_work_begin {
  doodle::chrono::local_time_pos time_;
};
struct adjust_work_end {
  doodle::chrono::local_time_pos time_;
};

struct adjust_rest_begin {
  doodle::chrono::local_time_pos time_;
};
struct adjust_rest_end {
  doodle::chrono::local_time_pos time_;
};

struct work_machine_front : public bmsm::state_machine_def<work_machine_front> {
  doodle::chrono::local_time_pos time_;
  chrono::seconds work_time_;
  //  std::optional<chrono::seconds> work_limit_;

  /// \brief 工作状态
  struct work_state : public bmsm::state<> {
  };
  /// \brief 休息状态
  struct rest_state : public bmsm::state<> {
  };
  typedef rest_state initial_state;

  virtual void add_time(const doodle::chrono::local_time_pos& in_time);

  //  inline explicit operator bool() const {
  //    return ok();
  //  }
  /// \brief 动作
  struct work_to_rest {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const& in_evt, FSM& in_fsm, SourceState&, TargetState&) {
      if (in_evt.time_ != in_fsm.time_) {
        in_fsm.add_time(in_evt.time_);
//        in_fsm.time_ = in_evt.time_;
      }
    }
  };

  struct rest_to_work {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const& in_evt, FSM& in_fsm, SourceState&, TargetState&) {
      if (in_evt.time_ != in_fsm.time_) {
        in_fsm.time_ = in_evt.time_;
      }
    }
  };

  struct transition_table
      : boost::mpl::vector<
            //            Start          Event         Next      Action               Guard
            // 正确的转换
            bmsm::Row<work_state, normal_work_end, rest_state, work_to_rest>,    // 正常工作结束
            bmsm::Row<work_state, adjust_rest_begin, rest_state, work_to_rest>,  // 调休开始
            bmsm::Row<work_state, adjust_work_end, rest_state, work_to_rest>,    // 加班结束

            bmsm::Row<rest_state, normal_work_begin, work_state, rest_to_work>,  // 正常工作开始
            bmsm::Row<rest_state, adjust_rest_end, work_state, rest_to_work>,    // 调休结束
            bmsm::Row<rest_state, adjust_work_begin, work_state, rest_to_work>   // 加班开始
            > {};
};

using work_clock_mfm_base = boost::msm::back::state_machine<work_machine_front>;
class work_clock_mfm;
}  // namespace detail

namespace work_attr {
/**
 * @brief
 *  - true, false
 *  - 正常,  调整
 *  - 工作,  休息
 *  - 开始,  结束
 */
using time_state = std::bitset<3>;

constexpr const static time_state normal_work_begin{0b111};
constexpr const static time_state normal_work_end{0b110};
constexpr const static time_state adjust_work_begin{0b011};
constexpr const static time_state adjust_work_end{0b010};

constexpr const static time_state adjust_rest_begin{0b001};
constexpr const static time_state adjust_rest_end{0b000};
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

  void add_event(doodle::business::detail::work_clock_mfm_base& in_mfm);

  bool operator<(const time_attr& in_rhs) const;
  bool operator>(const time_attr& in_rhs) const;
  bool operator<=(const time_attr& in_rhs) const;
  bool operator>=(const time_attr& in_rhs) const;
  bool operator==(const time_attr& in_rhs) const;
  bool operator!=(const time_attr& in_rhs) const;
};

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
  std::vector<adjust> extra_work{};
  std::vector<adjust> extra_rest{};

  /**
   * @brief 获取当天的工作时间段
   * @param in_day 传入的时间(一天)
   * @return
   */
  std::vector<time_attr> operator()(const chrono::year_month_day& in_day) const;

  void clamp_time(chrono::local_time_pos& in_time_pos) const;
};

namespace detail {
class work_clock_mfm : public work_clock_mfm_base {
 public:
  work_clock_mfm() = default;

  chrono::hours_double work_duration(const chrono::local_time_pos& in_e,
                                     const business::rules& in_rules);
};
class work_next_clock_mfm : public work_clock_mfm_base {
  chrono::seconds work_limit_;

 public:
  work_next_clock_mfm() = default;

  void add_time(const doodle::chrono::local_time_pos& in_time) override;
  chrono::local_time_pos next_time(const chrono::milliseconds& in_du_time,
                                   const business::rules& in_rules);
  void set_time(const chrono::local_time_pos& in_pos);
  [[nodiscard]] bool ok() const;
};

}  // namespace detail

}  // namespace business
namespace detail {

chrono::hours_double work_duration(
    const chrono::local_time_pos& in_s,
    const chrono::local_time_pos& in_e,
    const business::rules& in_rules);

chrono::local_time_pos next_time(
    const chrono::local_time_pos& in_s,
    const chrono::milliseconds& in_du_time,
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
