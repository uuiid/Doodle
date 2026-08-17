#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
///
struct DOODLE_CORE_API ai_preview_file {
  DOODLE_BASE_FIELDS();
  std::string extension_;  // 必填
};
}  // namespace doodle::seedance2