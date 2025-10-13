//
// Created by assistant on 25-10-10.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle {

struct DOODLE_CORE_API ai_image_metadata {
  DOODLE_BASE_FIELDS();

  // 生成图像所用的文本提示
  std::string prompt_;
  // 生成 生成id
  std::string task_id_;
  // 分类
  std::string category_;
  // 扩展名
  std::string extension_;

  // 图像尺寸
  std::int32_t width_;
  std::int32_t height_;

  // 生成时间（ISO 8601 字符串）
  chrono::system_zoned_time created_at_;

  // 作者或使用者信息
  uuid author_;

  // JSON 序列化
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const ai_image_metadata& p) {
    j["id"]         = p.uuid_id_;
    j["prompt"]     = p.prompt_;
    j["task_id"]    = p.task_id_;
    j["width"]      = p.width_;
    j["height"]     = p.height_;
    j["created_at"] = p.created_at_;
    j["author"]     = p.author_;
    j["category"]   = p.category_;
    j["extension"]  = p.extension_;
  }

  friend void from_json(const nlohmann::json& j, ai_image_metadata& p) {
    if (j.contains("id")) j["id"].get_to(p.uuid_id_);
    if (j.contains("prompt")) j["prompt"].get_to(p.prompt_);
    if (j.contains("task_id")) j["task_id"].get_to(p.task_id_);
    if (j.contains("width")) j["width"].get_to(p.width_);
    if (j.contains("height")) j["height"].get_to(p.height_);
    if (j.contains("created_at")) j["created_at"].get_to(p.created_at_);
    if (j.contains("author")) j["author"].get_to(p.author_);
    if (j.contains("category")) j["category"].get_to(p.category_);
    if (j.contains("extension")) j["extension"].get_to(p.extension_);
  }
};

}  // namespace doodle
