//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include "boost/operators.hpp"

#include "entt/entity/fwd.hpp"
#include <entt/entt.hpp>
namespace doodle {

namespace details {
enum class assets_type_enum {
  scene,
  prop,
  character,
  rig,
  animation,
  vfx,
  cfx,
  other,  // 基本是相机
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    assets_type_enum, {
                          {assets_type_enum::scene, "scene"},
                          {assets_type_enum::prop, "prop"},
                          {assets_type_enum::character, "character"},
                          {assets_type_enum::rig, "rig"},
                          {assets_type_enum::animation, "animation"},
                          {assets_type_enum::vfx, "vfx"},
                          {assets_type_enum::cfx, "cfx"},
                          {assets_type_enum::other, "other"},
                      }
);
}  // namespace details

details::assets_type_enum convert_assets_type_enum(const uuid& in_assets_type_id);

namespace assets_helper {
struct database_t {
  std::int32_t id_{};
  uuid uuid_id_{};
  std::string label_{};
  /// 这个数据不在数据库中
  uuid uuid_parent_{};
  std::int32_t order_{};
  friend void to_json(nlohmann::json& j, const database_t& v) {
    j["id"]        = v.uuid_id_;
    j["label"]     = v.label_;
    j["parent_id"] = v.uuid_parent_;
    j["order"]     = v.order_;
  }

  friend void from_json(const nlohmann::json& j, database_t& v) {
    j.at("label").get_to(v.label_);
    if (j.contains("parent_id") && j["parent_id"].is_string()) j.at("parent_id").get_to(v.uuid_parent_);
    if (j.contains("order")) j.at("order").get_to(v.order_);
    if (j.contains("id") && j["id"].is_string()) j.at("id").get_to(v.uuid_id_);
  }
};
}  // namespace assets_helper
}  // namespace doodle
