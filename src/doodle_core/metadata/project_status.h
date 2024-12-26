//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
/**
 * 描述项目的状态（主要是打开或关闭）。
 */
struct DOODLE_CORE_API project_status {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string color_;
};

}  // namespace doodle