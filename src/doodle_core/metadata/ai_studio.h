//
// Created by TD on 25-3-18.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API ai_studio {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string color_;
  std::string app_key_;
  std::string app_secret_;
  bool archived_;

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"name", &ai_studio::name_},              //
        std::pair{"color", &ai_studio::color_},            //
        std::pair{"archived", &ai_studio::archived_},      //
        std::pair{"app_key", &ai_studio::app_key_},        //
        std::pair{"app_secret", &ai_studio::app_secret_},  //
        std::pair{"archived", &ai_studio::archived_}       //
    };
  }

  // to json
  friend void to_json(nlohmann::json& j, const ai_studio& p) {
    j["id"]       = p.uuid_id_;
    j["name"]     = p.name_;
    j["color"]    = p.color_;
    j["archived"] = p.archived_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_studio& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("color")) j.at("color").get_to(p.color_);
    if (j.contains("archived")) j.at("archived").get_to(p.archived_);
    if (j.contains("app_key")) j.at("app_key").get_to(p.app_key_);
    if (j.contains("app_secret")) j.at("app_secret").get_to(p.app_secret_);
  }
};
}  // namespace doodle