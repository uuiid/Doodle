#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
///
struct DOODLE_CORE_API ai_generate_entity {
  DOODLE_BASE_FIELDS();
  std::string name_;                    // 必填
  uuid ai_generate_classification_id_;  // 必填

  uuid shot_uuid_id_;     // 选填 内部使用的UUID，对应镜头中的uuid_id_
  uuid project_uuid_id_;  // 必填 内部使用的UUID，对应项目中的uuid_id_
  uuid preview_file_;     // 选填 对应 ai_preview_file
};
}  // namespace doodle::seedance2