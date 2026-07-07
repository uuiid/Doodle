//
// Created by TD on 25-2-20.
//

#pragma once

#include "doodle_core/metadata/ai_studio.h"
#include "doodle_core/metadata/seedance2/assets_entity.h"
#include "doodle_core/metadata/seedance2/assets_entity_item.h"
#include "doodle_core/metadata/seedance2/group.h"
#include "doodle_core/metadata/seedance2/task.h"
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_image_metadata.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/attachment_file.h>
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/label.h>
#include <doodle_core/metadata/metadata_descriptor.h>
#include <doodle_core/metadata/notification.h>
#include <doodle_core/metadata/organisation.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/playlist.h>
#include <doodle_core/metadata/preview_background_file.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/subscription.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/metadata/working_file.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>

// clang-format off
#include <doodle_lib/sqlite_orm/detail/macro.h>
#include <doodle_lib/sqlite_orm/detail/nlohmann_json.h>
#include <doodle_lib/sqlite_orm/detail/std_chrono_duration.h>
#include <doodle_lib/sqlite_orm/detail/std_chrono_time_point.h>
#include <doodle_lib/sqlite_orm/detail/std_chrono_zoned_time.h>
#include <doodle_lib/sqlite_orm/detail/std_filesystem_path_orm.h>
#include <doodle_lib/sqlite_orm/detail/std_vector_string.h>
#include <doodle_lib/sqlite_orm/detail/uuid_to_blob.h>
#include <doodle_lib/sqlite_orm/detail/dynamic_where.h>
// clang-format on

#include <range/v3/view/split.hpp>

#include <type_traits>

namespace sqlite_orm {
DOODLE_SQLITE_ENUM_TYPE_(::doodle::computer_status)
DOODLE_SQLITE_ENUM_TYPE_(::doodle::server_task_info_status)
DOODLE_SQLITE_ENUM_TYPE_(::doodle::server_task_info_type)
DOODLE_SQLITE_ENUM_TYPE_(::doodle::project_styles)
// DOODLE_SQLITE_ENUM_TYPE_(doodle::details::assets_type_enum)
DOODLE_SQLITE_ENUM_TYPE_(::doodle::details::assets_type_enum);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::attendance_helper::att_enum);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::metadata_descriptor_data_type);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::two_factor_authentication_types);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::person_role_type);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::status_automation_change_type);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::preview_file_statuses);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::preview_file_validation_statuses);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::preview_file_source_enum);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::entity_status);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::contract_types);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::notification_type);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::software_enum);
DOODLE_SQLITE_ENUM_TYPE_(::doodle::seedance2::task_status);
DOODLE_SQLITE_ENUM_ARRAY_TYPE_(::doodle::person_role_type);

template <>
struct type_is_nullable<std::string> : std::true_type {
  bool operator()(const std::string& t) const { return t.empty(); }
};
template <>
struct type_is_nullable<boost::uuids::uuid> : std::true_type {
  bool operator()(const boost::uuids::uuid& t) const { return t.is_nil(); }
};
template <>
struct type_is_nullable<nlohmann::json> : std::true_type {
  bool operator()(const nlohmann::json& t) const { return t.is_null(); }
};
template <>
struct type_is_nullable<std::vector<std::string>> : std::true_type {
  bool operator()(const std::vector<std::string>& t) const { return t.empty(); }
};
template <>
struct type_is_nullable<doodle::FSys::path> : std::true_type {
  bool operator()(const doodle::FSys::path& t) const { return t.empty(); }
};
}  // namespace sqlite_orm

