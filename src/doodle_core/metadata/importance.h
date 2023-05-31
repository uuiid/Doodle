//
// Created by TD on 2021/5/18.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
class DOODLE_CORE_API importance {
 private:
  friend void to_json(nlohmann::json &j, const importance &p);
  friend void from_json(const nlohmann::json &j, importance &p);

 public:
  std::string cutoff_p;
  importance();
  explicit importance(std::string in_cutoff_p);
  virtual ~importance();
};
}  // namespace doodle
