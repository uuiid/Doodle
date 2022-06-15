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

  /// \brief 工作状态
  struct work_state : public bmsm::state<> {};
  /// \brief 休息状态
  struct rest_state : public bmsm::state<> {};
  struct holiday_state : public bmsm::state<> {};
  struct adjust_state : public bmsm::state<> {};
  struct overtime_state : public bmsm::state<> {};

  typedef rest_state initial_state;

  /**
   * @brief 状态机开始
   * @tparam Event
   * @tparam FSM
   */
  template <class Event, class FSM>
  void on_entry(Event const&, FSM& in_fsm) {
    work_time_ = {};
    in_fsm.set_states(boost::msm::back::states_ << rest_state{});
  }
  /**
   * @brief 状态机结束
   * @tparam Event
   * @tparam FSM
   */
  template <class Event, class FSM>
  void on_exit(Event const&, FSM&) {
  }

  virtual void add_time(const doodle::chrono::local_time_pos& in_time);
  void set_time_(const chrono::local_time_pos& in_pos);

  template <typename Duration_>
  void set_time(const chrono::time_point<chrono::local_t, Duration_>& in_time) {
    set_time_(chrono::floor<chrono::seconds>(in_time));
  };
  //  inline explicit operator bool() const {
  //    return ok();
  //  }
  /// \brief 动作
  struct work_to_rest {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const& in_evt, FSM& in_fsm, SourceState&, TargetState&) {
      if (in_fsm.time_ < in_evt.time_) {
        in_fsm.add_time(in_evt.time_);
        //        in_fsm.time_ = in_evt.time_;
      }
    }
  };

  struct rest_to_work {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const& in_evt, FSM& in_fsm, SourceState&, TargetState&) {
      if (in_fsm.time_ < in_evt.time_) {
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

  //  template <class FSM, class Event>
  //  void no_transition(Event const&, FSM&, int) {
  //
  //  }
};

using work_clock_mfm_base = boost::msm::back::state_machine<work_machine_front>;
class work_clock_mfm;

inline void print_work_clock_mfm(const work_clock_mfm_base& in_mfm) {
  typedef boost::msm::back::generate_state_set<work_clock_mfm_base::stt>::type all_states;  // states
  static char const* state_names[boost::mpl::size<all_states>::value];
  // array to fill with names
  // fill the names of the states defined in the state machine
  boost::mpl::for_each<all_states, boost::msm::wrap<boost::mpl::placeholders::_1>>(
      boost::msm::back::fill_state_names<work_clock_mfm_base::stt>(state_names));
  // display all active states
  //  for (unsigned int i = 0; i < some_fsm::nr_regions::value; ++i) {
  //    std::cout << " -> "
  //              << state_names[my_fsm_instance.current_state()[i]]
  //              << std::endl;
  //  }
  DOODLE_LOG_INFO(state_names[in_mfm.current_state()[0]])
}
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
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>> extra_holidays{};
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>> extra_work{};
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>> extra_rest{};

  /**
   * @brief 获取当天的工作时间段
   * @param in_day 传入的时间(一天)
   * @return
   */
  std::vector<time_attr> operator()(const chrono::year_month_day& in_day) const;

  void clamp_time(chrono::local_time_pos& in_time_pos) const;
  /// \brief 正常的工作(需要第一步收集)
  /// \param in_s
  /// \param in_e
  /// \return
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>
  normal_works(const chrono::year_month_day& in_day) const;
  /// \brief 节假日(需要减去)
  /// \param in_s
  /// \param in_e
  /// \return
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>
  holidays(const chrono::year_month_day& in_day) const;
  /**
   * @brief 调休
   * @param in_s
   * @param in_e
   * @return
   */
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>
  adjusts(const chrono::year_month_day& in_day) const;
  /**
   * @brief 加班(需要加上)
   * @param in_s
   * @param in_e
   * @return
   */
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>
  overtimes(const chrono::year_month_day& in_day) const;
};

namespace detail {
class work_clock_mfm : public work_clock_mfm_base {
 public:
  work_clock_mfm() = default;

  chrono::hours_double work_duration_(const chrono::local_time_pos& in_e,
                                      const business::rules& in_rules);

  template <typename Duration_>
  chrono::hours_double work_duration(const chrono::time_point<chrono::local_t, Duration_>& in_e,
                                     const business::rules& in_rules) {
    return work_duration_(chrono::floor<chrono::seconds>(in_e), in_rules);
  }
};
class work_next_clock_mfm : public work_clock_mfm_base {
  chrono::seconds work_limit_;

 public:
  work_next_clock_mfm() = default;

  void add_time(const doodle::chrono::local_time_pos& in_time) override;

  chrono::local_time_pos next_time_(const chrono::milliseconds& in_du_time,
                                    const business::rules& in_rules);
  template <typename Duration_>
  chrono::local_time_pos next_time(const Duration_& in_e,
                                   const business::rules& in_rules) {
    return next_time_(chrono::floor<chrono::milliseconds>(in_e), in_rules);
  }
  [[nodiscard]] bool ok() const;
};

}  // namespace detail

using work_clock_mfm      = doodle::business::detail::work_clock_mfm;
using work_next_clock_mfm = doodle::business::detail::work_next_clock_mfm;

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
