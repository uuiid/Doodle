#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <string>

namespace doodle::seedance2 {

/// 媒体角色 — 区分同一 canvas_element 的不同媒体版本
enum class DOODLE_CORE_API media_role {
  original,   // 原始上传文件
  thumbnail,  // 缩略图
  preview,    // 预览/压缩版本
  variant,    // 其他变体
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    media_role, {
                    {media_role::original, "original"},
                    {media_role::thumbnail, "thumbnail"},
                    {media_role::preview, "preview"},
                    {media_role::variant, "variant"},
                }
);

/// 画布元素媒体文件 — 与 canvas_element 分离存储，单实例含多版本文件
/// 各版本通过 media_role 区分 URL 请求，路径由 kitsu_ctx_t 派生
struct DOODLE_CORE_API canvas_media {
  DOODLE_BASE_FIELDS();

  uuid canvas_element_id_;  // 所属画布元素 (canvas_element.uuid_id_)

  // 原始文件
  std::string file_extension_;  // 扩展名（含点，如 ".png"）

  // 缩略图 — 固定 .png，不存扩展名和大小
  // 预览/压缩版本
  std::string preview_extension_;  // 预览扩展名

  // 媒体属性（以原始文件为准）
  std::int32_t width_{0};   // 宽度 px
  std::int32_t height_{0};  // 高度 px
  double duration_{0.0};    // 时长 秒 (视频/音频)

  std::string original_name_;  // 上传时的原始文件名

  // 扩展 — codec, bitrate, framerate, mime_type 等
  nlohmann::json metadata_;

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const canvas_media& p) {
    j["id"]                = p.uuid_id_;
    j["canvas_element_id"] = p.canvas_element_id_;
    j["file_extension"]    = p.file_extension_;
    if (!p.preview_extension_.empty()) {
      j["preview_extension"] = p.preview_extension_;
    }
    j["width"]    = p.width_;
    j["height"]   = p.height_;
    j["duration"] = p.duration_;
    if (!p.original_name_.empty()) j["original_name"] = p.original_name_;
    if (!p.metadata_.empty()) j["metadata"] = p.metadata_;
    j["created_at"] = p.created_at_;
    j["updated_at"] = p.updated_at_;
  }

  friend void from_json(const nlohmann::json& j, canvas_media& p) {
    j.at("canvas_element_id").get_to(p.canvas_element_id_);
    if (j.contains("file_extension")) j.at("file_extension").get_to(p.file_extension_);
    if (j.contains("preview_extension")) j.at("preview_extension").get_to(p.preview_extension_);
    if (j.contains("width")) j.at("width").get_to(p.width_);
    if (j.contains("height")) j.at("height").get_to(p.height_);
    if (j.contains("duration")) j.at("duration").get_to(p.duration_);
    if (j.contains("original_name")) j.at("original_name").get_to(p.original_name_);
    if (j.contains("metadata")) j.at("metadata").get_to(p.metadata_);
  }
};

}  // namespace doodle::seedance2