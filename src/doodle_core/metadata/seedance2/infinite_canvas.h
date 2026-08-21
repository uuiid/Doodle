#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <string>

namespace doodle::seedance2 {

/// 无限画布 — 画布元数据，与画布内容（canvas_element）分离存储
struct DOODLE_CORE_API infinite_canvas {
  DOODLE_BASE_FIELDS();

  std::string name_;                           // 画布名称
  std::string description_;                    // 画布描述
  uuid project_uuid_id_;                       // 所属项目
  uuid user_id_;                               // 创建者

  // 视口状态
  double viewport_x_{0.0};                     // 视口水平偏移
  double viewport_y_{0.0};                     // 视口垂直偏移
  double viewport_zoom_{1.0};                  // 视口缩放

  // 画布外观
  std::string background_color_{"#ffffff"};    // 背景色

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const infinite_canvas& p) {
    j["id"]               = p.uuid_id_;
    j["name"]             = p.name_;
    j["description"]      = p.description_;
    j["project_uuid_id"]  = p.project_uuid_id_;
    j["user_id"]          = p.user_id_;
    j["viewport_x"]       = p.viewport_x_;
    j["viewport_y"]       = p.viewport_y_;
    j["viewport_zoom"]    = p.viewport_zoom_;
    j["background_color"] = p.background_color_;
    j["created_at"]       = p.created_at_;
    j["updated_at"]       = p.updated_at_;
  }

  friend void from_json(const nlohmann::json& j, infinite_canvas& p) {
    j.at("name").get_to(p.name_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("project_uuid_id")) j.at("project_uuid_id").get_to(p.project_uuid_id_);
    if (j.contains("user_id")) j.at("user_id").get_to(p.user_id_);
    if (j.contains("viewport_x")) j.at("viewport_x").get_to(p.viewport_x_);
    if (j.contains("viewport_y")) j.at("viewport_y").get_to(p.viewport_y_);
    if (j.contains("viewport_zoom")) j.at("viewport_zoom").get_to(p.viewport_zoom_);
    if (j.contains("background_color")) j.at("background_color").get_to(p.background_color_);
  }
};

}  // namespace doodle::seedance2