#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>
#include <tuple>
#include <utility>

namespace doodle::seedance2 {
enum class DOODLE_CORE_API ai_category_type {
  shot,    ///< 镜头类别
  assets,  ///< 素材类别
};
/// AI生成实体类别, 通过 ai_generate_entity_id 与 ai_generate_entity 关联
struct DOODLE_CORE_API ai_category {
  DOODLE_BASE_FIELDS();
  std::string name_;                               ///< 类别名称 必填
  std::string description_;                        ///< 类别描述 选填
  ai_category_type type_{ai_category_type::shot};  ///< 类别类型 必填

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"name", &ai_category::name_},
        std::pair{"description", &ai_category::description_},
        // std::pair{"type", &ai_category::type_}
    };
  }

  // to json
  friend void to_json(nlohmann::json& j, const ai_category& p) {
    j["id"]          = p.uuid_id_;
    j["name"]        = p.name_;
    j["description"] = p.description_;
    j["type"]        = p.type_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_category& p) {
    j.at("name").get_to(p.name_);
    j.at("type").get_to(p.type_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
  }
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    ai_category_type, {
                          {ai_category_type::shot, "shot"},
                          {ai_category_type::assets, "assets"},
                      }
);
}  // namespace doodle::seedance2
