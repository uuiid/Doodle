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
  std::string seedance2_key_;
  std::string transfer_station_key_;
  bool archived_;

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"name", &ai_studio::name_},                                    //
        std::pair{"color", &ai_studio::color_},                                  //
        std::pair{"archived", &ai_studio::archived_},                            //
        std::pair{"seedance2_key", &ai_studio::seedance2_key_},                  //
        std::pair{"transfer_station_key", &ai_studio::transfer_station_key_},    //
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
    if (j.contains("seedance2_key")) j.at("seedance2_key").get_to(p.seedance2_key_);
    if (j.contains("transfer_station_key")) j.at("transfer_station_key").get_to(p.transfer_station_key_);
  }
};
}  // namespace doodle