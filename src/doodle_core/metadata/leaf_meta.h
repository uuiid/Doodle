//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>

#include <rttr/rttr_enable.h>

namespace doodle {
class DOODLE_CORE_API ref_meta {
  RTTR_ENABLE();

 private:
  friend void to_json(nlohmann::json &j, const ref_meta &p);
  friend void from_json(const nlohmann::json &j, ref_meta &p);

 public:
  std::vector<database::ref_data> ref_list;
  ref_meta();
};
}  // namespace doodle
