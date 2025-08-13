//
// Created by TD on 25-8-12.
//

#pragma once
#include <doodle_core/metadata/base.h>
namespace doodle {
struct software {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string short_name_;
  std::string file_extension_;
  nlohmann::json secondary_extensions_;
};
}  // namespace doodle