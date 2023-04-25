//
// Created by TD on 2022/5/10.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/MMessage.h>
namespace doodle {
namespace maya_plug {

class maya_call_guard {
 public:
  MCallbackId call_id{};
  explicit maya_call_guard(MCallbackId&& in_id) : call_id(std::move(in_id)) {}
  ~maya_call_guard() {
    auto k_s = MMessage::removeCallback(call_id);
    if (!k_s) {
      DOODLE_LOG_ERROR("卸载回调出错");
    }
  }
};

}  // namespace maya_plug
}  // namespace doodle
