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

BOOST_MSM_EUML_TRANSITION_TABLE(
    (
        need_load + load_meta == is_load,

        need_save + save_meta == is_load,
        need_save + load_meta == is_load,
        need_delete + delete_meta == is_load

        ),
    metadata_transition_table)
BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (metadata_transition_table, init_ << need_load),
    metadata_state_machine)
}  // namespace state_ns

}  // namespace doodle
