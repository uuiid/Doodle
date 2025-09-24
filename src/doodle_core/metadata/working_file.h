//
// Created by TD on 25-8-12.
//

#pragma once
#include <doodle_core/metadata/base.h>
namespace doodle {

enum class software_enum { maya, unreal_engine };

NLOHMANN_JSON_SERIALIZE_ENUM(
    software_enum, {
                       {software_enum::maya, "maya"},  //
                       {software_enum::unreal_engine, "unreal_engine"},
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
  nlohmann::json data_;
  software_enum software_type_;

  uuid task_id_;
  uuid entity_id_;
  uuid person_id_;
  // uuid software_id_;
  // to_json
  friend void to_json(nlohmann::json& j, const working_file& p) {
    j["id"]            = p.uuid_id_;
    j["name"]          = p.name_;
    j["description"]   = p.description_;
    j["comment"]       = p.comment_;
    j["revision"]      = p.revision_;
    j["size"]          = p.size_;
    j["checksum"]      = p.checksum_;
    j["path"]          = p.path_;
    j["data"]          = p.data_;
    j["software_type"] = p.software_type_;
    j["task_id"]       = p.task_id_;
    j["entity_id"]     = p.entity_id_;
    j["person_id"]     = p.person_id_;
  }
  // from_json
  friend void from_json(const nlohmann::json& j, working_file& p) {
    j.at("id").get_to(p.uuid_id_);
    j.at("name").get_to(p.name_);
    j.at("description").get_to(p.description_);
    j.at("comment").get_to(p.comment_);
    j.at("revision").get_to(p.revision_);
    j.at("size").get_to(p.size_);
    j.at("checksum").get_to(p.checksum_);
    j.at("path").get_to(p.path_);
    j.at("data").get_to(p.data_);
    j.at("software_type").get_to(p.software_type_);
    j.at("task_id").get_to(p.task_id_);
    j.at("entity_id").get_to(p.entity_id_);
    j.at("person_id").get_to(p.person_id_);
  }
};
}  // namespace doodle