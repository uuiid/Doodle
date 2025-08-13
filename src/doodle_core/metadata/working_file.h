//
// Created by TD on 25-8-12.
//

#pragma once
#include <doodle_core/metadata/base.h>
namespace doodle {
struct working_file {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string description_;
  std::string comment_;
  std::int32_t revision_{};
  std::int32_t size_;
  std::int32_t checksum_;
  FSys::path path_;
  nlohmann::json data_;

  uuid task_id_;
  uuid entity_id_;
  uuid person_id_;
  uuid software_id_;
};
}  // namespace doodle