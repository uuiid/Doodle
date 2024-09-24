//
// Created by TD on 24-9-11.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/project.h>

#include <entt/entt.hpp>
namespace doodle {

namespace scan_data_t {

struct database_t {
  uuid uuid_id_;

  std::optional<uuid> ue_uuid_;
  std::optional<uuid> rig_uuid_;
  std::optional<uuid> solve_uuid_;
  std::int32_t project_id_;

  std::optional<std::filesystem::path> ue_path_;
  std::optional<std::filesystem::path> rig_path_;
  std::optional<std::filesystem::path> solve_path_;
  std::int32_t season_;
  details::assets_type_enum dep_{};

  std::string name_;
  std::optional<std::string> version_;
  std::optional<std::string> num_;

  std::string hash_;

  std::int32_t id_;
};
};  // namespace scan_data_t

}  // namespace doodle
