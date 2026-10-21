#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <string>

namespace doodle::seedance2 {

/// 画布节点之间的连线 — 对齐 infinite-canvas 前端库的 CanvasConnection
/// 前端仅使用 id / fromNodeId / toNodeId
struct DOODLE_CORE_API canvas_link {
  DOODLE_BASE_FIELDS();

  uuid canvas_id_;                               // 所属画布 (infinite_canvas.uuid_id_)
  uuid source_id_;                               // 起始节点 (canvas_element.uuid_id_)
  uuid target_id_;                               // 目标节点 (canvas_element.uuid_id_)

  std::string label_;                            // 连线标签

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const canvas_link& p) {
    j["id"]         = p.uuid_id_;
    j["canvas_id"]  = p.canvas_id_;
    j["fromNodeId"] = p.source_id_;
    j["toNodeId"]   = p.target_id_;
    if (!p.label_.empty()) j["label"] = p.label_;
    j["created_at"] = p.created_at_;
    j["updated_at"] = p.updated_at_;
  }

  friend void from_json(const nlohmann::json& j, canvas_link& p) {
    j.at("canvas_id").get_to(p.canvas_id_);
    j.at("fromNodeId").get_to(p.source_id_);
    j.at("toNodeId").get_to(p.target_id_);
    if (j.contains("label")) j.at("label").get_to(p.label_);
  }
};

}  // namespace doodle::seedance2
