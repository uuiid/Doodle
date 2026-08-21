#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <string>

namespace doodle::seedance2 {

/// 画布节点之间的连线 — 对齐 infinite-canvas 前端库的 CanvasConnection
/// 前端仅使用 id / fromNodeId / toNodeId，样式字段为后端扩展
struct DOODLE_CORE_API canvas_link {
  DOODLE_BASE_FIELDS();

  uuid canvas_id_;                               // 所属画布 (infinite_canvas.uuid_id_)
  uuid source_id_;                               // 起始节点 (canvas_element.uuid_id_)
  uuid target_id_;                               // 目标节点 (canvas_element.uuid_id_)

  // 以下为后端扩展，前端不直接使用
  std::string label_;                            // 连线标签
  std::string color_{"#000000"};                 // 连线颜色
  double width_{2.0};                            // 连线宽度
  std::string dash_pattern_;                     // 虚线样式
  bool directed_{false};                         // 是否有箭头

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const canvas_link& p) {
    j["id"]         = p.uuid_id_;
    j["canvas_id"]  = p.canvas_id_;
    j["fromNodeId"] = p.source_id_;
    j["toNodeId"]   = p.target_id_;
    if (!p.label_.empty()) j["label"] = p.label_;
    j["color"]        = p.color_;
    j["width"]        = p.width_;
    if (!p.dash_pattern_.empty()) j["dashPattern"] = p.dash_pattern_;
    j["directed"]     = p.directed_;
    j["created_at"]   = p.created_at_;
    j["updated_at"]   = p.updated_at_;
  }

  friend void from_json(const nlohmann::json& j, canvas_link& p) {
    j.at("canvas_id").get_to(p.canvas_id_);
    j.at("fromNodeId").get_to(p.source_id_);
    j.at("toNodeId").get_to(p.target_id_);
    if (j.contains("label")) j.at("label").get_to(p.label_);
    if (j.contains("color")) j.at("color").get_to(p.color_);
    if (j.contains("width")) j.at("width").get_to(p.width_);
    if (j.contains("dashPattern")) j.at("dashPattern").get_to(p.dash_pattern_);
    if (j.contains("directed")) j.at("directed").get_to(p.directed_);
  }
};

}  // namespace doodle::seedance2