//
// Created by TD on 2022/7/21.
//

#pragma once
#include <main/maya_plug_fwd.h>

#include <maya/MDrawRegistry.h>
#include <maya/MMessage.h>
#include <stack>

/// 提前声明 maya MFnPlugin 类
OPENMAYA_MAJOR_NAMESPACE_OPEN
class MFnPlugin;
OPENMAYA_NAMESPACE_CLOSE

namespace doodle::maya_plug {
class maya_register {
  std::stack<std::function<MStatus(MFnPlugin&)>> maya_comm_call_back{};

 public:
  maya_register()  = default;
  ~maya_register() = default;

  template <typename comm_type>
  MStatus register_command(MFnPlugin& in_) {
    auto l_s = comm_type::registerCommand(in_);
    CHECK_MSTATUS(l_s);
    if (l_s) {
      maya_comm_call_back.emplace([](auto& in_plugin) {
        auto l_s = comm_type::deregisterCommand(in_plugin);
        CHECK_MSTATUS(l_s);
        return l_s;
      });
    }

    return l_s;
  }

  inline void register_callback(const MCallbackId& in_) {
    maya_comm_call_back.emplace([in_](auto& in) {
      auto l_s = MMessage::removeCallback(in_);
      CHECK_MSTATUS(l_s);
      return l_s;
    });
  }

  template <typename node_type, typename mfn_plugin_type>
  MStatus register_node(mfn_plugin_type& in_) {
    auto l_s = in_.registerNode(
        node_type::node_name.data(),
        node_type::doodle_id,
        &node_type::creator,
        &node_type::initialize,
        node_type::node_type,
        &node_type::drawDbClassification
    );
    CHECK_MSTATUS(l_s);
    if (l_s) {
      maya_comm_call_back.emplace([](auto& in_plugin) {
        auto l_s = in_plugin.deregisterNode(
            node_type::doodle_id
        );
        CHECK_MSTATUS(l_s);
        return l_s;
      });
    }
    return l_s;
  }
  template <typename node_type, typename draw_type>
  MStatus register_draw_overrider() {
    auto l_s = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
        node_type::drawDbClassification,
        node_type::drawRegistrantId,
        draw_type::Creator
    );
    CHECK_MSTATUS(l_s);
    if (l_s) {
      maya_comm_call_back.emplace([](auto& in) {
        auto l_s = MDrawRegistry::deregisterGeometryOverrideCreator(
            node_type::drawDbClassification,
            node_type::drawRegistrantId
        );
        CHECK_MSTATUS(l_s);
        return l_s;
      });
    }
    return l_s;
  }

  template <typename Fun_t>
  MStatus register_lab(
      Fun_t&& in_fun
  ) {
    maya_comm_call_back.emplace(std::forward<Fun_t>(in_fun));
  }

  template <typename mfn_plugin_type>
  MStatus unregister(mfn_plugin_type& in) {
    MStatus l_s{};
    while (!maya_comm_call_back.empty()) {
      l_s = maya_comm_call_back.top()(in);
      CHECK_MSTATUS(l_s);
      maya_comm_call_back.pop();
    }
    return l_s;
  }
};
}  // namespace doodle::maya_plug
