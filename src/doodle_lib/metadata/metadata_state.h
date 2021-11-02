//
// Created by TD on 2021/11/01.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>
namespace doodle {
namespace state_ns {
namespace msm = boost::msm;
using namespace boost::msm::front::euml;
BOOST_MSM_EUML_STATE((), need_load)
BOOST_MSM_EUML_STATE((), is_load)
BOOST_MSM_EUML_STATE((), need_save)
BOOST_MSM_EUML_STATE((), need_delete)

BOOST_MSM_EUML_EVENT(save_meta)
BOOST_MSM_EUML_EVENT(load_meta)
BOOST_MSM_EUML_EVENT(delete_meta)
BOOST_MSM_EUML_EVENT(updata_meta)
BOOST_MSM_EUML_EVENT(updata_server)
BOOST_MSM_EUML_EVENT(create_meta)

struct load_guard {
  template <class EVT, class FSM, class SourceState, class TargetState>
  bool operator()(EVT const& evt, FSM&, SourceState& in_source, TargetState& in_target) {
    if (in_source == need_load)
      return true;
    return false;
  }
};
struct save_guard {
  template <class EVT, class FSM, class SourceState, class TargetState>
  bool operator()(EVT const& evt, FSM&, SourceState& in_source, TargetState& in_target) {
    if (in_source == need_save)
      return true;
    return false;
  }
};
struct delete_guard {
  template <class EVT, class FSM, class SourceState, class TargetState>
  bool operator()(EVT const& evt, FSM&, SourceState& in_source, TargetState& in_target) {
    return true;
  }
};

BOOST_MSM_EUML_TRANSITION_TABLE(
    (
        need_load + load_meta == is_load,
        need_load + create_meta == is_load,

        need_save + save_meta == is_load,
        need_save + load_meta == is_load,
        is_load + updata_meta == need_save,
        is_load + updata_server == need_load,

        need_delete + delete_meta == is_load),
    metadata_transition_table)
BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (metadata_transition_table, init_ << need_load),
    metadata_state_machine)

}  // namespace state_ns

}  // namespace doodle
