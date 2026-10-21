#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <array>
#include <chrono>
#include <string>
#include <tuple>
#include <utility>

namespace doodle::seedance2 {

/// 画布元素类型 — 对齐 infinite-canvas 前端库的 CanvasNodeType
enum class DOODLE_CORE_API canvas_element_type {
  text,      // 文本
  image,     // 图片
  panorama,  // 全景图（等距柱状投影）
  video,     // 视频
  audio,     // 音频
  config,    // 生成配置节点（模型/参数/提示词）
  director,  // 导演台
  group,     // 组合/分组
};

DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    canvas_element_type,
    {
        {canvas_element_type::text, "text"},
        {canvas_element_type::image, "image"},
        {canvas_element_type::panorama, "panorama"},
        {canvas_element_type::video, "video"},
        {canvas_element_type::audio, "audio"},
        {canvas_element_type::config, "config"},
        {canvas_element_type::director, "director"},
        {canvas_element_type::group, "group"},
    }
);

/// 无限画布上的单个节点，独立记录存储
/// 对齐 infinite-canvas 前端库的 CanvasNodeData
struct DOODLE_CORE_API canvas_element {
  DOODLE_BASE_FIELDS();

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"type", &canvas_element::element_type_},
        std::pair{"title", &canvas_element::title_},
        std::pair{std::array{"position", "x"}, &canvas_element::position_x_},
        std::pair{std::array{"position", "y"}, &canvas_element::position_y_},
        std::pair{"width", &canvas_element::width_},
        std::pair{"height", &canvas_element::height_},
        std::pair{"metadata", &canvas_element::metadata_},
        std::pair{"group_id", &canvas_element::parent_id_},
    };
  }

  uuid canvas_id_;                               // 所属画布 (infinite_canvas.uuid_id_)
  canvas_element_type element_type_{canvas_element_type::text};

  std::string title_;                            // 节点标题（对齐前端 title）

  // 几何 — 序列化为 position: {x, y}
  double position_x_{0.0};
  double position_y_{0.0};
  double width_{340.0};
  double height_{240.0};

  // 扩展元数据 — 对齐前端 metadata 自由字段袋，原样存取（含 content）
  nlohmann::json metadata_;                      // content, prompt, status, model, size, generationMode, 等

  // 所属 group 节点 ID — 对应前端顶层 groupId，后端不从 metadata 提取
  uuid parent_id_;

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const canvas_element& p) {
    j["id"]         = p.uuid_id_;
    j["canvas_id"]  = p.canvas_id_;
    j["type"]       = p.element_type_;
    j["title"]      = p.title_;
    j["position"]   = {{"x", p.position_x_}, {"y", p.position_y_}};
    j["width"]      = p.width_;
    j["height"]     = p.height_;
    if (!p.metadata_.empty()) j["metadata"] = p.metadata_;
    if (!p.parent_id_.is_nil()) j["group_id"] = p.parent_id_;
    j["created_at"] = p.created_at_;
    j["updated_at"] = p.updated_at_;
  }

  friend void from_json(const nlohmann::json& j, canvas_element& p) {
    j.at("canvas_id").get_to(p.canvas_id_);
    if (j.contains("type")) j.at("type").get_to(p.element_type_);
    if (j.contains("title")) j.at("title").get_to(p.title_);
    if (j.contains("position")) {
      if (j["position"].contains("x")) j["position"]["x"].get_to(p.position_x_);
      if (j["position"].contains("y")) j["position"]["y"].get_to(p.position_y_);
    }
    if (j.contains("width")) j.at("width").get_to(p.width_);
    if (j.contains("height")) j.at("height").get_to(p.height_);
    if (j.contains("metadata")) p.metadata_ = j["metadata"];
    if (j.contains("group_id")) j.at("group_id").get_to(p.parent_id_);
  }
};

}  // namespace doodle::seedance2
