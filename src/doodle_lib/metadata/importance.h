//
// Created by TD on 2021/5/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class importance {
 private:
  friend void to_json(nlohmann::json &j, const importance &p);
  friend void from_json(const nlohmann::json &j, importance &p);

 public:
  std::string cutoff_p;
  importance();
  importance(std::string in_cutoff_p);
  ~importance();
};
}  // namespace doodle
