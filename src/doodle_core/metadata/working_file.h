//
// Created by TD on 25-8-12.
//

#pragma once
#include <doodle_core/metadata/base.h>
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

struct working_file_task_link {
  std::int64_t id_;
  uuid working_file_id_;
  uuid task_id_;
};
struct working_file_entity_link {
  std::int64_t id_;
  uuid working_file_id_;
  uuid entity_id_;
};

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

  // to json
  friend void to_json(nlohmann::json& j, const working_file_and_link& p) {
    to_json(j, static_cast<const working_file&>(p));
    j["entity_id"]      = p.entity_id_;
    j["entity_type_id"] = p.entity_type_id_;
  }
};
}  // namespace doodle