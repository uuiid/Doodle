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
  /// API易(apiyi.com) 的 key。与上两把是**不同主机**的凭据, 不能互相顶替:
  /// 拿 transfer_station_key_ 去发 API易 的请求等于把 A 家的密钥交给 B 家。
  /// 注意: 本列是新加的, 而 sync_schema() 不会对已存在的表做 ALTER TABLE
  /// (见 sqlite_orm/orm/session.cpp), 所以**已部署的库需要一次显式迁移**。
  std::string apipy_key_;
  /// Google Gemini(generativelanguage.googleapis.com) 的 key。同样是**另一台主机**的凭据。
  /// 与 apipy_key_ 一样, 已部署的库需要显式迁移才能出现这一列。
  std::string gemini_key_;
  bool archived_;

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"name", &ai_studio::name_},                                  //
        std::pair{"color", &ai_studio::color_},                                //
        std::pair{"archived", &ai_studio::archived_},                          //
        std::pair{"seedance2_key", &ai_studio::seedance2_key_},                //
        std::pair{"transfer_station_key", &ai_studio::transfer_station_key_},  //
        std::pair{"apipy_key", &ai_studio::apipy_key_},                        //
        std::pair{"gemini_key", &ai_studio::gemini_key_},                      //
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
    if (j.contains("apipy_key")) j.at("apipy_key").get_to(p.apipy_key_);
    if (j.contains("gemini_key")) j.at("gemini_key").get_to(p.gemini_key_);
  }
};
}  // namespace doodle