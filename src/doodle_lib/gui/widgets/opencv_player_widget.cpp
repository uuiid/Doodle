//
// Created by TD on 2021/11/04.
//

#include "opencv_player_widget.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/file_warp/opencv_read_player.h>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>

namespace doodle {

namespace opencv_player_ns {
namespace msm = boost::msm;
using namespace boost::msm::front::euml;

struct play {};
struct stop {};
struct pause {};
struct open_file {
  entt::entity p_ent;
};

struct player_ : public msm::front::state_machine_def<player_> {
  struct empty : public msm::front::state<> {
    template <class Event, class FSM>
    void on_entry(Event const&, FSM&) {
      DOODLE_LOG_INFO("empty");
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&) {
      DOODLE_LOG_INFO("empty");
    }
    struct internal_guard_fct {
      template <class EVT, class FSM, class SourceState, class TargetState>
      bool operator()(EVT const& evt, FSM&, SourceState&, TargetState&) {
        std::cout << "Empty::internal_transition_table guard\n";
        return false;
      }
    };
    struct internal_action_fct {
      template <class EVT, class FSM, class SourceState, class TargetState>
      void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
        std::cout << "Empty::internal_transition_table action" << std::endl;
      }
    };
    // Transition table for Empty
    struct internal_transition_table
        : boost::mpl::vector<
              // clang-format off
              //                   Start      Event         Next      Action                Guard
              //                   +---------+-------------+---------+---------------------+----------------------+
              msm::front::Internal<           open_file,              internal_action_fct,  internal_guard_fct>
              // clang-format on

              > {};
  };

  struct stopped : public msm::front::state<> {
  };

  struct playing : public msm::front::state<> {
  };

  struct paused : public msm::front::state<> {
  };
  ///  必须定义初始状态
  using initial_state = empty;

  /// 开始定义转换动作
  struct start_playback {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
      std::cout << "player::start_playback" << std::endl;
    }
  };
  struct open_drawer {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
      std::cout << "player::open_drawer" << std::endl;
    }
  };
  struct close_drawer {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
      std::cout << "player::close_drawer" << std::endl;
    }
  };
  struct store_cd_info {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&) {
      std::cout << "player::store_cd_info" << std::endl;
      fsm.process_event(play());
    }
  };
  struct stop_playback {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
      std::cout << "player::stop_playback" << std::endl;
    }
  };
  struct pause_playback {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
      std::cout << "player::pause_playback" << std::endl;
    }
  };
  struct resume_playback {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
      std::cout << "player::resume_playback" << std::endl;
    }
  };
  struct stop_and_open {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
      std::cout << "player::stop_and_open" << std::endl;
    }
  };
  struct stopped_again {
    template <class EVT, class FSM, class SourceState, class TargetState>
    void operator()(EVT const&, FSM&, SourceState&, TargetState&) {
      std::cout << "player::stopped_again" << std::endl;
    }
  };

  // 定义守卫

  struct dummy_guard {
    template <class EVT, class FSM, class SourceState, class TargetState>
    bool operator()(EVT const& evt, FSM& fsm, SourceState& src, TargetState& tgt) {
      return true;
    }
  };
  struct good_disk_format {
    template <class EVT, class FSM, class SourceState, class TargetState>
    bool operator()(EVT const& evt, FSM&, SourceState&, TargetState&) {
      // to test a guard condition, let's say we understand only CDs, not DVD
      if (false) {
        std::cout << "wrong disk, sorry" << std::endl;
        return false;
      }
      return true;
    }
  };
  struct always_true {
    template <class EVT, class FSM, class SourceState, class TargetState>
    bool operator()(EVT const& evt, FSM&, SourceState&, TargetState&) {
      return true;
    }
  };
  struct open_file_guard {
    template <class EVT, class FSM, class SourceState, class TargetState>
    bool operator()(EVT const& evt, FSM&, SourceState&, TargetState&) {
      auto k_h = make_handle(evt.p_ent);
      if (k_h) {
        return k_h.get<opencv_read_player>().is_open();
      }
      return false;
    }
  };

  /// 开始定义转换表
  struct transition_table
      : public boost::mpl::vector<
            // clang-format off
            //              Start      Event         Next      Action                         Guard
            //+------------+----------+-------------+---------+------------------------------+--------------
            msm::front::Row<empty,     open_file,    playing,  msm::front::none,              open_file_guard>,

            //+------------+----------+-------------+---------+------------------------------+--------------
            msm::front::Row<stopped,   play,         playing,  msm::front::ActionSequence_<
                                                                  boost::mpl::vector<
                                                                     start_playback>>,        dummy_guard     >,
            msm::front::Row<stopped,   stop,         stopped,  msm::front::none,              msm::front::none>,
            //+------------+----------+-------------+---------+------------------------------+--------------
            msm::front::Row<playing,   play,         playing,  msm::front::none,              msm::front::none>,
            msm::front::Row<playing,   stop,         stopped,  msm::front::none,              msm::front::none>,
            msm::front::Row<playing,   pause,        paused,   msm::front::none,              msm::front::none>,

            //+------------+----------+-------------+---------+------------------------------+--------------
            msm::front::Row<paused,    stop,         stopped,  msm::front::none,              msm::front::none>,
            msm::front::Row<paused,    play,         playing,  msm::front::none,              msm::front::none>
            // clang-format on
            > {};

  // 替换默认的无转换响应
  template <class FSM, class Event>
  void no_transition(Event const& e, FSM&, int state) {
    std::cout << "no transition from state " << state
              << " on event " << typeid(e).name() << std::endl;
  }
};
using player = msm::back::state_machine<player_>;

}  // namespace opencv_player_ns
using player = opencv_player_ns::player;

opencv_player_widget::opencv_player_widget() {
}

std::float_t compute_size(std::vector<opencv::frame>& in_size, const ImVec2& in_v2) {
  // 动画
  // 解算
  // 特效
  // 灯光
  std::float_t k_proportional{1};
  if (in_size.empty())
    return k_proportional;

  auto k_f = in_size.front();
  if (k_f.width > in_v2.x) {
    k_proportional = k_proportional * (in_v2.x / k_f.width);
    k_f.multiply(k_proportional);
  }

  if (k_f.height > in_v2.y) {
    k_proportional = k_proportional * (in_v2.y / k_f.height);
  }
  k_proportional = k_proportional / 2;
  return k_proportional;
}

void opencv_player_widget::init() {
  g_reg()->ctx().emplace<opencv_player_widget>(*this);
}
void opencv_player_widget::succeeded() {
}
void opencv_player_widget::failed() {
}
void opencv_player_widget::aborted() {
}
void opencv_player_widget::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  auto k_view = g_reg()->view<opencv_read_player>();
  std::vector<opencv::frame> k_list{};
  for (auto& k_i : k_view) {
    auto& k_open = k_view.get<opencv_read_player>(k_i);
    if (!k_open.is_open())
      continue;
    auto k_v = k_open.read(1);
    k_list.push_back(std::move(k_v));
    // imgui::Image(k_v, ImVec2{
    //                       boost::numeric_cast<std::float_t>(k_s.first),
    //                       boost::numeric_cast<std::float_t>(k_s.second)});
  }
  auto k_s    = compute_size(k_list, imgui::GetContentRegionAvail());
  bool k_line = true;
  for (auto& f : k_list) {
    f.multiply(k_s);
    imgui::Image(f.data, ImVec2{boost::numeric_cast<std::float_t>(f.width), boost::numeric_cast<std::float_t>(f.height)});
    if (k_line) {
      imgui::SameLine();
    }
    k_line = !k_line;
  }
}

}  // namespace doodle
