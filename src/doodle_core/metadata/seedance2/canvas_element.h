#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <string>

namespace doodle::seedance2 {

/// 画布元素类型
enum class DOODLE_CORE_API canvas_element_type {
  text,    // 文本
  image,   // 图片
  video,   // 视频
  audio,   // 音频
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    canvas_element_type,
    {
        {canvas_element_type::text, "text"},
        {canvas_element_type::image, "image"},
        {canvas_element_type::video, "video"},
        {canvas_element_type::audio, "audio"},
    }
);

/// 无限画布上的单个元素，独立记录存储
struct DOODLE_CORE_API canvas_element {
  DOODLE_BASE_FIELDS();

  uuid canvas_id_;                               // 所属画布 (infinite_canvas.uuid_id_)
  canvas_element_type element_type_{canvas_element_type::text};

  std::string name_;                             // 可选名称

  // 几何变换
  double position_x_{0.0};
  double position_y_{0.0};
  double width_{100.0};
  double height_{100.0};
  double rotation_{0.0};                         // 旋转角度（度）

  // 层级
  std::int32_t z_index_{0};

  // 样式
  std::string fill_color_{"#ffffff"};            // 填充色
  std::string stroke_color_{"#000000"};          // 描边色
  double stroke_width_{1.0};                     // 描边宽度
  double opacity_{1.0};                          // 不透明度 [0, 1]

  // 类型特定内容
  std::string text_content_;                     // text 类型的文本内容
  std::string image_url_;                        // image 类型的图片 URL
  std::string video_url_;                        // video 类型的视频 URL
  std::string audio_url_;                        // audio 类型的音频 URL

  // 组合与层级
  uuid parent_id_;                               // 父元素 ID，用于嵌套组合

  // 状态
  bool locked_{false};
  bool visible_{true};

  // 扩展数据
  nlohmann::json custom_data_;                   // 任意自定义属性

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const canvas_element& p) {
    j["id"]            = p.uuid_id_;
    j["canvas_id"]     = p.canvas_id_;
    j["element_type"]  = p.element_type_;
    j["name"]          = p.name_;
    j["position_x"]    = p.position_x_;
    j["position_y"]    = p.position_y_;
    j["width"]         = p.width_;
    j["height"]        = p.height_;
    j["rotation"]      = p.rotation_;
    j["z_index"]       = p.z_index_;
    j["fill_color"]    = p.fill_color_;
    j["stroke_color"]  = p.stroke_color_;
    j["stroke_width"]  = p.stroke_width_;
    j["opacity"]       = p.opacity_;
    j["text_content"]  = p.text_content_;
    j["image_url"]     = p.image_url_;
    j["video_url"]     = p.video_url_;
    j["audio_url"]     = p.audio_url_;
    j["parent_id"]     = p.parent_id_;
    j["locked"]        = p.locked_;
    j["visible"]       = p.visible_;
    j["custom_data"]   = p.custom_data_;
    j["created_at"]    = p.created_at_;
    j["updated_at"]    = p.updated_at_;
  }

  friend void from_json(const nlohmann::json& j, canvas_element& p) {
    j.at("canvas_id").get_to(p.canvas_id_);
    if (j.contains("element_type")) j.at("element_type").get_to(p.element_type_);
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("position_x")) j.at("position_x").get_to(p.position_x_);
    if (j.contains("position_y")) j.at("position_y").get_to(p.position_y_);
    if (j.contains("width")) j.at("width").get_to(p.width_);
    if (j.contains("height")) j.at("height").get_to(p.height_);
    if (j.contains("rotation")) j.at("rotation").get_to(p.rotation_);
    if (j.contains("z_index")) j.at("z_index").get_to(p.z_index_);
    if (j.contains("fill_color")) j.at("fill_color").get_to(p.fill_color_);
    if (j.contains("stroke_color")) j.at("stroke_color").get_to(p.stroke_color_);
    if (j.contains("stroke_width")) j.at("stroke_width").get_to(p.stroke_width_);
    if (j.contains("opacity")) j.at("opacity").get_to(p.opacity_);
    if (j.contains("text_content")) j.at("text_content").get_to(p.text_content_);
    if (j.contains("image_url")) j.at("image_url").get_to(p.image_url_);
    if (j.contains("video_url")) j.at("video_url").get_to(p.video_url_);
    if (j.contains("audio_url")) j.at("audio_url").get_to(p.audio_url_);
    if (j.contains("parent_id")) j.at("parent_id").get_to(p.parent_id_);
    if (j.contains("locked")) j.at("locked").get_to(p.locked_);
    if (j.contains("visible")) j.at("visible").get_to(p.visible_);
    if (j.contains("custom_data")) j.at("custom_data").get_to(p.custom_data_);
  }
};

}  // namespace doodle::seedance2