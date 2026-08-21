#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <string>

namespace doodle::seedance2 {

/// 链接线型
enum class DOODLE_CORE_API canvas_link_type {
  straight,    // 直线
  curved,      // 曲线
  orthogonal,  // 正交折线
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    canvas_link_type,
    {
        {canvas_link_type::straight, "straight"},
        {canvas_link_type::curved, "curved"},
        {canvas_link_type::orthogonal, "orthogonal"},
    }
);

/// 画布节点之间的链接，独立记录存储
struct DOODLE_CORE_API canvas_link {
  DOODLE_BASE_FIELDS();

  uuid canvas_id_;                               // 所属画布 (infinite_canvas.uuid_id_)
  uuid source_id_;                               // 起始元素 (canvas_element.uuid_id_)
  uuid target_id_;                               // 目标元素 (canvas_element.uuid_id_)

  canvas_link_type link_type_{canvas_link_type::straight};

  std::string label_;                            // 链接上的标签文字

  // 样式
  std::string color_{"#000000"};                 // 连线颜色
  double width_{2.0};                            // 连线宽度
  std::string dash_pattern_;                     // 虚线样式，如 "5,5"；空串为实线

  // 方向
  bool directed_{false};                         // 是否有箭头

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const canvas_link& p) {
    j["id"]           = p.uuid_id_;
    j["canvas_id"]    = p.canvas_id_;
    j["source_id"]    = p.source_id_;
    j["target_id"]    = p.target_id_;
    j["link_type"]    = p.link_type_;
    j["label"]        = p.label_;
    j["color"]        = p.color_;
    j["width"]        = p.width_;
    j["dash_pattern"] = p.dash_pattern_;
    j["directed"]     = p.directed_;
    j["created_at"]   = p.created_at_;
    j["updated_at"]   = p.updated_at_;
  }

  friend void from_json(const nlohmann::json& j, canvas_link& p) {
    j.at("canvas_id").get_to(p.canvas_id_);
    j.at("source_id").get_to(p.source_id_);
    j.at("target_id").get_to(p.target_id_);
    if (j.contains("link_type")) j.at("link_type").get_to(p.link_type_);
    if (j.contains("label")) j.at("label").get_to(p.label_);
    if (j.contains("color")) j.at("color").get_to(p.color_);
    if (j.contains("width")) j.at("width").get_to(p.width_);
    if (j.contains("dash_pattern")) j.at("dash_pattern").get_to(p.dash_pattern_);
    if (j.contains("directed")) j.at("directed").get_to(p.directed_);
  }
};

}  // namespace doodle::seedance2