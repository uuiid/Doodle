//
// Created by TD on 25-8-12.
//

#pragma once
#include <doodle_core/metadata/base.h>
#include <doodle_core/metadata/entity.h>
namespace doodle {

enum class software_enum { maya, unreal_engine, alembic, unreal_engine_sk, maya_sim, maya_rig };

NLOHMANN_JSON_SERIALIZE_ENUM(
    software_enum, {
                       {software_enum::maya, "maya"},  //
                       {software_enum::unreal_engine, "unreal_engine"},
                       {software_enum::alembic, "alembic"},
                       {software_enum::unreal_engine_sk, "unreal_engine_sk"},
                       {software_enum::maya_sim, "maya_sim"},
                       {software_enum::maya_rig, "maya_rig"},
                       // Add other software types as needed
                   }
);

struct working_file {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string description_;
  std::string comment_;
  std::int32_t revision_{};
  std::int64_t size_;
  std::int32_t checksum_;
  FSys::path path_;
  software_enum software_type_;

  // uuid software_id_;
  // to_json
  friend void to_json(nlohmann::json& j, const working_file& p) {
    j["id"]            = p.uuid_id_;
    j["name"]          = p.name_;
    j["description"]   = p.description_;
    j["comment"]       = p.comment_;
    j["revision"]      = p.revision_;
    j["size"]          = p.size_;
    j["path"]          = p.path_;
    j["software_type"] = p.software_type_;
  }
  // from_json
  friend void from_json(const nlohmann::json& j, working_file& p) {
    j.at("id").get_to(p.uuid_id_);
    j.at("name").get_to(p.name_);
    j.at("description").get_to(p.description_);
    j.at("comment").get_to(p.comment_);
    j.at("revision").get_to(p.revision_);
    j.at("size").get_to(p.size_);
    j.at("path").get_to(p.path_);
    j.at("software_type").get_to(p.software_type_);
  }
};

struct working_file_and_link : working_file {
  uuid entity_id_;
  uuid entity_type_id_;
  bool is_exists_{};
  std::string entity_name_;
  std::string pin_yin_ming_cheng_;
  std::string bian_hao_;
  std::string ban_ben_;
  explicit working_file_and_link(
      const working_file& in_working_file, const entity& in_ent, const entity_asset_extend& in_entity_asset_extend
  )
      : working_file(in_working_file),
        entity_id_(in_ent.uuid_id_),
        entity_type_id_(in_ent.entity_type_id_),
        entity_name_(in_ent.name_),
        pin_yin_ming_cheng_(in_entity_asset_extend.pin_yin_ming_cheng_),
        bian_hao_(in_entity_asset_extend.bian_hao_),
        ban_ben_(in_entity_asset_extend.ban_ben_)

  {}

  // to json
  friend void to_json(nlohmann::json& j, const working_file_and_link& p) {
    to_json(j, static_cast<const working_file&>(p));
    j["entity_id"]          = p.entity_id_;
    j["entity_type_id"]     = p.entity_type_id_;
    j["is_exists"]          = p.is_exists_;
    j["entity_name"]        = p.entity_name_;
    j["pin_yin_ming_cheng"] = p.pin_yin_ming_cheng_;
    j["bian_hao"]           = p.bian_hao_;
    j["ban_ben"]            = p.ban_ben_;
  }
};
}  // namespace doodle