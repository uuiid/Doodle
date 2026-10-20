#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <cstdint>
#include <string>
#include <vector>

namespace doodle::seedance2 {

struct DOODLE_CORE_API transfer_station_request_t {
  std::string model_;
  std::string prompt_;
  std::vector<std::string> reference_images_;
  std::string aspect_ratio_;
  std::string image_size_;
  std::string quality_;
  std::string background_;
  std::string mask_;
  std::string reply_type_;

  [[nodiscard]] bool has_reference_images() const { return !reference_images_.empty(); }

  friend void to_json(nlohmann::json& j, const transfer_station_request_t& p) {
    j["model"]  = p.model_;
    j["prompt"] = p.prompt_;
    if (!p.reference_images_.empty()) j["images"] = p.reference_images_;
    if (!p.aspect_ratio_.empty()) j["aspectRatio"] = p.aspect_ratio_;
    if (!p.image_size_.empty()) j["imageSize"] = p.image_size_;
    if (!p.quality_.empty()) j["quality"] = p.quality_;
    if (!p.background_.empty()) j["background"] = p.background_;
    if (!p.mask_.empty()) j["mask"] = p.mask_;
    if (!p.reply_type_.empty()) j["replyType"] = p.reply_type_;
  }

  friend void from_json(const nlohmann::json& j, transfer_station_request_t& p) {
    if (!j.is_object()) throw nlohmann::json::type_error::create(302, "中转站请求体必须是 JSON 对象", &j);

    if (!j.contains("model") || !j.at("model").is_string())
      throw nlohmann::json::type_error::create(302, "中转站请求体缺少必填的字符串键 model", &j);
    if (!j.contains("prompt") || !j.at("prompt").is_string())
      throw nlohmann::json::type_error::create(302, "中转站请求体缺少必填的字符串键 prompt", &j);
    j.at("model").get_to(p.model_);
    j.at("prompt").get_to(p.prompt_);

    const auto l_has_size   = j.contains("imageSize") && j.at("imageSize").is_string();
    const auto l_has_aspect = j.contains("aspectRatio") && j.at("aspectRatio").is_string();
    if (!l_has_size && !l_has_aspect)
      throw nlohmann::json::type_error::create(
          302, "中转站请求体必须给出 imageSize 与 aspectRatio 之一(均为字符串)", &j
      );

    if (l_has_aspect) j.at("aspectRatio").get_to(p.aspect_ratio_);
    if (l_has_size) j.at("imageSize").get_to(p.image_size_);

    if (j.contains("mask") && j.at("mask").is_string()) j.at("mask").get_to(p.mask_);
    if (j.contains("images") && j.at("images").is_array()) j.at("images").get_to(p.reference_images_);
    if (j.contains("quality") && j.at("quality").is_string()) j.at("quality").get_to(p.quality_);
    if (j.contains("background") && j.at("background").is_string()) j.at("background").get_to(p.background_);
    if (j.contains("replyType") && j.at("replyType").is_string()) j.at("replyType").get_to(p.reply_type_);
  }
};

}  // namespace doodle::seedance2
