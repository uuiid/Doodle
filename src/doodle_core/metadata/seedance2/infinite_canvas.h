#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <array>
#include <chrono>
#include <string>
#include <tuple>
#include <utility>

namespace doodle::seedance2 {

/// 画布背景模式 — 对齐 infinite-canvas 前端库的 backgroundMode
enum class DOODLE_CORE_API canvas_background_mode {
  blank,  // 空白
  lines,  // 网格线
  dots,   // 网点
};

DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    canvas_background_mode,
    {
        {canvas_background_mode::blank, "blank"},
        {canvas_background_mode::lines, "lines"},
        {canvas_background_mode::dots, "dots"},
    }
);

/// 无限画布 — 画布元数据，与画布内容（canvas_element）分离存储
/// 对齐 infinite-canvas 前端库的 CanvasProject
struct DOODLE_CORE_API infinite_canvas {
  DOODLE_BASE_FIELDS();

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"title", &infinite_canvas::title_},
        std::pair{"description", &infinite_canvas::description_},
        std::pair{std::array{"viewport", "x"}, &infinite_canvas::viewport_x_},
        std::pair{std::array{"viewport", "y"}, &infinite_canvas::viewport_y_},
        std::pair{std::array{"viewport", "k"}, &infinite_canvas::viewport_k_},
        std::pair{"background_mode", &infinite_canvas::background_mode_},
    };
  }

  std::string title_;                            // 画布名称（对齐前端 title）
  std::string description_;                      // 画布描述
  uuid project_uuid_id_;                         // 所属项目
  uuid subproject_id_;                           // 所属子项目
  uuid ai_episode_id_;                           // 所属剧集 (ai_episode.uuid_id_)
  uuid ai_generate_entity_id_;                   // 所属生成实体 (ai_generate_entity.uuid_id_)
  uuid user_id_;                                 // 创建者

  // 视口状态 — 序列化为 viewport: {x, y, k}
  double viewport_x_{0.0};                       // 视口水平偏移
  double viewport_y_{0.0};                       // 视口垂直偏移
  double viewport_k_{1.0};                       // 视口缩放（对齐前端 k）

  // 画布外观
  canvas_background_mode background_mode_{canvas_background_mode::dots};  // 背景模式

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const infinite_canvas& p) {
    j["id"]                    = p.uuid_id_;
    j["title"]                 = p.title_;
    j["description"]           = p.description_;
    j["project_uuid_id"]       = p.project_uuid_id_;
    j["subproject_id"]         = p.subproject_id_;
    j["ai_episode_id"]         = p.ai_episode_id_;
    j["ai_generate_entity_id"] = p.ai_generate_entity_id_;
    j["user_id"]               = p.user_id_;
    j["viewport"]              = {{"x", p.viewport_x_}, {"y", p.viewport_y_}, {"k", p.viewport_k_}};
    j["background_mode"]       = p.background_mode_;
    j["created_at"]            = p.created_at_;
    j["updated_at"]            = p.updated_at_;
  }

  friend void from_json(const nlohmann::json& j, infinite_canvas& p) {
    j.at("title").get_to(p.title_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("project_uuid_id")) j.at("project_uuid_id").get_to(p.project_uuid_id_);
    if (j.contains("subproject_id")) j.at("subproject_id").get_to(p.subproject_id_);
    if (j.contains("ai_episode_id")) j.at("ai_episode_id").get_to(p.ai_episode_id_);
    if (j.contains("ai_generate_entity_id")) j.at("ai_generate_entity_id").get_to(p.ai_generate_entity_id_);
    if (j.contains("user_id")) j.at("user_id").get_to(p.user_id_);
    if (j.contains("viewport")) {
      if (j["viewport"].contains("x")) j["viewport"]["x"].get_to(p.viewport_x_);
      if (j["viewport"].contains("y")) j["viewport"]["y"].get_to(p.viewport_y_);
      if (j["viewport"].contains("k")) j["viewport"]["k"].get_to(p.viewport_k_);
    }
    if (j.contains("background_mode")) j.at("background_mode").get_to(p.background_mode_);
  }
};

}  // namespace doodle::seedance2