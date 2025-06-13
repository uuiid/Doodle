//
// Created by TD on 24-12-25.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include "base.h"
namespace doodle {

enum class metadata_descriptor_data_type {
  string,
  number,
  list,
  taglist,
  boolean,
  checklist,
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    metadata_descriptor_data_type,
    {
        {metadata_descriptor_data_type::string, "string"},
        {metadata_descriptor_data_type::number, "number"},
        {metadata_descriptor_data_type::list, "list"},
        {metadata_descriptor_data_type::taglist, "taglist"},
        {metadata_descriptor_data_type::boolean, "boolean"},
        {metadata_descriptor_data_type::checklist, "checklist"},
    }
)
struct metadata_descriptor_department_link {
  std::int64_t id_;
  uuid metadata_descriptor_uuid_;
  uuid department_uuid_;
};

struct metadata_descriptor {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string entity_type_;
  uuid project_uuid_;
  metadata_descriptor_data_type data_type_;
  std::string field_name_;
  nlohmann::json choices_;
  bool for_client_;
  std::vector<uuid> department_;
};
}  // namespace doodle